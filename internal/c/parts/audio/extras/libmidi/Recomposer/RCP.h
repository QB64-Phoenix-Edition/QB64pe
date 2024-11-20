
/** $VER: RCP.h (2024.05.15) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#pragma once

#include "../framework.h"

#include "MIDIStream.h"
#include "Support.h"

class rcp_converter_options_t {
  public:
    uint16_t _RCPLoopCount = 2;

    bool _WriteBarMarkers = false;
    bool _WriteSysExNames = false;
    bool _ExtendLoops = true;
    bool _WolfteamLoopMode = false;
    bool _KeepDummyChannels = false;
    bool _IncludeControlData = true;
};

class rcp_string_t {
  public:
    rcp_string_t() : Data(), Size(), Len() {}

    const char *Data;
    uint16_t Size;
    uint16_t Len; // Length after trimming

    uint32_t Assign(const uint8_t *data, uint16_t size) {
        Data = (const char *)data;
        Size = size;

        Len = GetTrimmedLength(Data, Size, ' ', false);

        return size;
    }

    uint32_t AssignSpecial(const uint8_t *data, uint16_t size) {
        Data = (const char *)data;
        Size = size;

        // special trimming used for file names
        const char *Tail = (const char *)::memchr(Data, '\0', Size);

        Len = (Tail != nullptr) ? (uint16_t)(Tail - Data) : Size; // Stop at first '\0'.

        Len = GetTrimmedLength(Data, Len, ' ', false); // Trim spaces.

        return size;
    }
};

class rcp_user_sysex_t {
  public:
    rcp_user_sysex_t() : Data(), Size() {}

    rcp_string_t Name;

    const uint8_t *Data;
    uint32_t Size;
};

class rcp_track_t {
  public:
    uint32_t Offs;
    uint32_t Size;
    uint32_t Duration; // In ticks

    uint32_t LoopStartOffs; // Offset of the start of the loop
    uint32_t LoopStartTick; // Tick of the start of the loop
    uint16_t LoopCount;     // Number of loops
};

class rcp_file_t {
  public:
    rcp_file_t(const rcp_converter_options_t &options) : _Options(options) {
        _Version = 0;
        _TrackCount = 0;
        _TicksPerQuarter = 0;
        _Tempo = 0;
        _BPMNumerator = 0;
        _BPMDenominator = 0;
        _KeySignature = 0;
        _GlobalTransposition = 0;
        _CommentSize = 0;
    }

    rcp_file_t(const rcp_file_t &) = delete;
    rcp_file_t(const rcp_file_t &&) = delete;
    rcp_file_t &operator=(const rcp_file_t &) = delete;
    rcp_file_t &operator=(rcp_file_t &&) = delete;

    void ReadTrack(const uint8_t *data, uint32_t size, uint32_t offset, rcp_track_t *track) const;
    void ConvertTrack(const uint8_t *data, uint32_t size, uint32_t *offset, rcp_track_t *track, midi_stream_t &midiStream) const;

  private:
    uint16_t GetMultiCmdDataSize(const uint8_t *data, uint32_t size, uint32_t offset, uint8_t flags) const;
    uint16_t ReadMultiCmdData(const uint8_t *srcData, uint32_t srcSize, uint32_t *srcOffset, uint8_t *dstData, uint32_t dstSize, uint8_t flags) const;

  public:
    uint8_t _Version;
    uint16_t _TrackCount;
    uint16_t _TicksPerQuarter;
    uint16_t _Tempo;
    uint8_t _BPMNumerator;
    uint8_t _BPMDenominator;
    uint8_t _KeySignature;
    int8_t _GlobalTransposition;
    rcp_string_t _Title;
    uint16_t _CommentSize;
    rcp_string_t _Comments;
    rcp_string_t _CM6FileName;
    rcp_string_t _GSD1FileName;
    rcp_string_t _GSD2FileName;
    rcp_user_sysex_t _SysEx[8];

  private:
    const rcp_converter_options_t &_Options;
};

class cm6_file_t {
  public:
    cm6_file_t() {
        DeviceType = 0;

        laSystem = nullptr;
        laChnVol = nullptr;
        laPatchTemp = nullptr;
        laRhythmTemp = nullptr;
        laTimbreTemp = nullptr;
        laPatchMem = nullptr;
        laTimbreMem = nullptr;

        pcmPatchTemp = nullptr;
        pcmPatchMem = nullptr;
        pcmSystem = nullptr;
        pcmChnVol = nullptr;
    }

    void Read(const buffer_t &cm6Data);

  public:
    uint8_t DeviceType; // 0 - MT-32, 3 - CM-64
    rcp_string_t Comment;

    // MT-32 (LA) data
    const uint8_t *laSystem;
    const uint8_t *laChnVol;
    const uint8_t *laPatchTemp;
    const uint8_t *laRhythmTemp;
    const uint8_t *laTimbreTemp;
    const uint8_t *laPatchMem;
    const uint8_t *laTimbreMem;

    // CM-32P (PCM) data
    const uint8_t *pcmPatchTemp;
    const uint8_t *pcmPatchMem;
    const uint8_t *pcmSystem;
    const uint8_t *pcmChnVol;

  private:
    buffer_t _Data;
};

class gsd_file_t {
  public:
    gsd_file_t() {
        sysParams = nullptr;
        reverbParams = nullptr;
        chorusParams = nullptr;
        partParams = nullptr;
        drumSetup = nullptr;
        masterTune = nullptr;
    }

    void Read(const buffer_t &gsdData);

  public:
    const uint8_t *sysParams;
    const uint8_t *reverbParams;
    const uint8_t *chorusParams;
    const uint8_t *partParams;
    const uint8_t *drumSetup;
    const uint8_t *masterTune;

  private:
    buffer_t _Data;
};

class rcp_converter_t {
  public:
    void SetFilePath(const char *filePath) noexcept {
        _FilePath = filePath;
    }

    void Convert(const buffer_t &srcData, buffer_t &dstData, const char *dstType = "mid");

    void ConvertSequence(const buffer_t &rcpData, buffer_t &midData);
    void ConvertControl(const buffer_t &rcpData, buffer_t &midData, uint8_t fileType, uint8_t outMode);

    void Convert(const cm6_file_t &cm6File, midi_stream_t &midiStream, uint8_t mode);
    void Convert(const gsd_file_t &gsdFile, midi_stream_t &midiStream, uint8_t mode);

    static uint8_t GetFileType(const buffer_t &rcpData) noexcept;

  private:
    static uint16_t BalanceTrackTimes(std::vector<rcp_track_t> &rcpTracks, uint32_t minLoopTicks, uint8_t verbose);
    static uint8_t HandleDuration(midi_stream_t *fileInfo, uint32_t &duration);

  public:
    rcp_converter_options_t _Options;

    std::string _FilePath;
};

const uint8_t SysExHeaderMT32[] = {0x41, 0x10, 0x16, 0x12};
const uint8_t SysExHeaderSC55[] = {0x41, 0x10, 0x42, 0x12};
const uint8_t MT32PatchChange[] = {0xFF, 0xFF, 0x18, 0x32, 0x0C, 0x00, 0x01};

void RCP2MIDITimeSignature(uint8_t numerator, uint8_t denominator, uint8_t buffer[4]);
void RCP2MIDIKeySignature(uint8_t keySignature, uint8_t buffer[2]);
