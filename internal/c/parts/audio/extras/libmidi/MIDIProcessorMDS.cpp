
/** $VER: MIDIProcessorMDS.cpp (2023.08.14) MIDI Stream. created by Microsoft with the release of Windows 95 (http://www.vgmpf.com/Wiki/index.php?title=MDS) **/

#include "framework.h"

#include "MIDIProcessor.h"

bool midi_processor_t::IsMDS(std::vector<uint8_t> const &data) {
    if (data.size() < 8)
        return false;

    if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F')
        return false;

    uint32_t Size = (uint32_t)(data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24));

    if ((Size < 8) || (data.size() < (size_t)(Size + 8)))
        return false;

    if (data[8] != 'M' || data[9] != 'I' || data[10] != 'D' || data[11] != 'S' || data[12] != 'f' || data[13] != 'm' || data[14] != 't' || data[15] != ' ')
        return false;

    return true;
}

bool midi_processor_t::ProcessMDS(std::vector<uint8_t> const &data, midi_container_t &container) {
    if (data.size() < 20)
        return false;

    auto it = data.begin() + 16;
    auto end = data.end();

    uint32_t TimeFormat = 1;
    uint32_t Flags = 0;

    // Format chunk
    {
        uint32_t FormatSize = (uint32_t)(it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
        it += 4;

        if ((uint32_t)(end - it) < FormatSize)
            return false;

        if (FormatSize >= 4) {
            // dwTimeFormat. Low word - time format in SMF format
            TimeFormat = (uint32_t)(it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
            it += 4;
            FormatSize -= 4;

            if (TimeFormat == 0) // dtx == 0, will cause division by zero on tempo calculations
                return false;
        }

        if (FormatSize >= 4) {
            // cbMaxBuffer. Guaranteed max buffer size (default is 4096)
            it += 4;
            FormatSize -= 4;
        }

        if (FormatSize >= 4) {
            // dwFlags. Format flags (default is 1)
            Flags = (uint32_t)(it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
            it += 4;
            FormatSize -= 4;
        }

        it += (int)FormatSize;

        if (it == end)
            return false;

        if (FormatSize & 1)
            ++it;
    }

    container.Initialize(0, TimeFormat);

    if (end - it < 4)
        return false;

    // Data chunk
    if (it[0] != 'd' || it[1] != 'a' || it[2] != 't' || it[3] != 'a')
        return false; /*throw exception_io_data( "MIDS missing RIFF data chunk" );*/

    it += 4;

    {
        midi_track_t Track;

        Track.AddEvent(midi_event_t(0, midi_event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));
        container.AddTrack(Track);
    }

    if (end - it < 4)
        return false;

    uint32_t DataSize = (uint32_t)(it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
    it += 4;

    auto BodyEnd = it + (int)DataSize;

    if (BodyEnd - it < 4)
        return false;

    uint32_t SegmentCount = (uint32_t)(it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
    it += 4;

    bool IsEightByte = !!(Flags & 1);

    midi_track_t Track;

    uint32_t Timestamp = 0;

    for (uint32_t i = 0; i < SegmentCount; ++i) {
        if (end - it < 12)
            return false;

        it += 4;

        uint32_t SegmentSize = (uint32_t)(it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
        it += 4;

        auto SegmentEnd = it + (int)SegmentSize;

        while ((it != SegmentEnd) && (it != BodyEnd)) {
            if (SegmentEnd - it < 4)
                return false;

            uint32_t Delta = (uint32_t)(it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
            it += 4;

            Timestamp += Delta;

            if (!IsEightByte) {
                if (SegmentEnd - it < 4)
                    return false;

                it += 4;
            }

            if (SegmentEnd - it < 4)
                return false;

            uint32_t Event = (uint32_t)(it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24));
            it += 4;

            if ((Event >> 24) == 0x01) {
                uint8_t Data[5] = {StatusCodes::MetaData, MetaDataTypes::SetTempo};

                Data[2] = (uint8_t)(Event >> 16);
                Data[3] = (uint8_t)(Event >> 8);
                Data[4] = (uint8_t)Event;

                container.AddEventToTrack(0, midi_event_t(Timestamp, midi_event_t::Extended, 0, Data, sizeof(Data)));
            } else if ((Event >> 24) == 0x00) {
                unsigned int StatusCode = (Event & 0xF0) >> 4;

                if (StatusCode >= 0x08 && StatusCode <= 0x0E) {
                    uint8_t Data[2] = {(uint8_t)(Event >> 8)};
                    size_t Size = 1;

                    if (StatusCode != 0x0C && StatusCode != 0x0D) {
                        Data[1] = (uint8_t)(Event >> 16);
                        Size = 2;
                    }

                    Track.AddEvent(midi_event_t(Timestamp, (midi_event_t::event_type_t)(StatusCode - 8), Event & 0x0F, Data, Size));
                }
            }
        }
    }

    Track.AddEvent(midi_event_t(Timestamp, midi_event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

    container.AddTrack(Track);

    return true;
}
