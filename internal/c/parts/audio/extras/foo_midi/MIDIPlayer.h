
/** $VER: MIDIPlayer.h (2024.05.11) **/

#pragma once

#include "../libmidi/MIDIContainer.h"

typedef float audio_sample;

enum class MIDIFlavor { None = 0, GM, GM2, SC55, SC88, SC88Pro, SC8850, XG };

enum class LoopType {
    NeverLoop = 0,             // Never loop
    NeverLoopAddDecayTime = 1, // Never loop, add configured decay time at the end

    LoopAndFadeWhenDetected = 2, // Loop and fade when detected
    LoopAndFadeAlways = 3,       // Loop and fade always

    PlayIndefinitelyWhenDetected = 4, // Play indefinitely when detected
    PlayIndefinitely = 5,             // Play indefinitely
};

class MIDIPlayer {
  public:
    MIDIPlayer();
    virtual ~MIDIPlayer() {};

    enum LoopMode { None = 0x00, Enabled = 0x01, Forced = 0x02 };

    bool Load(const midi_container_t &midiContainer, uint32_t subsongIndex, LoopType loopMode, uint32_t cleanFlags);
    uint32_t Play(audio_sample *samples, uint32_t samplesSize) noexcept;
    void Seek(uint32_t seekTime);

    void SetSampleRate(uint32_t sampleRate);

    void Configure(MIDIFlavor midiFlavor, bool filterEffects);

    uint32_t GetPosition() const noexcept {
        return uint32_t((uint64_t(_Position) * 1000ul) / uint64_t(_SampleRate));
    }

    uint32_t GetFramePosition() const noexcept {
        return _Position;
    }

    // Should return the block size that the player expects, otherwise 0.
    virtual uint32_t GetSampleBlockSize() const noexcept {
        return 0;
    }

    virtual uint32_t GetActiveVoiceCount() const {
        return 0;
    }

    virtual bool GetErrorMessage(std::string &) {
        return false;
    }

  protected:
    virtual bool Startup() {
        return false;
    }

    virtual void Shutdown() {};

    virtual void Render(audio_sample *, uint32_t) {}

    virtual bool Reset() {
        return false;
    }

    virtual void SendEvent(uint32_t) {}

    virtual void SendSysEx(const uint8_t *, size_t, uint32_t) {};

    // Only implemented by Secret Sauce and VSTi-specific
    virtual void SendEvent(uint32_t, uint32_t){};
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t, uint32_t) {};

    void SendSysExReset(uint8_t portNumber, uint32_t time);

#ifdef _WIN32
    uint32_t GetProcessorArchitecture(const std::string &filePath) const;
#endif

  protected:
    bool _IsInitialized;
    uint32_t _SampleRate;
    sysex_table_t _SysExMap;

    MIDIFlavor _MIDIFlavor;
    bool _FilterEffects;

  private:
    void SendEventFiltered(uint32_t data);
    void SendEventFiltered(uint32_t data, uint32_t time);

    void SendSysExFiltered(const uint8_t *event, size_t size, uint8_t portNumber);
    void SendSysExFiltered(const uint8_t *event, size_t size, uint8_t portNumber, uint32_t time);

    void SendSysExSetToneMapNumber(uint8_t portNumber, uint32_t time);
    void SendSysExGS(uint8_t *data, size_t size, uint8_t portNumber, uint32_t time);

    static inline constexpr int32_t MulDiv(int32_t val, int32_t mul, int32_t div) {
        return int32_t((int64_t(val) * mul + (div >> 1)) / div);
    }

  private:
    static const uint32_t DecayTime = 1000;

    std::vector<midi_item_t> _Stream;
    size_t _StreamPosition; // Current position in the event stream

    uint32_t _Position;  // Current position in the sample stream
    uint32_t _Length;    // Total length of the sample stream
    uint32_t _Remainder; // In samples

    LoopType _LoopType;

    uint32_t _StreamLoopBegin;
    uint32_t _StreamLoopEnd;

    uint32_t _LoopBegin; // Position of the start of a loop in the sample stream
};
