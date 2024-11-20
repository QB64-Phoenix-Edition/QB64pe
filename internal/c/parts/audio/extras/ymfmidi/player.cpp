//----------------------------------------------------------------------------------------------------------------------
// ymfmidi: OPL3 MIDI player using the ymfm emulation core (https://github.com/devinacker/ymfmidi)
// Copyright (c) 2021-2022, Devin Acker
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#include "player.h"
#include <algorithm>
#include <cmath>
#include <cstring>

const unsigned OPLPlayer::voice_num[18] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108};

const unsigned OPLPlayer::oper_num[18] = {0x0, 0x1, 0x2, 0x8, 0x9, 0xA, 0x10, 0x11, 0x12, 0x100, 0x101, 0x102, 0x108, 0x109, 0x10A, 0x110, 0x111, 0x112};

// ----------------------------------------------------------------------------
OPLPlayer::OPLPlayer(int numChips, int frequency) {
    m_numChips = numChips;
    m_voices.resize(numChips * 18);
    m_sampleFIFO.resize(m_numChips);

    m_sampleRate = frequency;

    reset();
}

// ----------------------------------------------------------------------------
OPLPlayer::~OPLPlayer() {
    for (auto &opl : m_opl3)
        delete opl;
}

// ----------------------------------------------------------------------------
bool OPLPlayer::loadPatches(const char *path) {
    return OPLPatch::load(m_patches, path);
}

// ----------------------------------------------------------------------------
bool OPLPlayer::loadPatches(FILE *file, int offset, size_t size) {
    return OPLPatch::load(m_patches, file, offset, size);
}

// ----------------------------------------------------------------------------
bool OPLPlayer::loadPatches(const uint8_t *data, size_t size) {
    return OPLPatch::load(m_patches, data, size);
}

// ----------------------------------------------------------------------------
void OPLPlayer::generate(float *data, unsigned numSamples) {
    static constexpr float normalization_factor = 1.0f / 32768.0f;

    unsigned samp = 0;

    while (samp < numSamples * 2) {
        updateMIDI();

        data[samp] = (float)m_output.first * normalization_factor;
        data[samp + 1] = (float)m_output.second * normalization_factor;

        samp += 2;
    }
}

// ----------------------------------------------------------------------------
void OPLPlayer::generate(int16_t *data, unsigned numSamples) {
    unsigned samp = 0;

    while (samp < numSamples * 2) {
        updateMIDI();

        data[samp] = m_output.first;
        data[samp + 1] = m_output.second;

        samp += 2;
    }
}

// ----------------------------------------------------------------------------
void OPLPlayer::updateMIDI() {
    for (auto &voice : m_voices) {
        if (voice.duration < UINT_MAX)
            voice.duration++;
        voice.justChanged = false;
    }

    std::pair<int16_t, int16_t> output; // output per chip
    std::pair<int, int> sample;         // final mixed samples

    for (unsigned i = 0; i < m_numChips; i++) {
        if (m_sampleFIFO[i].empty()) {
            m_opl3[i]->Sample(&output.first, &output.second);
        } else {
            output = m_sampleFIFO[i].front();
            m_sampleFIFO[i].pop();
        }

        // Mix output from all chips
        sample.first += output.first;
        sample.second += output.second;
    }

    // Clamp and update
    m_output.first = std::clamp(sample.first, -32768, 32767);
    m_output.second = std::clamp(sample.second, -32768, 32767);
}

// ----------------------------------------------------------------------------
void OPLPlayer::reset() {
    for (auto &opl : m_opl3)
        delete opl;

    m_opl3.resize(m_numChips);
    for (auto &opl : m_opl3) {
        opl = new Opal(m_sampleRate);
        opl->Port(REG_NEW, 1);
    }

    // reset MIDI channel and OPL voice status
    m_midiType = GeneralMIDI;
    for (int i = 0; i < 16; i++) {
        m_channels[i] = MIDIChannel();
        m_channels[i].num = i;
    }
    m_channels[9].percussion = true;

    for (int i = 0; i < m_voices.size(); i++) {
        m_voices[i] = OPLVoice();
        m_voices[i].chip = i / 18;
        m_voices[i].num = voice_num[i % 18];
        m_voices[i].op = oper_num[i % 18];

        // configure 4op voices
        switch (i % 9) {
        case 0:
        case 1:
        case 2:
            m_voices[i].fourOpPrimary = true;
            m_voices[i].fourOpOther = &m_voices[i + 3];
            break;
        case 3:
        case 4:
        case 5:
            m_voices[i].fourOpPrimary = false;
            m_voices[i].fourOpOther = &m_voices[i - 3];
            break;
        default:
            m_voices[i].fourOpPrimary = false;
            m_voices[i].fourOpOther = nullptr;
            break;
        }
    }
}

// ----------------------------------------------------------------------------
void OPLPlayer::runSamples(int chip, unsigned count) {
    // add some delay between register writes where needed
    // (i.e. when forcing a voice off, changing 4op flags, etc.)
    while (count--) {
        std::pair<int16_t, int16_t> output;
        m_opl3[chip]->Sample(&output.first, &output.second);
        m_sampleFIFO[chip].push(output);
    }
}

// ----------------------------------------------------------------------------
void OPLPlayer::write(int chip, uint16_t addr, uint8_t data) {
    m_opl3[chip]->Port(addr, data);
}

// ----------------------------------------------------------------------------
OPLVoice *OPLPlayer::findVoice(uint8_t channel, const OPLPatch *patch, uint8_t note) {
    OPLVoice *found = nullptr;
    uint32_t duration = 0;

    // try to find the "oldest" voice, prioritizing released notes
    // (or voices that haven't ever been used yet)
    for (auto &voice : m_voices) {
        if (useFourOp(patch) && !voice.fourOpPrimary)
            continue;

        if (!voice.channel)
            return &voice;

        if (!voice.on && !voice.justChanged) {
            if (voice.channel->num == channel && voice.note == note && voice.duration < UINT_MAX) {
                // found an old voice that was using the same note and patch
                // don't immediately use it, but make it a high priority candidate for later
                // (to help avoid pop/click artifacts when retriggering a recently off note)
                silenceVoice(voice);
                if (useFourOp(voice.patch) && voice.fourOpOther)
                    silenceVoice(*voice.fourOpOther);
            } else if (voice.duration > duration) {
                found = &voice;
                duration = voice.duration;
            }
        }
    }

    if (found)
        return found;
    // if we didn't find one yet, just try to find an old one
    // using the same patch, even if it should still be playing.
    for (auto &voice : m_voices) {
        if (useFourOp(patch) && !voice.fourOpPrimary)
            continue;

        if (voice.patch == patch && voice.duration > duration) {
            found = &voice;
            duration = voice.duration;
        }
    }

    if (found)
        return found;
    // last resort - just find any old voice at all

    for (auto &voice : m_voices) {
        if (useFourOp(patch) && !voice.fourOpPrimary)
            continue;
        // don't let a 2op instrument steal an active voice from a 4op one
        if (!useFourOp(patch) && voice.on && useFourOp(voice.patch))
            continue;

        if (voice.duration > duration) {
            found = &voice;
            duration = voice.duration;
        }
    }

    return found;
}

// ----------------------------------------------------------------------------
OPLVoice *OPLPlayer::findVoice(uint8_t channel, uint8_t note, bool justChanged) {
    channel &= 15;
    for (auto &voice : m_voices) {
        if (voice.on && voice.justChanged == justChanged && voice.channel == &m_channels[channel] && voice.note == note) {
            return &voice;
        }
    }

    return nullptr;
}

// ----------------------------------------------------------------------------
const OPLPatch *OPLPlayer::findPatch(uint8_t channel, uint8_t note) const {
    uint16_t key;
    const MIDIChannel &ch = m_channels[channel & 15];

    if (ch.percussion)
        key = 0x80 | note | (ch.patchNum << 8);
    else
        key = ch.patchNum | (ch.bank << 8);

    // if this patch+bank combo doesn't exist, default to bank 0
    if (!m_patches.count(key))
        key &= 0x00ff;
    // if patch still doesn't exist in bank 0, use patch 0 (or drum note 0)
    if (!m_patches.count(key))
        key &= 0x0080;
    // if that somehow still doesn't exist, forget it
    if (!m_patches.count(key))
        return nullptr;

    return &m_patches.at(key);
}

// ----------------------------------------------------------------------------
bool OPLPlayer::useFourOp(const OPLPatch *patch) const {
    return patch->fourOp;
}

// ----------------------------------------------------------------------------
std::pair<bool, bool> OPLPlayer::activeCarriers(const OPLVoice &voice) const {
    bool scale[2] = {0};
    const auto patchVoice = voice.patchVoice;

    if (!patchVoice) {
        scale[0] = scale[1] = false;
    } else if (!useFourOp(voice.patch)) {
        // 2op FM (0): scale op 2 only
        // 2op AM (1): scale op 1 and 2
        scale[0] = (patchVoice->conn & 1);
        scale[1] = true;
    } else if (voice.fourOpPrimary) {
        // 4op FM+FM (0, 0): don't scale op 1 or 2
        // 4op AM+FM (1, 0): scale op 1 only
        // 4op FM+AM (0, 1): scale op 2 only
        // 4op AM+AM (1, 1): scale op 1 only
        scale[0] = (voice.patch->voice[0].conn & 1);
        scale[1] = (voice.patch->voice[1].conn & 1) && !scale[0];
    } else {
        // 4op FM+FM (0, 0): scale op 4 only
        // 4op AM+FM (1, 0): scale op 4 only
        // 4op FM+AM (0, 1): scale op 4 only
        // 4op AM+AM (1, 1): scale op 3 and 4
        scale[0] = (voice.patch->voice[0].conn & 1) && (voice.patch->voice[1].conn & 1);
        scale[1] = true;
    }

    return std::make_pair(scale[0], scale[1]);
}

// ----------------------------------------------------------------------------
void OPLPlayer::updateChannelVoices(int8_t channel, void (OPLPlayer::*func)(OPLVoice &)) {
    for (auto &voice : m_voices) {
        if ((channel < 0) || (voice.channel == &m_channels[channel & 15]))
            (this->*func)(voice);
    }
}

// ----------------------------------------------------------------------------
void OPLPlayer::updatePatch(OPLVoice &voice, const OPLPatch *newPatch, uint8_t numVoice) {
    // assign the MIDI channel's current patch (or the current drum patch) to this voice

    const PatchVoice &patchVoice = newPatch->voice[numVoice];

    if (voice.patchVoice != &patchVoice) {
        bool oldFourOp = voice.patch ? useFourOp(voice.patch) : false;

        voice.patch = newPatch;
        voice.patchVoice = &patchVoice;

        // update enable status for 4op channels on this chip
        if (useFourOp(newPatch) != oldFourOp) {
            // if going from part of a 4op patch to a 2op one, kill the other one
            OPLVoice *other = voice.fourOpOther;
            if (other && other->patch && useFourOp(other->patch) && !useFourOp(newPatch)) {
                silenceVoice(*other);
            }

            uint8_t enable = 0x00;
            uint8_t bit = 0x01;
            for (unsigned i = voice.chip * 18; i < voice.chip * 18 + 18; i++) {
                if (m_voices[i].fourOpPrimary) {
                    if (m_voices[i].patch && useFourOp(m_voices[i].patch))
                        enable |= bit;
                    bit <<= 1;
                }
            }

            write(voice.chip, REG_4OP, enable);
            //	runSamples(voice.chip, 1);
        }

        // kill an existing voice, then send the chip far enough forward in time to let the envelope die off
        // (ROTT: fixes nasty reverse cymbal noises in spray.mid
        //        without disrupting note timing too much for the staccato drums in fanfare2.mid)
        silenceVoice(voice);
        runSamples(voice.chip, 48);

        // 0x20: vibrato, sustain, multiplier
        write(voice.chip, REG_OP_MODE + voice.op, patchVoice.op_mode[0]);
        write(voice.chip, REG_OP_MODE + voice.op + 3, patchVoice.op_mode[1]);
        // 0x60: attack/decay
        write(voice.chip, REG_OP_AD + voice.op, patchVoice.op_ad[0]);
        write(voice.chip, REG_OP_AD + voice.op + 3, patchVoice.op_ad[1]);
        // 0xe0: waveform
        write(voice.chip, REG_OP_WAVEFORM + voice.op, patchVoice.op_wave[0]);
        write(voice.chip, REG_OP_WAVEFORM + voice.op + 3, patchVoice.op_wave[1]);
    }

    // 0x80: sustain/release
    // update even for the same patch in case silenceVoice was called from somewhere else on this voice
    write(voice.chip, REG_OP_SR + voice.op, patchVoice.op_sr[0]);
    write(voice.chip, REG_OP_SR + voice.op + 3, patchVoice.op_sr[1]);
}

// ----------------------------------------------------------------------------
void OPLPlayer::updateVolume(OPLVoice &voice) {
    // lookup table shamelessly stolen from Nuke.YKT
    static constexpr uint8_t opl_volume_map[32] = {80, 63, 40, 36, 32, 28, 23, 21, 19, 17, 15, 14, 13, 12, 11, 10,
                                                   9,  8,  7,  6,  5,  5,  4,  4,  3,  3,  2,  2,  1,  1,  0,  0};

    if (!voice.patch || !voice.channel)
        return;

    uint8_t atten = opl_volume_map[(voice.velocity * voice.channel->volume) >> 9];
    uint8_t level;

    const auto patchVoice = voice.patchVoice;
    const auto scale = activeCarriers(voice);

    // 0x40: key scale / volume
    if (scale.first)
        level = std::min(0x3f, patchVoice->op_level[0] + atten);
    else
        level = patchVoice->op_level[0];
    write(voice.chip, REG_OP_LEVEL + voice.op, level | patchVoice->op_ksr[0]);

    if (scale.second)
        level = std::min(0x3f, patchVoice->op_level[1] + atten);
    else
        level = patchVoice->op_level[1];
    write(voice.chip, REG_OP_LEVEL + voice.op + 3, level | patchVoice->op_ksr[1]);
}

// ----------------------------------------------------------------------------
void OPLPlayer::updatePanning(OPLVoice &voice) {
    if (!voice.patch || !voice.channel)
        return;

    // 0xc0: output/feedback/mode
    uint8_t pan = 0x30;
    if (voice.channel->pan < 32)
        pan = 0x10;
    else if (voice.channel->pan >= 96)
        pan = 0x20;

    write(voice.chip, REG_VOICE_CNT + voice.num, voice.patchVoice->conn | pan);
}

// ----------------------------------------------------------------------------
void OPLPlayer::updateFrequency(OPLVoice &voice) {
    static const uint16_t noteFreq[12] = {// calculated from A440
                                          345, 365, 387, 410, 435, 460, 488, 517, 547, 580, 615, 651};

    if (!voice.patch || !voice.channel)
        return;
    if (useFourOp(voice.patch) && !voice.fourOpPrimary)
        return;

    int note = (!voice.channel->percussion ? voice.note : voice.patch->fixedNote) + voice.patchVoice->tune;

    int octave = note / 12;
    note %= 12;

    // calculate base frequency (and apply pitch bend / patch detune)
    unsigned freq = (note >= 0) ? noteFreq[note] : (noteFreq[note + 12] >> 1);
    if (octave < 0)
        freq >>= -octave;
    else if (octave > 0)
        freq <<= octave;

    freq *= voice.channel->pitch * voice.patchVoice->finetune;

    // convert the calculated frequency back to a block and F-number
    octave = 0;
    while (freq > 0x3ff) {
        freq >>= 1;
        octave++;
    }
    octave = std::min(7, octave);
    voice.freq = freq | (octave << 10);

    write(voice.chip, REG_VOICE_FREQL + voice.num, voice.freq & 0xff);
    write(voice.chip, REG_VOICE_FREQH + voice.num, (voice.freq >> 8) | (voice.on ? (1 << 5) : 0));
}

// ----------------------------------------------------------------------------
void OPLPlayer::silenceVoice(OPLVoice &voice) {
    voice.on = false;
    voice.justChanged = true;
    voice.duration = UINT_MAX;

    write(voice.chip, REG_OP_SR + voice.op, 0xff);
    write(voice.chip, REG_OP_SR + voice.op + 3, 0xff);
    write(voice.chip, REG_VOICE_FREQH + voice.num, voice.freq >> 8);
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    note &= 0x7f;
    velocity &= 0x7f;

    // if we just now turned this same note on, don't do it again
    if (findVoice(channel, note, true))
        return;

    if (!velocity)
        return midiNoteOff(channel, note);

    //	printf("midiNoteOn: chn %u, note %u\n", channel, note);
    const OPLPatch *newPatch = findPatch(channel, note);
    if (!newPatch)
        return;

    const int numVoices = ((useFourOp(newPatch) || newPatch->dualTwoOp) ? 2 : 1);

    OPLVoice *voice = nullptr;
    for (int i = 0; i < numVoices; i++) {
        if (voice && useFourOp(newPatch) && voice->fourOpOther)
            voice = voice->fourOpOther;
        else
            voice = findVoice(channel, newPatch, note);
        if (!voice)
            continue; // ??

        updatePatch(*voice, newPatch, i);

        // update the note parameters for this voice
        voice->channel = &m_channels[channel & 15];
        voice->on = voice->justChanged = true;
        voice->note = note;
        voice->velocity = std::clamp((int)velocity + newPatch->velocity, 0, 127);
        voice->duration = 0;

        updateVolume(*voice);
        updatePanning(*voice);

        // for 4op instruments, don't key on until we've written both voices...
        if (!useFourOp(newPatch)) {
            updateFrequency(*voice);
        } else if (i > 0) {
            updateFrequency(*voice->fourOpOther);
        }
    }
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiNoteOff(uint8_t channel, uint8_t note) {
    note &= 0x7f;

    //	printf("midiNoteOff: chn %u, note %u\n", channel, note);
    OPLVoice *voice;
    while ((voice = findVoice(channel, note)) != nullptr) {
        voice->justChanged = voice->on;
        voice->on = false;

        write(voice->chip, REG_VOICE_FREQH + voice->num, voice->freq >> 8);
    }
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiPitchControl(uint8_t channel, double pitch) {
    //	printf("midiPitchControl: chn %u, val %.02f\n", channel, pitch);
    MIDIChannel &ch = m_channels[channel & 15];

    ch.basePitch = pitch;
    ch.pitch = midiCalcBend(pitch * ch.bendRange);
    updateChannelVoices(channel, &OPLPlayer::updateFrequency);
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiProgramChange(uint8_t channel, uint8_t patchNum) {
    m_channels[channel & 15].patchNum = patchNum & 0x7f;
    // patch change will take effect on the next note for this channel
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    channel &= 15;
    control &= 0x7f;
    value &= 0x7f;

    MIDIChannel &ch = m_channels[channel];

    //	printf("midiControlChange: chn %u, ctrl %u, val %u\n", channel, control, value);
    switch (control) {
    case 0:
        if (m_midiType == RolandGS)
            ch.bank = value;
        else if (m_midiType == YamahaXG)
            ch.percussion = (value == 0x7f);
        break;

    case 6:
        if (ch.rpn == 0) {
            ch.bendRange = value;
            midiPitchControl(channel, ch.basePitch);
        }
        break;

    case 7:
        ch.volume = value;
        updateChannelVoices(channel, &OPLPlayer::updateVolume);
        break;

    case 10:
        ch.pan = value;
        updateChannelVoices(channel, &OPLPlayer::updatePanning);
        break;

    case 32:
        if (m_midiType == YamahaXG || m_midiType == GeneralMIDI2)
            ch.bank = value;
        break;

    case 98:
    case 99:
        ch.rpn = 0x3fff;
        break;

    case 100:
        ch.rpn &= 0x3f80;
        ch.rpn |= value;
        break;

    case 101:
        ch.rpn &= 0x7f;
        ch.rpn |= (value << 7);
        break;
    }
}

// ----------------------------------------------------------------------------
void OPLPlayer::midiSysEx(const uint8_t *data, uint32_t length) {
    if (length > 0 && data[0] == 0xF0) {
        data++;
        length--;
    }

    if (length == 0)
        return;

    if (data[0] == 0x7e) // universal non-realtime
    {
        if (length == 5 && data[1] == 0x7f && data[2] == 0x09) {
            if (data[3] == 0x01)
                m_midiType = GeneralMIDI;
            else if (data[3] == 0x03)
                m_midiType = GeneralMIDI2;
        }
    } else if (data[0] == 0x41 && length >= 10 // Roland
               && data[2] == 0x42 && data[3] == 0x12) {
        // if we received one of these, assume GS mode
        // (some MIDIs seem to e.g. send drum map messages without a GS reset)
        m_midiType = RolandGS;

        uint32_t address = (data[4] << 16) | (data[5] << 8) | data[6];
        // for single part parameters, map "part number" to channel number
        // (using the default mapping)
        uint8_t channel = (address & 0xf00) >> 8;
        if (channel == 0)
            channel = 9;
        else if (channel <= 9)
            channel--;

        // Roland GS part parameters
        if ((address & 0xfff0ff) == 0x401015) // set drum map
            m_channels[channel].percussion = (data[7] != 0x00);
    } else if (length >= 8 && !memcmp(data, "\x43\x10\x4c\x00\x00\x7e\x00\xf7", 8)) // Yamaha
    {
        m_midiType = YamahaXG;
    }
}

// ----------------------------------------------------------------------------
double OPLPlayer::midiCalcBend(double semitones) {
    return pow(2, semitones / 12.0);
}

// ----------------------------------------------------------------------------
int OPLPlayer::activeVoiceCount() const {
    int totalVoices = 0;

    for (auto &voice : m_voices) {
        if (voice.channel && (voice.on || voice.justChanged))
            totalVoices++;
    }

    return totalVoices;
}
