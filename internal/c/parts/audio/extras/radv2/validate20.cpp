/*

    Code to check a RAD V2 tune file is valid.  That is, it will check the tune
    can be played without crashing the player.  It doesn't exhaustively check
    the tune except where needed to prevent a possible player crash.

    Call RADValidate with a pointer to your tune and the size in bytes.  It
    will return either NULL for all okay, or a pointer to a null-terminated
    string describing what is wrong with the data.

*/

#include <cstdint>

//==================================================================================================
// The error strings are all supplied here in case you want to translate them to another language
// (or supply your own more descriptive error messages).
//==================================================================================================
const char *g_RADNotATuneFile = "Not a RAD tune file.";
const char *g_RADNotAVersion21Tune = "Not a version 2.1 file format RAD tune.";
const char *g_RADTruncated = "Tune file has been truncated and is incomplete.";
const char *g_RADBadFlags = "Tune file has invalid flags.";
const char *g_RADBadBPMValue = "Tune's BPM value is out of range.";
const char *g_RADBadInstrument = "Tune file contains a bad instrument definition.";
const char *g_RADUnknownMIDIVersion = "Tune file contains an unknown MIDI instrument version.";
const char *g_RADOrderListTooLarge = "Order list in tune file is an invalid size.";
const char *g_RADBadJumpMarker = "Order list jump marker is invalid.";
const char *g_RADBadOrderEntry = "Order list entry is invalid.";
const char *g_RADBadPattNum = "Tune file contains a bad pattern index.";
const char *g_RADPattTruncated = "Tune file contains a truncated pattern.";
const char *g_RADPattExtraData = "Tune file contains a pattern with extraneous data.";
const char *g_RADPattBadLineNum = "Tune file contains a pattern with a bad line definition.";
const char *g_RADPattBadChanNum = "Tune file contains a pattern with a bad channel definition.";
const char *g_RADPattBadNoteNum = "Pattern contains a bad note number.";
const char *g_RADPattBadInstNum = "Pattern contains a bad instrument number.";
const char *g_RADPattBadEffect = "Pattern contains a bad effect and/or parameter.";
const char *g_RADBadRiffNum = "Tune file contains a bad riff index.";
const char *g_RADExtraBytes = "Tune file contains extra bytes.";

//=============================================================================
// Validate a RAD V1 file. -Lachesis
//=============================================================================
static const char *RADValidate10(const void *data, const uint8_t *end) {
    const uint8_t *start = (const uint8_t *)data;
    const uint8_t *pos = start;
    uint16_t pattern_offsets[32];
    uint32_t i;
    bool pattern_used[32] = {false};
    bool last_line;
    bool last_note;

    // NOTE: an extra byte was allocated and set to null so some extra tracks
    // would pass this, so bump the end pointer.
    end++;

    // Description
    pos += 17;
    if (*(pos++) & 0x80) {
        do {
            if (pos >= end)
                return g_RADTruncated;
        } while (*(pos++));
    }

    // Instruments
    while (true) {
        uint8_t num = *(pos++);

        if (num) {
            pos += 11;

            if (pos >= end)
                return g_RADTruncated;
        } else
            break;
    }

    // Order list
    uint8_t num_orders = *(pos++);
    if (num_orders > 128)
        return g_RADOrderListTooLarge;

    if (pos + num_orders >= end)
        return g_RADTruncated;

    for (i = 0; i < num_orders; i++) {
        uint8_t order = *(pos++);

        // Jump marker
        if (order & 0x80) {
            order &= 0x7F;
            if (order >= num_orders)
                return g_RADBadJumpMarker;
        }

        // Pattern number
        else {
            if (order >= 32)
                return g_RADBadOrderEntry;

            // Mark used patterns for validation.
            // TODO: possibly walk through the order list instead so unreached
            // orders don't have their patterns marked used.
            pattern_used[order] = true;
        }
    }

    // Pattern offset table
    if (pos + 64 >= end)
        return g_RADTruncated;

    for (i = 0; i < 32; i++) {
        // Ignore offsets to unused patterns- usually they will already be
        // zero, but "blue_ad.rad" has unused garbage patterns that will fail
        // validation despite the track otherwise working fine.
        pattern_offsets[i] = pattern_used[i] ? (pos[0] | (pos[1] << 8)) : 0;
        pos += 2;

        if (pattern_used[i] && (start + pattern_offsets[i] >= end))
            return g_RADPattTruncated;
    }

    // Patterns
    for (i = 0; i < 32; i++) {
        if (!pattern_offsets[i])
            continue;

        pos = start + pattern_offsets[i];

        do {
            // Line number
            uint8_t line_num = *(pos++);
            if ((line_num & 0x7F) >= 64)
                return g_RADPattBadLineNum;

            last_line = (line_num & 0x80) ? true : false;

            do {
                if (pos + 2 >= end)
                    return g_RADPattTruncated;

                // Channel
                uint8_t channel = *(pos++);
                if ((channel & 0x7F) >= 9)
                    return g_RADPattBadChanNum;

                last_note = (channel & 0x80) ? true : false;

                uint8_t b1 = *(pos++);
                uint8_t b2 = *(pos++);

                // Note
                uint8_t note = (b1 & 0x0F);
                if (note == 13 || note == 14)
                    return g_RADPattBadNoteNum;

                // Instrument
                // uint8_t inst = ((b1 & 0x80) >> 4) | ((b2 & 0xF0) >> 1);

                // Effect
                uint8_t fx = (b2 & 0x0F);
                if (fx) {
                    // Parameter
                    // uint8_t param = *pos;
                    pos++;
                }
            } while (!last_note);
        } while (!last_line);
    }
    return 0;
}

//==================================================================================================
// Validate a RAD V2 (file format 2.1) tune file.  Note, this uses no C++ standard library code.
//==================================================================================================
static const char *RADCheckPattern(const uint8_t *&s, const uint8_t *e, bool riff) {

    // Get pattern size
    if (s + 2 > e)
        return g_RADTruncated;
    uint16_t pattsize = s[0] | (uint16_t(s[1]) << 8);
    s += 2;

    // Calculate end of pattern
    const uint8_t *pe = s + pattsize;
    if (pe > e)
        return g_RADTruncated;

    uint8_t linedef, chandef;
    do {

        // Check line of pattern
        if (s >= pe)
            return g_RADPattTruncated;
        linedef = *s++;
        uint8_t linenum = linedef & 0x7F;
        if (linenum >= 64)
            return g_RADPattBadLineNum;

        do {

            // Check channel of pattern
            if (s >= pe)
                return g_RADPattTruncated;
            chandef = *s++;
            uint8_t channum = chandef & 0x0F;
            if (!riff && channum >= 9)
                return g_RADPattBadChanNum;

            // Check note
            if (chandef & 0x40) {
                if (s >= pe)
                    return g_RADPattTruncated;
                uint8_t note = *s++;
                uint8_t notenum = note & 15;
                // uint8_t octave = (note >> 4) & 7;
                if (notenum == 0 || notenum == 13 || notenum == 14)
                    return g_RADPattBadNoteNum;
            }

            // Check instrument.  This shouldn't be supplied if bit 7 of the note byte is set,
            // but it doesn't break anything if it is so we don't check for it
            if (chandef & 0x20) {
                if (s >= pe)
                    return g_RADPattTruncated;
                uint8_t inst = *s++;
                if (inst == 0 || inst >= 128)
                    return g_RADPattBadInstNum;
            }

            // Check effect.  A non-existent effect could be supplied, but it'll just be
            // ignored by the player so we don't care
            if (chandef & 0x10) {
                if (s + 2 > pe)
                    return g_RADPattTruncated;
                uint8_t effect = *s++;
                uint8_t param = *s++;
                if (effect > 31 || param > 99)
                    return g_RADPattBadEffect;
            }

        } while (!(chandef & 0x80));

    } while (!(linedef & 0x80));

    if (s != pe)
        return g_RADPattExtraData;

    return 0;
}

//--------------------------------------------------------------------------------------------------
const char *RADValidate(const void *data, size_t data_size) {

    const uint8_t *s = (const uint8_t *)data;
    const uint8_t *e = s + data_size;
    uint8_t version;

    // Check header
    if (data_size < 17)
        return g_RADNotATuneFile;

    const char *hdrtxt = "RAD by REALiTY!!";
    for (int i = 0; i < 16; i++)
        if (char(*s++) != *hdrtxt++)
            return g_RADNotATuneFile;

    // Check version
    version = *(s++);
    if (version == 0x10)
        return RADValidate10(data, e);
    if (version != 0x21)
        return g_RADNotAVersion21Tune;

    // Check flags
    if (s >= e)
        return g_RADTruncated;

    uint8_t flags = *s++;
    if (flags & 0x80)
        return g_RADBadFlags; // Bit 7 is unused

    // NOTE: the vanilla validator incorrectly checks 0x40 here.
    if (flags & 0x20) {
        if (s + 2 > e)
            return g_RADTruncated;
        uint16_t bpm = s[0] | (uint16_t(s[1]) << 8);
        s += 2;
        if (bpm < 46 || bpm > 300)
            return g_RADBadBPMValue;
    }

    // Check description.  This is actually freeform text so there's not a lot to check, just that
    // it's a null-terminated string
    do {
        if (s >= e)
            return g_RADTruncated;
    } while (*s++);

    // Check instruments.  We don't actually validate the individual instrument fields as the tune
    // file will still play with bad instrument data.  We're only concerned that the tune file
    // doesn't crash the player
    uint8_t last_inst = 0;
    while (1) {

        // Get instrument number, or 0 for end of instrument list
        if (s >= e)
            return g_RADTruncated;
        uint8_t inst = *s++;
        if (inst == 0)
            break;

        // RAD always saves the instruments out in order
        if (inst > 127 || inst <= last_inst)
            return g_RADBadInstrument;
        last_inst = inst;

        // Check the name
        if (s >= e)
            return g_RADTruncated;
        uint8_t namelen = *s++;
        s += namelen;

        // Get algorithm
        if (s > e)
            return g_RADTruncated;
        uint8_t alg = *s;

        if ((alg & 7) == 7) {

            // MIDI instrument.  We need to check the version as this can affect the following
            // data size
            if (s + 6 > e)
                return g_RADTruncated;
            if (s[2] >> 4)
                return g_RADUnknownMIDIVersion;
            // NOTE the vanilla validator skips 6 bytes here as if the algorithm
            // byte was already skipped. It wasn't, so skip 7 instead.
            s += 7;

        } else {

            s += 24;
            if (s > e)
                return g_RADTruncated;
        }

        // Riff track supplied?
        if (alg & 0x80) {

            const char *err = RADCheckPattern(s, e, false);
            if (err)
                return err;
        }
    }

    // Get the order list
    if (s >= e)
        return g_RADTruncated;
    uint8_t order_size = *s++;
    const uint8_t *order_list = s;
    if (order_size > 128)
        return g_RADOrderListTooLarge;
    s += order_size;

    for (uint8_t i = 0; i < order_size; i++) {
        uint8_t order = order_list[i];

        if (order & 0x80) {

            // Check jump marker
            order &= 0x7F;
            if (order >= order_size)
                return g_RADBadJumpMarker;
        } else {

            // Check pattern number.  It doesn't matter if there is no pattern with this number
            // defined later, as missing patterns are treated as empty
            if (order >= 100)
                return g_RADBadOrderEntry;
        }
    }

    // Check the patterns
    while (1) {

        // Get pattern number
        if (s >= e)
            return g_RADTruncated;
        uint8_t pattnum = *s++;

        // Last pattern?
        if (pattnum == 0xFF)
            break;

        if (pattnum >= 100)
            return g_RADBadPattNum;

        const char *err = RADCheckPattern(s, e, false);
        if (err)
            return err;
    }

    // Check the riffs
    while (1) {

        // Get riff number
        if (s >= e)
            return g_RADTruncated;
        uint8_t riffnum = *s++;

        // Last riff?
        if (riffnum == 0xFF)
            break;

        uint8_t riffpatt = riffnum >> 4;
        uint8_t riffchan = riffnum & 15;
        if (riffpatt > 9 || riffchan == 0 || riffchan > 9)
            return g_RADBadRiffNum;

        const char *err = RADCheckPattern(s, e, true);
        if (err)
            return err;
    }

    // We should be at the end of the file now.  Note, you can safely remove this check if you
    // like - extra bytes won't affect playback
    if (s != e)
        return g_RADExtraBytes;

    // Tune file is all good
    return 0;
}
