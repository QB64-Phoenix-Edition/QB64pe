//----------------------------------------------------------------------------------------------------------------------
// ymfmidi: OPL3 MIDI player using the ymfm emulation core (https://github.com/devinacker/ymfmidi)
// Copyright (c) 2021-2022, Devin Acker
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "../radv2/opal.h"
#include "patches.h"
#include <climits>
#include <queue>
#include <vector>

struct MIDIChannel {
    uint8_t num = 0;

    bool percussion = false;
    uint8_t bank = 0;
    uint8_t patchNum = 0;
    uint8_t volume = 127;
    uint8_t pan = 64;
    double basePitch = 0.0; // pitch wheel position
    double pitch = 1.0;     // frequency multiplier

    uint16_t rpn = 0x3fff;

    uint8_t bendRange = 2;
};

struct OPLVoice {
    int chip = 0;
    const MIDIChannel *channel = nullptr;
    const OPLPatch *patch = nullptr;
    const PatchVoice *patchVoice = nullptr;

    uint16_t num = 0;
    uint16_t op = 0; // base operator number, set based on voice num.
    bool fourOpPrimary = false;
    OPLVoice *fourOpOther = nullptr;

    bool on = false;
    bool justChanged = false; // true after note on/off, false after generating at least 1 sample
    uint8_t note = 0;
    uint8_t velocity = 0;

    // block and F number, calculated from note and channel pitch
    uint16_t freq = 0;

    // how long has this note been playing (incremented each midi update)
    uint32_t duration = UINT_MAX;
};

class OPLPlayer {
  public:
    enum MIDIType { GeneralMIDI, RolandGS, YamahaXG, GeneralMIDI2 };

    OPLPlayer(int numChips = 1, int frequency = 44100);
    virtual ~OPLPlayer();

    // load instrument patches from the specified path
    bool loadPatches(const char *path);
    // load instrument patches from an already opened file,
    // optionally at a given offset
    // if 'size' is 0, the full file will be read (starting from 'offset')
    bool loadPatches(FILE *file, int offset = 0, size_t size = 0);
    // load instrument patches from a block of memory
    bool loadPatches(const uint8_t *data, size_t size);

    // render the audio output during playback.
    // note: regardless of sound settings, output stream is always stereo (two floats or int16s per sample)
    void generate(float *data, unsigned numSamples);
    void generate(int16_t *data, unsigned numSamples);

    // reset OPL and midi file
    void reset();

    // helpers for midiEvent
    void midiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void midiNoteOff(uint8_t channel, uint8_t note);
    void midiPitchControl(uint8_t channel, double pitch); // range is -1.0 to 1.0
    void midiProgramChange(uint8_t channel, uint8_t patchNum);
    void midiControlChange(uint8_t channel, uint8_t control, uint8_t value);
    // sysex data (data and length *don't* include the opening 0xF0)
    void midiSysEx(const uint8_t *data, uint32_t length);

    // helper for pitch bend and finetune
    static double midiCalcBend(double semitones);

    // misc. informational stuff
    int sampleRate() const {
        return m_sampleRate;
    }

    const std::string &patchName(uint8_t num) {
        return m_patches[num].name;
    }

    int activeVoiceCount() const;

  private:
    static constexpr unsigned masterClock = 14318181;

    enum {
        REG_TEST = 0x01,

        REG_OP_MODE = 0x20,
        REG_OP_LEVEL = 0x40,
        REG_OP_AD = 0x60,
        REG_OP_SR = 0x80,
        REG_VOICE_FREQL = 0xA0,
        REG_VOICE_FREQH = 0xB0,
        REG_VOICE_CNT = 0xC0,
        REG_OP_WAVEFORM = 0xE0,

        REG_4OP = 0x104,
        REG_NEW = 0x105,
    };

    void updateMIDI();

    void runSamples(int chip, unsigned count);

    void write(int chip, uint16_t addr, uint8_t data);

    // find a voice with the oldest note, or the same patch & note
    // if no "off" voices are found, steal one using the same patch or MIDI channel
    OPLVoice *findVoice(uint8_t channel, const OPLPatch *patch, uint8_t note);
    // find a voice that's playing a specific note on a specific channel
    OPLVoice *findVoice(uint8_t channel, uint8_t note, bool justChanged = false);

    // find the patch to use for a specific MIDI channel and note
    const OPLPatch *findPatch(uint8_t channel, uint8_t note) const;

    // determine whether this patch should be configured as 4op
    bool useFourOp(const OPLPatch *patch) const;

    // determine which operator(s) to scale based on the current operator settings
    std::pair<bool, bool> activeCarriers(const OPLVoice &voice) const;

    // update a property of all currently playing voices on a MIDI channel
    // (or all channels if `channel` < 0)
    void updateChannelVoices(int8_t channel, void (OPLPlayer::*func)(OPLVoice &));

    // update the patch parameters for a voice
    void updatePatch(OPLVoice &voice, const OPLPatch *newPatch, uint8_t numVoice = 0);

    // update the volume level for a voice
    void updateVolume(OPLVoice &voice);

    // update the pan position for a voice
    void updatePanning(OPLVoice &voice);

    // update the block and F-number for a voice (also key on/off)
    void updateFrequency(OPLVoice &voice);

    // silence a voice immediately
    void silenceVoice(OPLVoice &voice);

    std::vector<Opal *> m_opl3;
    unsigned m_numChips;

    int m_sampleRate; // output sample rate (default 44.1k)

    std::pair<int16_t, int16_t> m_output; // output sample data
    // if we need to clock one of the OPLs between register writes, save the resulting sample
    std::vector<std::queue<std::pair<int16_t, int16_t>>> m_sampleFIFO;

    MIDIChannel m_channels[16];
    std::vector<OPLVoice> m_voices;
    MIDIType m_midiType;

    OPLPatchSet m_patches;

    static const unsigned voice_num[18];
    static const unsigned oper_num[18];
};
