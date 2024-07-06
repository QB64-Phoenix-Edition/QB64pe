
/** $VER: GDSFile.cpp (2024.05.09) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "RCP.h"

static void ConvertParameter2Bulk(const uint8_t *srcData, uint8_t *dstData);
static void ConvertBytes2Nibbles(const uint8_t *srcData, size_t srcSize, uint8_t *dstData);

/// <summary>
///
/// </summary>
void gsd_file_t::Read(const buffer_t &data) {
    uint8_t FileType = rcp_converter_t::GetFileType(data);

    if (FileType != 0x11)
        throw std::runtime_error("Invalid CM6 control file.");

    if (data.Size < 0x0A71)
        throw std::runtime_error("Insufficient data.");

    _Data = data;

    sysParams = &_Data.Data[0x0020];
    reverbParams = &_Data.Data[0x0027];
    chorusParams = &_Data.Data[0x002E];
    partParams = &_Data.Data[0x0036];
    drumSetup = &_Data.Data[0x07D6];
    masterTune = &_Data.Data[0x0A6E];
}

/// <summary>
///
/// </summary>
void rcp_converter_t::Convert(const gsd_file_t &gsdFile, midi_stream_t &midiStream, uint8_t mode) {
    static const uint8_t Part2Channel[0x10] = {0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    static const uint8_t Channel2Part[0x10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

    // The order follows how Recomposer 3.0 sends the data.
    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "SC-55 Common Settings");

    // Recomposer 3.0 sends Master Volume (40 00 04), Key-Shift (40 00 06) and Pan (via GM SysEx) separately, but doing a bulk-dump works just fine on SC-55/88.
    midiStream.WriteRolandSysEx(SysExHeaderSC55, 0x400000, gsdFile.sysParams, 0x07, SYXOPT_DELAY);

    {
        uint8_t VoiceReserve[0x10] = {};

        for (size_t i = 0; i < _countof(VoiceReserve); ++i)
            VoiceReserve[i] = gsdFile.partParams[Part2Channel[i] * 0x7A + 0x79];

        if (mode & 0x10)
            midiStream.WriteMetaEvent(MetaDataTypes::Text, "SC-55 Voice Reserve");

        midiStream.WriteRolandSysEx(SysExHeaderSC55, 0x400110, VoiceReserve, _countof(VoiceReserve), SYXOPT_DELAY);
    }

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "SC-55 Reverb Settings");

    midiStream.WriteRolandSysEx(SysExHeaderSC55, 0x400130, gsdFile.reverbParams, 0x07, SYXOPT_DELAY);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "SC-55 Chorus Settings");

    midiStream.WriteRolandSysEx(SysExHeaderSC55, 0x400138, gsdFile.chorusParams, 0x08, SYXOPT_DELAY);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "SC-55 Part Settings");

    {
        uint8_t Bulk[0x100] = {};

        for (uint8_t i = 0; i < 0x10; ++i) // Each channel
        {
            uint32_t addrOfs = (uint32_t)(0x90 + (Channel2Part[i] * 0xE0));
            uint32_t syxAddr = ((addrOfs & 0x007F) << 0) | ((addrOfs & 0x3F80) << 1);

            ConvertParameter2Bulk(&gsdFile.partParams[i * 0x7A], Bulk);

            midiStream.WriteRolandSysEx(SysExHeaderSC55, 0x480000 | syxAddr, Bulk, 0xE0, SYXOPT_DELAY, 0x80);
        }

        if (mode & 0x10)
            midiStream.WriteMetaEvent(MetaDataTypes::Text, "SC-55 Drum Setup");

        for (size_t i = 0; i < 2; ++i) // Each drum map
        {
            // Drum level, pan, reverb, chorus
            static const uint8_t DRMPAR_ADDR[4] = {0x02, 0x06, 0x08, 0x0A};

            const uint8_t *drmPtr = &gsdFile.drumSetup[i * 0x014C];

            uint8_t Parameter[0x80];

            for (size_t j = 0; j < 4; ++j) {
                uint32_t syxAddr = (uint32_t)(i << 12) | (DRMPAR_ADDR[j] << 8);

                ::memset(Parameter, 0, sizeof(Parameter));

                for (size_t k = 0; k < 82; k++) // Each note
                    Parameter[27 + k] = drmPtr[j + (k * 4)];

                ConvertBytes2Nibbles(Parameter, sizeof(Parameter), Bulk);

                midiStream.WriteRolandSysEx(SysExHeaderSC55, 0x490000 | syxAddr, Bulk, sizeof(Bulk), SYXOPT_DELAY, 0x80);
            }
        }
    }

    // Recomposer 3.0 doesn't seem to send SysEx for the additional Master Tuning settings.
    // gsdInf->masterTune;

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "Setup Finished.");
}

/// <summary>
///
/// </summary>
static void ConvertParameter2Bulk(const uint8_t *srcData, uint8_t *dstData) {
    uint8_t Part[0x70] = {};

    Part[0x00] = srcData[0x00]; // Bank MSB
    Part[0x01] = srcData[0x01]; // Tone number

    // Rx. Pitch Bend / Ch. Pressure / Program Change / Control Change / Poly Pressure / Note Message / RPN / NRPN
    Part[0x02] = 0x00;

    for (uint8_t i = 0; i < 8; i++)
        Part[0x02] |= (srcData[0x03 + i] & 0x01) << (7 - i);

    // Rx. Modulation/Volume/Panpot/Expression/Hold 1 (Sustain)/Portamento/SostenutoSoft Pedal
    Part[0x03] = 0x00;

    for (size_t i = 0; i < 8; i++)
        Part[0x03] |= (srcData[0x0B + i] & 0x01) << (7 - i);

    Part[0x04] = srcData[0x02]; // Rx. Channel

    Part[0x05] = (uint8_t)((srcData[0x13] & 0x01) << 7);                         // Mono/Poly Mode
    Part[0x05] |= ((srcData[0x15] & 0x03) << 5) | (srcData[0x15] ? 0x10 : 0x00); // Rhythm Part Mode
    Part[0x05] |= (srcData[0x14] & 0x03) << 0;                                   // Assign Mode

    Part[0x06] = srcData[0x16];                                                     // Pitch Key Shift
    Part[0x07] = (uint8_t)(((srcData[0x17] & 0x0F) << 4) | (srcData[0x18] & 0x0F)); // Pitch Offset Fine
    Part[0x08] = srcData[0x19];                                                     // Part Level
    Part[0x09] = srcData[0x1C];                                                     // Part Panpot
    Part[0x0A] = srcData[0x1B];                                                     // Velocity Sense Offset
    Part[0x0B] = srcData[0x1A];                                                     // Velocity Sense Depth
    Part[0x0C] = srcData[0x1D];                                                     // Key Range Low
    Part[0x0D] = srcData[0x1E];                                                     // Key Range High

    // Chorus Send Depth/Reverb Send Depth/Tone Modify 1-8
    for (size_t i = 0x00; i < 0x0A; i++)
        Part[0x0E + i] = srcData[0x21 + i];

    Part[0x18] = 0x00;
    Part[0x19] = 0x00;

    // Scale Tuning C to B
    for (size_t i = 0; i < 12; ++i)
        Part[0x1A + i] = srcData[0x2B + i];

    Part[0x26] = srcData[0x1F]; // CC1 Controller Number
    Part[0x27] = srcData[0x20]; // CC2 Controller Number

    // Destination Controllers
    for (size_t i = 0; i < 6; ++i) // Each controller
    {
        uint8_t srcPos = (uint8_t)(0x37 + ((uint8_t)i * 0x0B));
        uint8_t dstPos = (uint8_t)(0x28 + ((uint8_t)i * 0x0C));

        for (uint8_t j = 0; j < 3; ++j)
            Part[dstPos + 0x00 + j] = srcData[srcPos + 0x00 + j];

        Part[dstPos + 0x03] = (uint8_t)((i == 2 || i == 3) ? 0x40 : 0x00); // Verified with Recomposer 3.0 PC-98

        for (uint8_t j = 0; j < 8; ++j)
            Part[dstPos + 0x04 + j] = srcData[srcPos + 0x03 + j];
    }

    ConvertBytes2Nibbles(Part, sizeof(Part), dstData);
}

/// <summary>
///
/// </summary>
static void ConvertBytes2Nibbles(const uint8_t *srcData, size_t srcSize, uint8_t *dstData) {
    for (size_t i = 0; i < srcSize; ++i) {
        dstData[i * 2 + 0] = (uint8_t)((srcData[i] >> 4) & 0x0F);
        dstData[i * 2 + 1] = (uint8_t)((srcData[i] >> 0) & 0x0F);
    }
}
