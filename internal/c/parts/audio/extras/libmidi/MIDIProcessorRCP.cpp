
/** $VER: MIDIProcessorRCP.cpp (2024.05.15) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "framework.h"

#include "MIDIProcessor.h"
#include "Recomposer/RCP.h"

#include <filesystem>

/// <summary>
/// Returns true if data points to an RCP sequence.
/// </summary>
bool midi_processor_t::IsRCP(std::vector<uint8_t> const &data, const char *fileExtension) {
    if (fileExtension == nullptr)
        return false;

    if (data.size() < 28)
        return false;

    if (::strncmp((const char *)data.data(), "RCM-PC98V2.0(C)COME ON MUSIC", 28) == 0) {
        if (::strcasecmp(fileExtension, "rcp") == 0)
            return true;

        if (::strcasecmp(fileExtension, "r36") == 0)
            return true;

        return false;
    }

    if (data.size() < 31)
        return false;

    if (::strncmp((const char *)data.data(), "COME ON MUSIC RECOMPOSER RCP3.0", 31) == 0) {
        if (::strcasecmp(fileExtension, "g18") == 0)
            return true;

        if (::strcasecmp(fileExtension, "g36") == 0)
            return true;

        return false;
    }

    return false;
}

/// <summary>
/// Processes the sequence data.
/// </summary>
bool midi_processor_t::ProcessRCP(std::vector<uint8_t> const &data, const char *filePath, midi_container_t &container) {
    rcp_converter_t RCPConverter;

    RCPConverter.SetFilePath(filePath);

    rcp_converter_options_t &Options = RCPConverter._Options;

    Options._RCPLoopCount = _Options._LoopExpansion;

    Options._WriteBarMarkers = _Options._WriteBarMarkers;
    Options._WriteSysExNames = _Options._WriteSysExNames;
    Options._ExtendLoops = _Options._ExtendLoops;
    Options._WolfteamLoopMode = _Options._WolfteamLoopMode;
    Options._KeepDummyChannels = _Options._KeepDummyChannels;
    Options._IncludeControlData = _Options._IncludeControlData;

    buffer_t SrcData;

    SrcData.Copy(data.data(), data.size());

    buffer_t DstData;

    RCPConverter.Convert(SrcData, DstData);

    std::vector<uint8_t> Data;

    Data.insert(Data.end(), DstData.Data, DstData.Data + DstData.Size);

    midi_processor_t::Process(Data, filePath, container);

    return true;
}
