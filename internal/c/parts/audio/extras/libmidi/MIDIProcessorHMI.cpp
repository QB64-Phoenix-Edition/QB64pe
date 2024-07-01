
/** $VER: MIDIProcessorHMI.cpp (2023.08.19) Human Machine Interface (http://www.vgmpf.com/Wiki/index.php?title=HMI) **/

#include "framework.h"

#include "MIDIProcessor.h"

/// <summary>
/// Returns true if data points to an HMI sequence.
/// </summary>
bool midi_processor_t::IsHMI(std::vector<uint8_t> const &data) {
    if (data.size() < 12)
        return false;

    const char Id[] = {'H', 'M', 'I', '-', 'M', 'I', 'D', 'I', 'S', 'O', 'N', 'G'};

    return (::memcmp(data.data(), Id, _countof(Id)) == 0);
}

/// <summary>
/// Processes the sequence data.
/// </summary>
bool midi_processor_t::ProcessHMI(std::vector<uint8_t> const &data, midi_container_t &container) {
    auto it = data.begin() + 0xE4;

    uint32_t TrackCount = (uint32_t)(it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
    uint32_t Offsets = (uint32_t)(it[4] | (it[5] << 8) | (it[6] << 16) | (it[7] << 24));

    if (Offsets >= data.size() || Offsets + (size_t)(TrackCount * 4) > data.size())
        throw MIDIException("Insufficient data for track offsets");

    // Read the track offsets.
    it = data.begin() + (int)Offsets;

    std::vector<uint32_t> TrackOffsets(TrackCount);

    for (size_t i = 0; i < TrackCount; ++i) {
        TrackOffsets[i] = (uint32_t)(it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
        it += 4;
    }

    // Add a conductor track.
    container.Initialize(1, 0xC0);

    {
        midi_track_t Track;

        uint8_t Data[] = {StatusCodes::MetaData, MetaDataTypes::SetTempo, 0, 0, 0};

        {
            uint32_t us = (uint32_t)(60 * 1000 * 1000) / _Options._DefaultTempo; // Convert from bpm to �s / quarter note.

            Data[4] = us & 0x7F;
            us >>= 7;

            if (us != 0) {
                Data[3] = 0x80 | (us & 0x7F);
                us >>= 7;
            }
            if (us != 0) {
                Data[2] = us & 0x7F;
            }
        }

        Track.AddEvent(midi_event_t(0, midi_event_t::Extended, 0, Data, _countof(Data)));
        Track.AddEvent(midi_event_t(0, midi_event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

        container.AddTrack(Track);
    }

    // Process each track.
    std::vector<uint8_t> Temp;

    for (uint32_t i = 0; i < TrackCount; ++i) {
        uint32_t Offs = TrackOffsets[i];

        uint32_t Size;

        if (i + 1 < TrackCount)
            Size = TrackOffsets[(size_t)(i + 1)] - Offs;
        else
            Size = (uint32_t)data.size() - Offs;

        if ((Size < 13) || (Offs >= data.size()) || ((size_t)(Offs + Size) > data.size()))
            throw MIDIException(std::string("Insufficient data for track " + std::to_string(i + 1)));

        auto Data = data.begin() + (int)Offs;
        auto Tail = Data + (int)Size;

        const char Id[] = {'H', 'M', 'I', '-', 'M', 'I', 'D', 'I', 'T', 'R', 'A', 'C', 'K'};

        if (::memcmp(&Data[0], Id, _countof(Id)) != 0)
            throw MIDIException(std::string("Invalid data for track " + std::to_string(i + 1)));

        midi_track_t Track;

        uint32_t RunningTime = 0;
        uint8_t RunningStatus = 0xFF;

        uint32_t LastTime = 0;

        if (Size < 0x4B + 4)
            throw MIDIException("Insufficient data for metadata");

        // Convert the metadata.
        {
            uint32_t MetaOffset = (uint32_t)(Data[0x4B] | (Data[0x4C] << 8) | (Data[0x4D] << 16) | (Data[0x4E] << 24));

            if ((MetaOffset > 0) && (MetaOffset + 1 < Size)) {
                Temp.resize(2);
                std::copy(Data + (int)MetaOffset, Data + (int)MetaOffset + 2, Temp.begin());

                uint32_t MetadataSize = Temp[1];

                if (MetaOffset + 2 + MetadataSize > Size)
                    throw MIDIException("Insufficient data for metadata");

                Temp.resize((size_t)(MetadataSize + 2));
                std::copy(Data + (int)MetaOffset + 2, Data + (int)MetaOffset + 2 + (int)MetadataSize, Temp.begin() + 2);

                // Truncate trailing spaces.
                while ((MetadataSize > 0) && Temp[(size_t)(MetadataSize + 1)] == ' ')
                    --MetadataSize;

                if (MetadataSize > 0) {
                    Temp[0] = StatusCodes::MetaData;
                    Temp[1] = MetaDataTypes::Text;

                    Track.AddEvent(midi_event_t(0, midi_event_t::Extended, 0, Temp.data(), MetadataSize + 2));
                }
            }
        }

        if (Size < 0x57 + 4)
            throw MIDIException("Insufficient data for HMI events");

        uint32_t TrackDataOffset = (uint32_t)(Data[0x57] | (Data[0x58] << 8) | (Data[0x59] << 16) | (Data[0x5A] << 24));

        it = Data + (int)TrackDataOffset;

        Temp.resize(3);

        while (it != Tail) {
            {
                int DeltaTime = DecodeVariableLengthQuantity(it, Tail);

                if (DeltaTime < 0 || DeltaTime > 0xFFFF) {
                    RunningTime = LastTime; /*console::formatter() << "[foo_midi] Large HMI delta detected, shunting.";*/
                } else {
                    RunningTime += DeltaTime;

                    if (RunningTime > LastTime)
                        LastTime = RunningTime;
                }
            }

            if (it == Tail)
                throw MIDIException("Insufficient data for HMI events");

            Temp[0] = *it++;

            if (Temp[0] == StatusCodes::MetaData) {
                RunningStatus = 0xFF;

                if (it == Tail)
                    throw MIDIException("Insufficient data for HMI meta data event");

                Temp[1] = *it++;

                int MetadataSize = DecodeVariableLengthQuantity(it, Tail);

                if (MetadataSize < 0)
                    throw MIDIException("Invalid HMI metadata event");

                if (Tail - it < MetadataSize)
                    throw MIDIException("Insufficient data for HMI metadata event");

                Temp.resize((size_t)(MetadataSize + 2));
                std::copy(it, it + MetadataSize, Temp.begin() + 2);

                it += MetadataSize;

                if ((Temp[1] == MetaDataTypes::EndOfTrack) && (LastTime > RunningTime))
                    RunningTime = LastTime;

                Track.AddEvent(midi_event_t(RunningTime, midi_event_t::Extended, 0, &Temp[0], (size_t)(MetadataSize + 2)));

                if (Temp[1] == MetaDataTypes::EndOfTrack)
                    break;
            } else if (Temp[0] == StatusCodes::SysEx) {
                RunningStatus = 0xFF;

                int SysExSize = DecodeVariableLengthQuantity(it, Tail);

                if (SysExSize < 0)
                    throw MIDIException("Invalid HMI SysEx event");

                if (Tail - it < SysExSize)
                    throw MIDIException("Insufficient data for HMI SysEx event");

                Temp.resize((size_t)(SysExSize + 1));
                std::copy(it, it + SysExSize, Temp.begin() + 1);

                it += SysExSize;
                Track.AddEvent(midi_event_t(RunningTime, midi_event_t::Extended, 0, &Temp[0], (size_t)(SysExSize + 1)));
            } else if (Temp[0] == StatusCodes::ActiveSensing) {
                RunningStatus = 0xFF;

                if (it == Tail)
                    throw MIDIException("Insufficient data for HMI Active Sensing event");

                Temp[1] = *it++;

                if (Temp[1] == 0x10) {
                    if (Tail - it < 3)
                        throw MIDIException("Insufficient data for HMI Active Sensing event");

                    it += 2;
                    Temp[2] = *it++;

                    if (Tail - it < ((long)Temp[2] + 4))
                        throw MIDIException("Insufficient data for HMI Active Sensing event");

                    it += ((long)Temp[2] + 4);
                } else if (Temp[1] == 0x12) {
                    if (Tail - it < 2)
                        throw MIDIException("Insufficient data for HMI Active Sensing event");

                    it += 2;
                } else if (Temp[1] == 0x13) {
                    if (Tail - it < 10)
                        throw MIDIException("Insufficient data for HMI Active Sensing event");

                    it += 10;
                } else if (Temp[1] == 0x14) {
                    if (Tail - it < 2)
                        throw MIDIException("Insufficient data for HMI Active Sensing event");

                    it += 2;
                    container.AddEventToTrack(0, midi_event_t(RunningTime, midi_event_t::Extended, 0, LoopBeginMarker, _countof(LoopBeginMarker)));
                } else if (Temp[1] == 0x15) {
                    if (Tail - it < 6)
                        throw MIDIException("Insufficient data for HMI Active Sensing event");

                    it += 6;
                    container.AddEventToTrack(0, midi_event_t(RunningTime, midi_event_t::Extended, 0, LoopEndMarker, _countof(LoopEndMarker)));
                } else
                    throw MIDIException("Invalid HMI Active Sensing event");
            } else if (Temp[0] < StatusCodes::SysEx) {
                size_t BytesRead = 1;

                if (Temp[0] >= StatusCodes::NoteOff) {
                    if (it == Tail)
                        throw MIDIException("Insufficient data for HMI message");

                    Temp[1] = *it++;
                    RunningStatus = Temp[0];
                } else {
                    if (RunningStatus == 0xFF)
                        throw MIDIException("Invalid shortened HMI event after metadata or SysEx event");

                    Temp[1] = Temp[0];
                    Temp[0] = RunningStatus;
                }

                midi_event_t::event_type_t Type = (midi_event_t::event_type_t)((Temp[0] >> 4) - 8);

                uint32_t Channel = (uint32_t)(Temp[0] & 0x0F);

                if ((Type != midi_event_t::ProgramChange) && (Type != midi_event_t::ChannelPressure)) {
                    if (it == Tail)
                        throw MIDIException("Insufficient data for HMI event");

                    Temp[2] = *it++;
                    BytesRead = 2;
                }

                Track.AddEvent(midi_event_t(RunningTime, Type, Channel, &Temp[1], BytesRead));

                // Add a NoteOff event after a NoteOn event.
                if (Type == midi_event_t::NoteOn) {
                    Temp[2] = 0x00;

                    int DeltaTime = DecodeVariableLengthQuantity(it, Tail);

                    if (DeltaTime < 0)
                        throw MIDIException("Invalid HMI note event");

                    uint32_t EndTime = RunningTime + DeltaTime;

                    if (EndTime > LastTime)
                        LastTime = EndTime;

                    Track.AddEvent(midi_event_t(EndTime, midi_event_t::NoteOff, Channel, Temp.data() + 1, BytesRead));
                }
            } else
                throw MIDIException("Invalid status code");
        }

        container.AddTrack(Track);
    }

    return true;
}
