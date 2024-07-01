
/** $VER: CM6File.cpp (2024.05.09) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "RCP.h"

/// <summary>
///
/// </summary>
void cm6_file_t::Read(const buffer_t &data) {
    uint8_t FileType = rcp_converter_t::GetFileType(data);

    if (FileType != 0x10)
        throw std::runtime_error("Invalid CM6 control file.");

    if (data.Size < 0x5849)
        throw std::runtime_error("Insufficient data.");

    _Data = data;

    DeviceType = _Data.Data[0x001A];

    Comment.Assign(&_Data.Data[0x0040], 0x40);

    laSystem = &_Data.Data[0x0080];
    laChnVol = &_Data.Data[0x0097];
    laPatchTemp = &_Data.Data[0x00A0];
    laRhythmTemp = &_Data.Data[0x0130];
    laTimbreTemp = &_Data.Data[0x0284];
    laPatchMem = &_Data.Data[0x0A34];
    laTimbreMem = &_Data.Data[0x0E34];
    pcmPatchTemp = &_Data.Data[0x4E34];
    pcmPatchMem = &_Data.Data[0x4EB2];
    pcmSystem = &_Data.Data[0x5832];
    pcmChnVol = &_Data.Data[0x5843];
}

/// <summary>
/// Converts a CM6 file to a MIDI file.
/// </summary>
void rcp_converter_t::Convert(const cm6_file_t &cm6File, midi_stream_t &midiStream, uint8_t mode) {
    if (mode & 0x01)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, cm6File.Comment.Data, cm6File.Comment.Len);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "MT-32 System");

    midiStream.WriteRolandSysEx(SysExHeaderMT32, 0x100000, cm6File.laSystem, 0x17, SYXOPT_DELAY);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "MT-32 Patch Temporary");

    midiStream.WriteRolandSysEx(SysExHeaderMT32, 0x030000, cm6File.laPatchTemp, 0x90, SYXOPT_DELAY);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "MT-32 Rhythm Setup");

    midiStream.WriteRolandSysEx(SysExHeaderMT32, 0x030110, cm6File.laRhythmTemp, 0x154, SYXOPT_DELAY, 0x100);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "MT-32 Timbre Temporary");

    midiStream.WriteRolandSysEx(SysExHeaderMT32, 0x040000, cm6File.laTimbreTemp, 0x7B0, SYXOPT_DELAY, 0x100);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "MT-32 Patch Memory");

    midiStream.WriteRolandSysEx(SysExHeaderMT32, 0x050000, cm6File.laPatchMem, 0x400, SYXOPT_DELAY, 0x100);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "MT-32 Timbre Memory");

    midiStream.WriteRolandSysEx(SysExHeaderMT32, 0x080000, cm6File.laTimbreMem, 0x4000, SYXOPT_DELAY, 0x100);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "CM-32P Patch Temporary");

    midiStream.WriteRolandSysEx(SysExHeaderMT32, 0x500000, cm6File.pcmPatchTemp, 0x7E, SYXOPT_DELAY);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "CM-32P Patch Memory");

    midiStream.WriteRolandSysEx(SysExHeaderMT32, 0x510000, cm6File.pcmPatchMem, 0x980, SYXOPT_DELAY, 0x100);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "CM-32P System");

    midiStream.WriteRolandSysEx(SysExHeaderMT32, 0x520000, cm6File.pcmSystem, 0x11, SYXOPT_DELAY);

    if (mode & 0x10)
        midiStream.WriteMetaEvent(MetaDataTypes::Text, "Setup Finished.");
}
