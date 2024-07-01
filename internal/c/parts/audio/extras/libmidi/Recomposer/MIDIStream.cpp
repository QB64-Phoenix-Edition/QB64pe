
/** $VER: MIDIStream.cpp (2024.05.12) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "MIDIStream.h"

#include "Support.h"

// Optional callback for injecting raw data before writing a MIDI timestamp. Returning non-zero makes it skip writing the timestamp.
uint8_t (*midi_stream_t::_HandleDuration)(midi_stream_t *midiStream, uint32_t &duration);

/// <summary>
/// Writes a Roland SysEx message in chunks.
/// </summary>
void midi_stream_t::WriteRolandSysEx(const uint8_t *syxHdr, uint32_t address, const uint8_t *data, uint32_t size, uint8_t opts, uint32_t blockSize) {
    uint32_t Data = ((address & 0x00007F) >> 0) | ((address & 0x007F00) >> 1) | ((address & 0x7F0000) >> 2);

    for (uint32_t Offs = 0; Offs < size;) {
        uint32_t Size = std::min(size - Offs, blockSize);
        uint32_t syxOffs = ((Data & 0x00007F) << 0) | ((Data & 0x003F80) << 1) | ((Data & 0x1FC000) << 2);

        WriteRolandSysEx(syxHdr, syxOffs, data + Offs, Size, opts);

        Data += Size;
        Offs += Size;
    }
}

/// <summary>
/// Writes a Roland SysEx message.
/// </summary>
void midi_stream_t::WriteRolandSysEx(const uint8_t *syxHdr, uint32_t address, const uint8_t *data, uint32_t size, uint8_t opts) {
    const uint32_t Size = 0x09 + size;

    WriteTimestamp();

    Ensure(1 + 4 + Size); // Worst case: 4 bytes of data length

    _Data[_Offs++] = 0xF0;

    WriteVariableLengthQuantity(Size);

    _Data[_Offs + 0x00] = syxHdr[0];
    _Data[_Offs + 0x01] = syxHdr[1];
    _Data[_Offs + 0x02] = syxHdr[2];
    _Data[_Offs + 0x03] = syxHdr[3];

    _Data[_Offs + 0x04] = (address >> 16) & 0x7F;
    _Data[_Offs + 0x05] = (address >> 8) & 0x7F;
    _Data[_Offs + 0x06] = (address >> 0) & 0x7F;

    ::memcpy(_Data + _Offs + 0x07, data, size);

    {
        uint8_t Checksum = 0;

        for (uint32_t i = 0x04; i < 0x07 + size; i++)
            Checksum += _Data[_Offs + i];

        _Data[_Offs + size + 0x07] = (uint8_t)((-Checksum) & 0x7F);
        _Data[_Offs + size + 0x08] = 0xF7;
    }

    _Offs += Size;

    if (opts & SYXOPT_DELAY)
        _State.Duration += MulDivCeil(1 + Size, _TicksPerQuarter * 320, _Tempo); // F0 status code + data size
}
