//----------------------------------------------------------------------------------------------------------------------
// ymfmidi: OPL3 MIDI player using the ymfm emulation core (https://github.com/devinacker/ymfmidi)
// Copyright (c) 2021-2022, Devin Acker
//
// Modified by a740g for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#include "patches.h"
#include "player.h"
#include <cstring>
#include <vector>

const char *OPLPatch::names[256] = {
    "Acoustic Grand Piano",
    "Bright Acoustic Piano",
    "Electric Grand Piano",
    "Honky-tonk Piano",
    "Electric Piano 1",
    "Electric Piano 2",
    "Harpsichord",
    "Clavi",
    "Celesta",
    "Glockenspiel",
    "Music Box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tubular Bells",
    "Dulcimer",
    "Drawbar Organ",
    "Percussive Organ",
    "Rock Organ",
    "Church Organ",
    "Reed Organ",
    "Accordion",
    "Harmonica",
    "Tango Accordion",
    "Acoustic Guitar (nylon)",
    "Acoustic Guitar (steel)",
    "Electric Guitar (jazz)",
    "Electric Guitar (clean)",
    "Electric Guitar (muted)",
    "Overdriven Guitar",
    "Distortion Guitar",
    "Guitar Harmonics",
    "Acoustic Bass",
    "Electric Bass (finger)",
    "Electric Bass (pick)",
    "Fretless Bass",
    "Slap Bass 1",
    "Slap Bass 2",
    "Synth Bass 1",
    "Synth Bass 2",
    "Violin",
    "Viola",
    "Cello",
    "Contrabass",
    "Tremolo Strings",
    "Pizzicato Strings",
    "Orchestral Harp",
    "Timpani",
    "String Ensemble 1",
    "String Ensemble 2",
    "SynthStrings 1",
    "SynthStrings 2",
    "Choir Aahs",
    "Voice Oohs",
    "Synth Voice",
    "Orchestra Hit",
    "Trumpet",
    "Trombone",
    "Tuba",
    "Muted Trumpet",
    "French Horn",
    "Brass Section",
    "SynthBrass 1",
    "SynthBrass 2",
    "Soprano Sax",
    "Alto Sax",
    "Tenor Sax",
    "Baritone Sax",
    "Oboe",
    "English Horn",
    "Bassoon",
    "Clarinet",
    "Piccolo",
    "Flute",
    "Recorder",
    "Pan Flute",
    "Blown Bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",
    "Lead 1 (square)",
    "Lead 2 (sawtooth)",
    "Lead 3 (calliope)",
    "Lead 4 (chiff)",
    "Lead 5 (charang)",
    "Lead 6 (voice)",
    "Lead 7 (fifths)",
    "Lead 8 (bass + lead)",
    "Pad 1 (new age)",
    "Pad 2 (warm)",
    "Pad 3 (polysynth)",
    "Pad 4 (choir)",
    "Pad 5 (bowed)",
    "Pad 6 (metallic)",
    "Pad 7 (halo)",
    "Pad 8 (sweep)",
    "FX 1 (rain)",
    "FX 2 (soundtrack)",
    "FX 3 (crystal)",
    "FX 4 (atmosphere)",
    "FX 5 (brightness)",
    "FX 6 (goblins)",
    "FX 7 (echoes)",
    "FX 8 (sci-fi)",
    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bagpipe",
    "Fiddle",
    "Shanai",
    "Tinkle Bell",
    "Agogo",
    "Steel Drums",
    "Woodblock",
    "Taiko Drum",
    "Melodic Tom",
    "Synth Drum",
    "Reverse Cymbal",
    "Guitar Fret Noise",
    "Breath Noise",
    "Seashore",
    "Bird Tweet",
    "Telephone Ring",
    "Helicopter",
    "Applause",
    "Gunshot",

    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Acoustic Bass Drum",
    "Bass Drum 1",
    "Side Stick",
    "Acoustic Snare",
    "Hand Clap",
    "Electric Snare",
    "Low Floor Tom",
    "Closed Hi Hat",
    "High Floor Tom",
    "Pedal Hi-Hat",
    "Low Tom",
    "Open Hi-Hat",
    "Low-Mid Tom",
    "Hi-Mid Tom",
    "Crash Cymbal 1",
    "High Tom",
    "Ride Cymbal 1",
    "Chinese Cymbal",
    "Ride Bell",
    "Tambourine",
    "Splash Cymbal",
    "Cowbell",
    "Crash Cymbal 2",
    "Vibraslap",
    "Ride Cymbal 2",
    "Hi Bongo",
    "Low Bongo",
    "Mute Hi Conga",
    "Open Hi Conga",
    "Low Conga",
    "High Timbale",
    "Low Timbale",
    "High Agogo",
    "Low Agogo",
    "Cabasa",
    "Maracas",
    "Short Whistle",
    "Long Whistle",
    "Short Guiro",
    "Long Guiro",
    "Claves",
    "Hi Wood Block",
    "Low Wood Block",
    "Mute Cuica",
    "Open Cuica",
    "Mute Triangle",
    "Open Triangle",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

// ----------------------------------------------------------------------------
bool OPLPatch::load(OPLPatchSet &patches, const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file)
        return false;

    bool ok = load(patches, file);

    fclose(file);

    return ok;
}

// ----------------------------------------------------------------------------
bool OPLPatch::load(OPLPatchSet &patches, FILE *file, int offset, size_t size) {
    if (!size) {
        fseek(file, 0, SEEK_END);
        if (ftell(file) < 0)
            return false;
        size = ftell(file) - offset;
    }

    fseek(file, offset, SEEK_SET);
    std::vector<uint8_t> data(size);
    if (fread(data.data(), 1, size, file) != size)
        return false;

    return load(patches, data.data(), size);
}

// ----------------------------------------------------------------------------
bool OPLPatch::load(OPLPatchSet &patches, const uint8_t *data, size_t size) {
    return loadWOPL(patches, data, size) || loadOP2(patches, data, size) || loadAIL(patches, data, size) || loadTMB(patches, data, size);
}

// ----------------------------------------------------------------------------
bool OPLPatch::loadWOPL(OPLPatchSet &patches, const uint8_t *data, size_t size) {
    if (size < 19)
        return false;
    if (strcmp((const char *)data, "WOPL3-BANK"))
        return false;

    // mixed endianness? why???
    uint16_t version = data[11] | (data[12] << 8);
    uint16_t numMelody = (data[13] << 8) | data[14];
    uint16_t numPerc = (data[15] << 8) | data[16];

    if (version > 3)
        return false;

    // currently not supported: global LFO flags, volume model options

    const uint32_t bankOffset = 19;
    const uint32_t patchOffset = bankOffset + 34 * (numMelody + numPerc);

    const unsigned instSize = (version >= 3) ? 66 : 62;
    const unsigned bankInfoSize = (version >= 2) ? 34 : 0;

    if (size < (numMelody + numPerc) * (128 * instSize + bankInfoSize))
        return false;

    for (unsigned i = 0; i < 128 * (numMelody + numPerc); i++) {
        const uint8_t *bytes;
        unsigned key = i & 0x7f;
        if (version >= 2) {
            const unsigned bank = i >> 7;
            bytes = data + bankOffset + 34 * bank;

            if (bank >= numMelody) // percussion banks (use LSB)
                key |= (bytes[32] << 8) | 0x80;
            else if (bytes[32]) // bank LSB set (XG)
                key |= (bytes[32] << 8);
            else if (bytes[33]) // bank MSB set (GS)
                key |= (bytes[33] << 8);
        }

        bytes = data + patchOffset + instSize * i;

        // ignore other data for this patch if it's a blank instrument
        // *or* if one of the rhythm mode bits is set (not supported here)
        if (bytes[39] & 0x3c)
            continue;

        OPLPatch &patch = patches[key];
        // clear patch data
        patch = OPLPatch();

        // patch names
        if (bytes[0])
            patch.name = std::string((const char *)bytes, 31);
        else
            patch.name = names[key & 0xff];

        // patch global settings
        patch.voice[0].tune = (int8_t)bytes[33] - 12;
        patch.voice[1].tune = (int8_t)bytes[35] - 12;
        patch.velocity = (int8_t)bytes[36];
        patch.voice[1].finetune = OPLPlayer::midiCalcBend((int8_t)bytes[37] / 64.0);
        patch.fixedNote = bytes[38];
        patch.fourOp = (bytes[39] & 3) == 1;
        patch.dualTwoOp = (bytes[39] & 3) == 3;
        patch.voice[0].conn = bytes[40];
        patch.voice[1].conn = bytes[41];

        // patch operator settings
        unsigned pos = 42;
        for (unsigned op = 0; op < 4; op++) {
            PatchVoice &voice = patch.voice[op / 2];

            const unsigned n = (op % 2) ^ 1;

            voice.op_mode[n] = bytes[pos++];
            voice.op_ksr[n] = bytes[pos] & 0xc0;
            voice.op_level[n] = bytes[pos++] & 0x3f;
            voice.op_ad[n] = bytes[pos++];
            voice.op_sr[n] = bytes[pos++];
            voice.op_wave[n] = bytes[pos++];
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
bool OPLPatch::loadOP2(OPLPatchSet &patches, const uint8_t *data, size_t size) {
    if (size < 175 * (36 + 32) + 8)
        return false;

    if (strncmp((const char *)data, "#OPL_II#", 8))
        return false;

    // read data for all patches (128 melodic + 47 percussion)
    for (int i = 0; i < 128 + 47; i++) {
        // patches 0-127 are melodic; the rest are for percussion notes 35 thru 81
        unsigned key = (i < 128) ? i : (i + 35);

        OPLPatch &patch = patches[key];
        // clear patch data
        patch = OPLPatch();

        // seek to patch data
        const uint8_t *bytes = data + (36 * i) + 8;

        // read the common data for both 2op voices
        // flag bit 0 is "fixed pitch" (for drums), but it's seemingly only used for drum patches anyway, so ignore it?
        patch.dualTwoOp = (bytes[0] & 4);
        // second voice detune
        patch.voice[1].finetune = OPLPlayer::midiCalcBend((int8_t)(bytes[2] - 128) / 64.0);

        patch.fixedNote = bytes[3];

        // read data for both 2op voices
        unsigned pos = 4;
        for (int j = 0; j < 2; j++) {
            PatchVoice &voice = patch.voice[j];

            for (int op = 0; op < 2; op++) {
                // operator mode
                voice.op_mode[op] = bytes[pos++];
                // operator envelope
                voice.op_ad[op] = bytes[pos++];
                voice.op_sr[op] = bytes[pos++];
                // operator waveform
                voice.op_wave[op] = bytes[pos++];
                // KSR & output level
                voice.op_ksr[op] = bytes[pos++] & 0xc0;
                voice.op_level[op] = bytes[pos++] & 0x3f;

                // feedback/connection (first op only)
                if (op == 0)
                    voice.conn = bytes[pos];
                pos++;
            }

            // midi note offset (int16, but only really need the LSB)
            voice.tune = (int8_t)bytes[pos];
            pos += 2;
        }

        // fix for some bugged DMX patches (e.g. Doom II electric snare)
        if (!(patch.voice[1].op_ad[0] | patch.voice[1].op_ad[1]))
            patch.dualTwoOp = false;

        // seek to patch name
        bytes = data + (32 * i) + (36 * 175) + 8;
        if (bytes[0])
            patch.name = std::string((const char *)bytes, 31);
        else
            patch.name = names[key];
    }

    return true;
}

// ----------------------------------------------------------------------------
bool OPLPatch::loadAIL(OPLPatchSet &patches, const uint8_t *data, size_t size) {
    int index = 0;

    while (true) {
        if (size < index * 6)
            return false;

        const uint8_t *entry = data + (index * 6);
        uint16_t key;

        if (entry[0] == 0xff && entry[1] == 0xff)
            return true; // end of patches
        else if (entry[1] == 0x7f)
            key = entry[0] | 0x80;
        else
            key = (entry[0] | (entry[1] << 8)) & 0x7f7f;

        OPLPatch &patch = patches[key];
        // clear patch data
        patch = OPLPatch();
        patch.name = names[key & 0xff];

        uint32_t patchPos = entry[2] | (entry[3] << 8) | (entry[4] << 16) | (entry[5] << 24);
        if (size < patchPos)
            return false;

        const uint8_t *bytes = data + patchPos;

        if (size < patchPos + bytes[0])
            return false;
        else if (bytes[0] == 0x0e)
            patch.fourOp = false;
        else if (bytes[0] == 0x19)
            patch.fourOp = true;
        else
            return false;
        index++;

        patch.voice[0].tune = patch.voice[1].tune = (int8_t)bytes[2] - 12;
        patch.voice[0].conn = bytes[8] & 0x0f;
        patch.voice[1].conn = bytes[8] >> 7;

        unsigned pos = 3;
        for (int i = 0; i < (patch.fourOp ? 2 : 1); i++) {
            PatchVoice &voice = patch.voice[i];

            for (int op = 0; op < 2; op++) {
                // operator mode
                voice.op_mode[op] = bytes[pos++];
                // KSR & output level
                voice.op_ksr[op] = bytes[pos] & 0xc0;
                voice.op_level[op] = bytes[pos++] & 0x3f;
                // operator envelope
                voice.op_ad[op] = bytes[pos++];
                voice.op_sr[op] = bytes[pos++];
                // operator waveform
                voice.op_wave[op] = bytes[pos++];

                // already handled the feedback/connection byte
                if (op == 0)
                    pos++;
            }
        }
    }
}

// ----------------------------------------------------------------------------
bool OPLPatch::loadTMB(OPLPatchSet &patches, const uint8_t *data, size_t size) {
    if (size < 256 * 13)
        return false;

    for (uint16_t key = 0; key < 256; key++) {
        OPLPatch &patch = patches[key];
        // clear patch data
        patch = OPLPatch();
        patch.name = names[key];

        const uint8_t *bytes = data + (key * 13);

        // since this format has no identifying info, we can only really reject it
        // if it has invalid values in a few spots
        if ((bytes[8] | bytes[9] | bytes[10]) & 0xf0)
            return false;

        PatchVoice &voice = patch.voice[0];
        voice.op_mode[0] = bytes[0];
        voice.op_mode[1] = bytes[1];
        voice.op_ksr[0] = bytes[2] & 0xc0;
        voice.op_level[0] = bytes[2] & 0x3f;
        voice.op_ksr[1] = bytes[3] & 0xc0;
        voice.op_level[1] = bytes[3] & 0x3f;
        voice.op_ad[0] = bytes[4];
        voice.op_ad[1] = bytes[5];
        voice.op_sr[0] = bytes[6];
        voice.op_sr[1] = bytes[7];
        voice.op_wave[0] = bytes[8];
        voice.op_wave[1] = bytes[9];
        voice.conn = bytes[10];
        voice.tune = (int8_t)bytes[11] - 12;
        patch.velocity = (int8_t)bytes[12];
    }

    return true;
}
