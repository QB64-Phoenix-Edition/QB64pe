
/** $VER: MIDIProcessorXMI.cpp (2024.05.16) Extended Multiple Instrument Digital Interface (http://www.vgmpf.com/Wiki/index.php?title=XMI) **/

#include "framework.h"

#include "MIDIProcessor.h"

/// <summary>
/// Returns true if the byte vector contains XMI data.
/// </summary>
bool midi_processor_t::IsXMI(std::vector<uint8_t> const &data) {
    if (data.size() < 34)
        return false;

    if (data[0] != 'F' || data[1] != 'O' || data[2] != 'R' || data[3] != 'M' || data[8] != 'X' || data[9] != 'D' || data[10] != 'I' || data[11] != 'R' ||
        data[30] != 'X' || data[31] != 'M' || data[32] != 'I' || data[33] != 'D')
        return false;

    return true;
}

/// <summary>
/// Processes a byte vector with XMI data.
/// </summary>
bool midi_processor_t::ProcessXMI(std::vector<uint8_t> const &data, midi_container_t &container) {
    iff_stream_t Stream;

    if (!ReadStream(data, Stream))
        return false;

    const iff_chunk_t &FORMChunk = Stream.FindChunk(FOURCC_FORM);

    if (FORMChunk.Type != FOURCC_XDIR)
        throw MIDIException("FORM XDIR chunk not found");

    const iff_chunk_t &CATChunk = Stream.FindChunk(FOURCC_CAT);

    if (CATChunk.Type != FOURCC_XMID)
        throw MIDIException("CAT XMID chunk not found");

    uint32_t TrackCount = CATChunk.GetChunkCount(FOURCC_FORM);

    container.Initialize(TrackCount > 1 ? 2u : 0u, 60);

    for (uint32_t i = 0; i < TrackCount; ++i) {
        const iff_chunk_t &SubFORMChunk = CATChunk.FindChunk(FOURCC_FORM, i);

        if (SubFORMChunk.Type != FOURCC_XMID)
            throw MIDIException("FORM XMID chunk not found");

        const iff_chunk_t &EVNTChunk = SubFORMChunk.FindChunk(FOURCC_EVNT);

        if (EVNTChunk.Id != FOURCC_EVNT)
            throw MIDIException("EVNT chunk not found");

        std::vector<uint8_t> const &Data = EVNTChunk._Data;

        {
            midi_track_t Track;

            bool IsTempoSet = false;

            uint32_t CurrentTimestamp = 0;
            uint32_t LastEventTimestamp = 0;

            std::vector<uint8_t> Temp(3);

            auto it = Data.begin(), end = Data.end();

            while (it != end) {
                uint32_t Delta = DecodeVariableLengthQuantityXMI(it, end);

                CurrentTimestamp += Delta;

                if (CurrentTimestamp > LastEventTimestamp)
                    LastEventTimestamp = CurrentTimestamp;

                if (it == end)
                    throw MIDIException("Insufficient data in the stream");

                Temp[0] = *it++;

                if (Temp[0] == StatusCodes::MetaData) {
                    if (it == end)
                        throw MIDIException("Insufficient data in the stream");

                    Temp[1] = *it++;

                    long Size = 0;

                    if (Temp[1] == MetaDataTypes::EndOfTrack) {
                        if (LastEventTimestamp > CurrentTimestamp)
                            CurrentTimestamp = LastEventTimestamp;
                    } else {
                        Size = DecodeVariableLengthQuantity(it, end);

                        if (Size < 0)
                            throw MIDIException("Invalid meta data message");

                        if (end - it < Size)
                            throw MIDIException("Insufficient data in the stream");

                        Temp.resize((size_t)(Size + 2));
                        std::copy(it, it + Size, Temp.begin() + 2);
                        it += Size;

                        if ((Temp[1] == MetaDataTypes::SetTempo) && (Size == 3)) {
                            uint32_t Tempo = (uint32_t)(Temp[2] * 0x10000 + Temp[3] * 0x100 + Temp[4]);
                            uint32_t PpQN = (Tempo * 3) / 25000;

                            Tempo = Tempo * 60 / PpQN;

                            Temp[2] = (uint8_t)(Tempo / 0x10000);
                            Temp[3] = (uint8_t)(Tempo / 0x100);
                            Temp[4] = (uint8_t)(Tempo);

                            if (CurrentTimestamp == 0)
                                IsTempoSet = true;
                        }
                    }

                    Track.AddEvent(midi_event_t(CurrentTimestamp, midi_event_t::Extended, 0, &Temp[0], (size_t)(Size + 2)));

                    if (Temp[1] == MetaDataTypes::EndOfTrack)
                        break;
                } else if (Temp[0] == StatusCodes::SysEx) {
                    long Size = DecodeVariableLengthQuantity(it, end);

                    if (Size < 0)
                        throw MIDIException("Invalid System Exclusive message");

                    if (end - it < Size)
                        throw MIDIException("Insufficient data in the stream");

                    Temp.resize((size_t)(Size + 1));
                    std::copy(it, it + Size, Temp.begin() + 1);
                    it += Size;

                    Track.AddEvent(midi_event_t(CurrentTimestamp, midi_event_t::Extended, 0, &Temp[0], (size_t)(Size + 1)));
                } else if (Temp[0] >= StatusCodes::NoteOff && Temp[0] <= StatusCodes::ActiveSensing) {
                    if (it == end)
                        throw MIDIException("Insufficient data in the stream");

                    Temp.resize(3);

                    Temp[1] = *it++;
                    uint32_t BytesRead = 1;

                    midi_event_t::event_type_t Type = (midi_event_t::event_type_t)((Temp[0] >> 4) - 8);
                    uint32_t Channel = (uint32_t)(Temp[0] & 0x0F);

                    if ((Type != midi_event_t::ProgramChange) && (Type != midi_event_t::ChannelPressure)) {
                        if (it == end)
                            throw MIDIException("Insufficient data in the stream");

                        Temp[2] = *it++;
                        BytesRead = 2;
                    }

                    Track.AddEvent(midi_event_t(CurrentTimestamp, Type, Channel, &Temp[1], BytesRead));

                    if (Type == midi_event_t::NoteOn) {
                        Temp[2] = 0x00;

                        int Length = DecodeVariableLengthQuantity(it, end);

                        if (Length < 0)
                            throw MIDIException("Invalid note message");

                        uint32_t Timestamp = CurrentTimestamp + Length;

                        if (Timestamp > LastEventTimestamp)
                            LastEventTimestamp = Timestamp;

                        Track.AddEvent(midi_event_t(Timestamp, Type, Channel, &Temp[1], BytesRead));
                    }
                } else
                    throw MIDIException("Unknown status code");
            }

            if (!IsTempoSet)
                Track.AddEvent(midi_event_t(0, midi_event_t::Extended, 0, DefaultTempoXMI, _countof(DefaultTempoXMI)));

            container.AddTrack(Track);
        }
    }

    return true;
}

/// <summary>
/// Reads a byte vector and converts it to a stream of chunks.
/// </summary>
bool midi_processor_t::ReadStream(std::vector<uint8_t> const &data, iff_stream_t &stream) {
    auto it = data.begin(), end = data.end();

    bool IsFirstChunk = true;

    while (it != end) {
        iff_chunk_t Chunk;

        if (ReadChunk(it, end, Chunk, IsFirstChunk)) {
            stream._Chunks.push_back(Chunk);
            IsFirstChunk = false;
        } else if (IsFirstChunk)
            return false;
        else
            break;
    }

    return true;
}

/// <summary>
/// Reads a chunk from a byte vector.
/// </summary>
bool midi_processor_t::ReadChunk(std::vector<uint8_t>::const_iterator &it, std::vector<uint8_t>::const_iterator end, iff_chunk_t &chunk, bool isFirstChunk) {
    if (end - it < 8)
        return false;

    std::copy(it, it + 4, (uint8_t *)&chunk.Id);
    it += 4;

    uint32_t ChunkSize = (uint32_t)(it[0] << 24) | (it[1] << 16) | (it[2] << 8) | it[3];

    if ((size_t)(end - it) < ChunkSize)
        return false;

    it += 4;

    bool IsCATChunk = (chunk.Id == FOURCC_CAT);
    bool IsFORMChunk = (chunk.Id == FOURCC_FORM);

    const size_t ChunkSizeLimit = (size_t)(end - it);

    if (ChunkSize > ChunkSizeLimit)
        ChunkSize = (uint32_t)ChunkSizeLimit;

    if ((isFirstChunk && IsFORMChunk) || (!isFirstChunk && IsCATChunk)) {
        if (end - it < 4)
            return false;

        // Read the sub-chunks of a FORM or CAT chunk.
        auto ChunkEnd = it + (int)ChunkSize;

        std::copy(it, it + 4, (uint8_t *)&chunk.Type);
        it += 4;

        while (it < ChunkEnd) {
            iff_chunk_t SubChunk;

            if (!ReadChunk(it, ChunkEnd, SubChunk, IsCATChunk))
                return false;

            chunk._Chunks.push_back(SubChunk);
        }

        it = ChunkEnd;

        if ((ChunkSize & 1) && (it != end))
            ++it;
    } else if (!IsFORMChunk && !IsCATChunk) {
        chunk._Data.assign(it, it + (int)ChunkSize);
        it += (int)ChunkSize;

        if ((ChunkSize & 1) && (it != end))
            ++it;
    } else {
        /*      if (first_chunk)
                    throw exception_io_data( pfc::string_formatter() << "Found " << pfc::string8( (const char *)p_out.m_id, 4 ) << " chunk instead of FORM" );
                else
                    throw exception_io_data("Found multiple FORM chunks");
        */
        return false;
    }

    return true;
}

/// <summary>
/// Decodes a variable length quantity.
/// </summary>
uint32_t midi_processor_t::DecodeVariableLengthQuantityXMI(std::vector<uint8_t>::const_iterator &it, std::vector<uint8_t>::const_iterator end) noexcept {
    uint32_t Quantity = 0;

    if (it == end)
        return 0;

    uint8_t Byte = *it++;

    if (!(Byte & 0x80)) {
        do {
            Quantity += Byte;

            if (it == end)
                break;

            Byte = *it++;
        } while (!(Byte & 0x80) && it != end);
    }

    --it;

    return Quantity;
}

const uint8_t midi_processor_t::DefaultTempoXMI[5] = {StatusCodes::MetaData, MetaDataTypes::SetTempo, 0x07, 0xA1, 0x20};
