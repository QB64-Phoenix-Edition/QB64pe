
/** $VER: MIDIProcessor.h (2024.05.19) **/

#pragma once

#include "framework.h"

#include "Exception.h"
#include "IFF.h"
#include "MIDIContainer.h"

struct midi_processor_options_t {
    // RCP
    uint16_t _LoopExpansion = 0;
    bool _WriteBarMarkers = false;
    bool _WriteSysExNames = false;
    bool _ExtendLoops = true;
    bool _WolfteamLoopMode = false;
    bool _KeepDummyChannels = false;
    bool _IncludeControlData = true;

    // HMI / HMP
    uint16_t _DefaultTempo = 160; // in bpm

    // a740g: constructor here for the const DefaultOptions below
    midi_processor_options_t(uint16_t loopExpansion, bool writeBarMarkers, bool writeSysExNames)
        : _LoopExpansion(loopExpansion), _WriteBarMarkers(writeBarMarkers), _WriteSysExNames(writeSysExNames) {}

    midi_processor_options_t() = default;
};

const midi_processor_options_t DefaultOptions(0, false, 0); // a740g: not sure why this is needed

class midi_processor_t {
  public:
    static bool Process(std::vector<uint8_t> const &data, const char *filePath, midi_container_t &container,
                        const midi_processor_options_t &options = DefaultOptions);

  private:
    static bool IsSMF(std::vector<uint8_t> const &data);
    static bool IsRIFF(std::vector<uint8_t> const &data);
    static bool IsHMP(std::vector<uint8_t> const &data);
    static bool IsHMI(std::vector<uint8_t> const &data);
    static bool IsXMI(std::vector<uint8_t> const &data);
    static bool IsMUS(std::vector<uint8_t> const &data);
    static bool IsMDS(std::vector<uint8_t> const &data);
    static bool IsLDS(std::vector<uint8_t> const &data, const char *fileExtension);
    static bool IsGMF(std::vector<uint8_t> const &data);
    static bool IsRCP(std::vector<uint8_t> const &data, const char *fileExtension);
    static bool IsSysEx(std::vector<uint8_t> const &data);

    static bool ProcessSMF(std::vector<uint8_t> const &data, midi_container_t &container);
    static bool ProcessRIFF(std::vector<uint8_t> const &data, midi_container_t &container);
    static bool ProcessHMP(std::vector<uint8_t> const &data, midi_container_t &container);
    static bool ProcessHMI(std::vector<uint8_t> const &data, midi_container_t &container);
    static bool ProcessXMI(std::vector<uint8_t> const &data, midi_container_t &container);
    static bool ProcessMUS(std::vector<uint8_t> const &data, midi_container_t &container);
    static bool ProcessMDS(std::vector<uint8_t> const &data, midi_container_t &container);
    static bool ProcessLDS(std::vector<uint8_t> const &data, midi_container_t &container);
    static bool ProcessGMF(std::vector<uint8_t> const &data, midi_container_t &container);
    static bool ProcessRCP(std::vector<uint8_t> const &data, const char *filePath, midi_container_t &container);
    static bool ProcessSysEx(std::vector<uint8_t> const &data, midi_container_t &container);

    static bool ProcessSMFTrack(std::vector<uint8_t>::const_iterator &it, std::vector<uint8_t>::const_iterator end, midi_container_t &container,
                                bool needs_end_marker);
    static int DecodeVariableLengthQuantity(std::vector<uint8_t>::const_iterator &it, std::vector<uint8_t>::const_iterator end) noexcept;

    static uint32_t DecodeVariableLengthQuantityHMP(std::vector<uint8_t>::const_iterator &it, std::vector<uint8_t>::const_iterator end) noexcept;

    static bool ReadStream(std::vector<uint8_t> const &data, iff_stream_t &stream);
    static bool ReadChunk(std::vector<uint8_t>::const_iterator &it, std::vector<uint8_t>::const_iterator end, iff_chunk_t &chunk, bool isFirstChunk);
    static uint32_t DecodeVariableLengthQuantityXMI(std::vector<uint8_t>::const_iterator &it, std::vector<uint8_t>::const_iterator end) noexcept;

  private:
    static const uint8_t MIDIEventEndOfTrack[2];
    static const uint8_t LoopBeginMarker[11];
    static const uint8_t LoopEndMarker[9];

    static const uint8_t DefaultTempoXMI[5];

    static const uint8_t DefaultTempoMUS[5];
    static const uint8_t MusControllers[15];

    static const uint8_t DefaultTempoLDS[5];

    static midi_processor_options_t _Options;
};
