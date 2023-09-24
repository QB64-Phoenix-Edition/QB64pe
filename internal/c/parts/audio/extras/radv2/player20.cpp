/*

    C++ player code for Reality Adlib Tracker 2.0a (file version 2.1).

    Please note, this is just the player code.  This does no checking of the tune data before
    it tries to play it, as most use cases will be a known tune being used in a production.
    So if you're writing an application that loads unknown tunes in at run time then you'll
    want to do more validity checking.

    To use:

        - Instantiate the RADPlayer object

        - Initialise player for your tune by calling the Init() method.  Supply a pointer to the
          tune file and a function for writing to the OPL3 registers.

        - Call the Update() method a number of times per second as returned by GetHertz().  If
          your tune is using the default BPM setting you can safely just call it 50 times a
          second, unless it's a legacy "slow-timer" tune then it'll need to be 18.2 times a
          second.

        - When you're done, stop calling Update() and call the Stop() method to turn off all
          sound and reset the OPL3 hardware.

*/

#include <cstdint>
#include <cstdlib>
#include <cstring>

#ifndef RAD_DETECT_REPEATS
#    define RAD_DETECT_REPEATS 0
#endif

//==================================================================================================
// RAD player class.
//==================================================================================================
class RADPlayer {

    // Various constants
    enum {
        kTracks = 100,
        kChannels = 9,
        kTrackLines = 64,
        kRiffTracks = 10,
        kInstruments = 127,

        cmPortamentoUp = 0x1,
        cmPortamentoDwn = 0x2,
        cmToneSlide = 0x3,
        cmToneVolSlide = 0x5,
        cmVolSlide = 0xA,
        cmSetVol = 0xC,
        cmJumpToLine = 0xD,
        cmSetSpeed = 0xF,
        cmIgnore = ('I' - 55),
        cmMultiplier = ('M' - 55),
        cmRiff = ('R' - 55),
        cmTranspose = ('T' - 55),
        cmFeedback = ('U' - 55),
        cmVolume = ('V' - 55)
    };

    enum e_Source { SNone, SRiff, SIRiff };

    enum { fKeyOn = 1 << 0, fKeyOff = 1 << 1, fKeyedOn = 1 << 2 };

    struct CInstrument {
        uint8_t Feedback[2];
        uint8_t Panning[2];
        uint8_t Algorithm;
        uint8_t Detune;
        uint8_t Volume;
        uint8_t RiffSpeed;
        uint8_t *Riff;
        uint8_t Operators[4][5];
    };

    struct CEffects {
        int8_t PortSlide;
        int8_t VolSlide;
        uint16_t ToneSlideFreq;
        uint8_t ToneSlideOct;
        uint8_t ToneSlideSpeed;
        int8_t ToneSlideDir;
    };

    struct CChannel {
        uint8_t LastInstrument;
        CInstrument *Instrument;
        uint8_t Volume;
        uint8_t DetuneA;
        uint8_t DetuneB;
        uint8_t KeyFlags;
        uint16_t CurrFreq;
        int8_t CurrOctave;
        CEffects FX;
        struct CRiff {
            CEffects FX;
            uint8_t *Track;
            uint8_t *TrackStart;
            uint8_t Line;
            uint8_t Speed;
            uint8_t SpeedCnt;
            int8_t TransposeOctave;
            int8_t TransposeNote;
            uint8_t LastInstrument;
        } Riff, IRiff;
    };

  public:
    RADPlayer() : Initialised(false) {}
    void Init(const void *tune, void (*opl3)(void *, uint16_t, uint8_t), void *arg);
    void Stop();
    bool Update();
    int GetHertz() const { return Hertz; }
    int GetPlayTimeInSeconds() const { return PlayTime / Hertz; }
    int GetTunePos() const { return Order; }
    int GetTuneLength() const { return OrderListSize; }
    int GetTuneLine() const { return Line; }
    void SetMasterVolume(int vol) { MasterVol = vol; }
    int GetMasterVolume() const { return MasterVol; }
    int GetSpeed() const { return Speed; }

    /* BEGIN MEGAZEUX ADDITIONS */

    void SetTunePos(uint32_t order, uint32_t line);
    int GetTuneEffectiveLength();

    /* END MEGAZEUX ADDITIONS */

#if RAD_DETECT_REPEATS
    uint32_t ComputeTotalTime();
#endif

  private:
    bool UnpackNote(uint8_t *&s, uint8_t &last_instrument);
    uint8_t *GetTrack();
    uint8_t *SkipToLine(uint8_t *trk, uint8_t linenum, bool chan_riff = false);
    void PlayLine();
    void PlayNote(int channum, int8_t notenum, int8_t octave, uint16_t instnum, uint8_t cmd = 0, uint8_t param = 0, e_Source src = SNone, int op = 0);
    void LoadInstrumentOPL3(int channum);
    void PlayNoteOPL3(int channum, int8_t octave, int8_t note);
    void ResetFX(CEffects *fx);
    void TickRiff(int channum, CChannel::CRiff &riff, bool chan_riff);
    void ContinueFX(int channum, CEffects *fx);
    void SetVolume(int channum, uint8_t vol);
    void GetSlideDir(int channum, CEffects *fx);
    void LoadInstMultiplierOPL3(int channum, int op, uint8_t mult);
    void LoadInstVolumeOPL3(int channum, int op, uint8_t vol);
    void LoadInstFeedbackOPL3(int channum, int which, uint8_t fb);
    void Portamento(uint16_t channum, CEffects *fx, int8_t amount, bool toneslide);
    void Transpose(int8_t note, int8_t octave);
    void SetOPL3(uint16_t reg, uint8_t val) {
        OPL3Regs[reg] = val;
        OPL3(OPL3Arg, reg, val);
    }
    uint8_t GetOPL3(uint16_t reg) const { return OPL3Regs[reg]; }

    /* BEGIN MEGAZEUX ADDITIONS */

    void Init10(const void *tune);
    bool UnpackNote10(uint8_t *&s);
    uint8_t *SkipToLine10(uint8_t *trk, uint8_t linenum);
    uint8_t FixRadv21KSLVolume(uint8_t val);
    int LastPatternOrder;
    bool Is10;

    /* END MEGAZEUX ADDITIONS */

    void (*OPL3)(void *, uint16_t, uint8_t);
    void *OPL3Arg;
    CInstrument Instruments[kInstruments];
    CChannel Channels[kChannels];
    uint32_t PlayTime;
#if RAD_DETECT_REPEATS
    uint32_t OrderMap[4];
    bool Repeating;
#endif
    int16_t Hertz;
    uint8_t *OrderList;
    uint8_t *Tracks[kTracks];
    uint8_t *Riffs[kRiffTracks][kChannels];
    uint8_t *Track;
    bool Initialised;
    uint8_t Speed;
    uint8_t OrderListSize;
    uint8_t SpeedCnt;
    uint8_t Order;
    uint8_t Line;
    int8_t Entrances;
    uint8_t MasterVol;
    int8_t LineJump;
    uint8_t OPL3Regs[512];

    // Values exported by UnpackNote()
    int8_t NoteNum;
    int8_t OctaveNum;
    uint8_t InstNum;
    uint8_t EffectNum;
    uint8_t Param;
    // Unused field, commented out to suppress warnings.
    // bool                LastNote;

    static const int8_t NoteSize[];
    static const uint16_t ChanOffsets3[9], Chn2Offsets3[9];
    static const uint16_t NoteFreq[];
    static const uint16_t OpOffsets3[9][4];
    static const bool AlgCarriers[7][4];
};
//--------------------------------------------------------------------------------------------------
const int8_t RADPlayer::NoteSize[] = {0, 2, 1, 3, 1, 3, 2, 4};
const uint16_t RADPlayer::ChanOffsets3[9] = {0, 1, 2, 0x100, 0x101, 0x102, 6, 7, 8};             // OPL3 first channel
const uint16_t RADPlayer::Chn2Offsets3[9] = {3, 4, 5, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108}; // OPL3 second channel
const uint16_t RADPlayer::NoteFreq[] = {0x16b, 0x181, 0x198, 0x1b0, 0x1ca, 0x1e5, 0x202, 0x220, 0x241, 0x263, 0x287, 0x2ae};
// clang-format off
const uint16_t RADPlayer::OpOffsets3[9][4] = {
    {  0x00B, 0x008, 0x003, 0x000  },
    {  0x00C, 0x009, 0x004, 0x001  },
    {  0x00D, 0x00A, 0x005, 0x002  },
    {  0x10B, 0x108, 0x103, 0x100  },
    {  0x10C, 0x109, 0x104, 0x101  },
    {  0x10D, 0x10A, 0x105, 0x102  },
    {  0x113, 0x110, 0x013, 0x010  },
    {  0x114, 0x111, 0x014, 0x011  },
    {  0x115, 0x112, 0x015, 0x012  }
};
const bool RADPlayer::AlgCarriers[7][4] = {
    {  true, false, false, false  },  // 0 - 2op - op < op
    {  true, true,  false, false  },  // 1 - 2op - op + op
    {  true, false, false, false  },  // 2 - 4op - op < op < op < op
    {  true, false, false, true   },  // 3 - 4op - op < op < op + op
    {  true, false, true,  false  },  // 4 - 4op - op < op + op < op
    {  true, false, true,  true   },  // 5 - 4op - op < op + op + op
    {  true, true,  true,  true   },  // 6 - 4op - op + op + op + op
};
// clang-format on

//==================================================================================================
// Initialise a RAD tune for playback.  This assumes the tune data is valid and does minimal data
// checking.
//==================================================================================================
void RADPlayer::Init(const void *tune, void (*opl3)(void *, uint16_t, uint8_t), void *arg) {

    Initialised = false;
    Is10 = false;
    LastPatternOrder = -1;

    // Version check; we only support version 2.1 tune files
    uint8_t version = *((uint8_t *)tune + 0x10);
    if ((version != 0x10) && (version != 0x21)) {
        Hertz = -1;
        return;
    }

    // The OPL3 call-back
    OPL3 = opl3;
    OPL3Arg = arg;

    for (int i = 0; i < kTracks; i++)
        Tracks[i] = 0;

    for (int i = 0; i < kRiffTracks; i++)
        for (int j = 0; j < kChannels; j++)
            Riffs[i][j] = 0;

    // These aren't guaranteed to all be stored in the file...
    memset(&Instruments, 0, sizeof(Instruments));

    if (version == 0x10) {
        Is10 = true;
        Init10(tune);
        return;
    }

    uint8_t *s = (uint8_t *)tune + 0x11;

    uint8_t flags = *s++;
    Speed = flags & 0x1F;

    // Is BPM value present?
    Hertz = 50;
    if (flags & 0x20) {
        Hertz = (s[0] | (int(s[1]) << 8)) * 2 / 5;
        s += 2;
    }

    // Slow timer tune?  Return an approximate hz
    if (flags & 0x40)
        Hertz = 18;

    // Skip any description
    while (*s)
        s++;
    s++;

    // Unpack the instruments
    while (1) {

        // Instrument number, 0 indicates end of list
        uint8_t inst_num = *s++;
        if (inst_num == 0)
            break;

        // Skip instrument name
        uint8_t name_len = *(s++);
        s += name_len;

        CInstrument &inst = Instruments[inst_num - 1];

        uint8_t alg = *s++;
        inst.Algorithm = alg & 7;
        inst.Panning[0] = (alg >> 3) & 3;
        inst.Panning[1] = (alg >> 5) & 3;

        if (inst.Algorithm < 7) {

            uint8_t b = *s++;
            inst.Feedback[0] = b & 15;
            inst.Feedback[1] = b >> 4;

            b = *s++;
            inst.Detune = b >> 4;
            inst.RiffSpeed = b & 15;

            inst.Volume = *s++;

            for (int i = 0; i < 4; i++) {
                uint8_t *op = inst.Operators[i];
                op[0] = *s++;
                op[1] = FixRadv21KSLVolume(*s++);
                op[2] = *s++;
                op[3] = *s++;
                op[4] = *s++;
            }

        } else {

            // Ignore MIDI instrument data
            s += 6;
        }

        // Instrument riff?
        if (alg & 0x80) {
            int size = s[0] | (int(s[1]) << 8);
            s += 2;
            inst.Riff = s;
            s += size;
        } else
            inst.Riff = 0;
    }

    // Get order list
    OrderListSize = *s++;
    OrderList = s;
    s += OrderListSize;

    // Locate the tracks
    while (1) {

        // Track number
        uint8_t track_num = *s++;
        if (track_num >= kTracks)
            break;

        // Track size in bytes
        int size = s[0] | (int(s[1]) << 8);
        s += 2;

        Tracks[track_num] = s;
        s += size;
    }

    // Locate the riffs
    while (1) {

        // Riff id
        uint8_t riffid = *s++;
        uint8_t riffnum = riffid >> 4;
        uint8_t channum = riffid & 15;
        if (riffnum >= kRiffTracks || channum > kChannels)
            break;

        // Track size in bytes
        int size = s[0] | (int(s[1]) << 8);
        s += 2;

        Riffs[riffnum][channum - 1] = s;
        s += size;
    }

    // Done parsing tune, now set up for play
    for (int i = 0; i < 512; i++)
        OPL3Regs[i] = 255;
    Stop();

    Initialised = true;
}

//==================================================================================================
// Stop all sounds and reset the tune.  Tune will play from the beginning again if you continue to
// Update().
//==================================================================================================
void RADPlayer::Stop() {

    // Clear all registers
    for (uint16_t reg = 0x20; reg < 0xF6; reg++) {

        // Ensure envelopes decay all the way
        uint8_t val = (reg >= 0x60 && reg < 0xA0) ? 0xFF : 0;

        SetOPL3(reg, val);
        SetOPL3(reg + 0x100, val);
    }

    // Configure OPL3
    SetOPL3(1, 0x20);  // Allow waveforms
    SetOPL3(8, 0);     // No split point
    SetOPL3(0xbd, 0);  // No drums, etc.
    SetOPL3(0x104, 0); // Everything 2-op by default
    SetOPL3(0x105, 1); // OPL3 mode on

#if RAD_DETECT_REPEATS
    // The order map keeps track of which patterns we've played so we can detect when the tune
    // starts to repeat.  Jump markers can't be reliably used for this
    PlayTime = 0;
    Repeating = false;
    for (int i = 0; i < 4; i++)
        OrderMap[i] = 0;
#endif

    // Initialise play values
    SpeedCnt = 1;
    Order = 0;
    Track = GetTrack();
    Line = 0;
    Entrances = 0;
    MasterVol = 64;

    // Initialise channels
    for (int i = 0; i < kChannels; i++) {
        CChannel &chan = Channels[i];
        chan.LastInstrument = 0;
        chan.Instrument = 0;
        chan.Volume = 0;
        chan.DetuneA = 0;
        chan.DetuneB = 0;
        chan.KeyFlags = 0;
        chan.Riff.SpeedCnt = 0;
        chan.IRiff.SpeedCnt = 0;
    }
}

//==================================================================================================
// Playback update.  Call BPM * 2 / 5 times a second.  Use GetHertz() for this number after the
// tune has been initialised.  Returns true if tune is starting to repeat.
//==================================================================================================
bool RADPlayer::Update() {

    if (!Initialised)
        return false;

    // Run riffs
    for (int i = 0; i < kChannels; i++) {
        CChannel &chan = Channels[i];
        TickRiff(i, chan.IRiff, false);
        TickRiff(i, chan.Riff, true);
    }

    // Run main track
    PlayLine();

    // Run effects
    for (int i = 0; i < kChannels; i++) {
        CChannel &chan = Channels[i];
        ContinueFX(i, &chan.IRiff.FX);
        ContinueFX(i, &chan.Riff.FX);
        ContinueFX(i, &chan.FX);
    }

    // Update play time.  We convert to seconds when queried
    PlayTime++;

#if RAD_DETECT_REPEATS
    return Repeating;
#else
    return false;
#endif
}

//==================================================================================================
// Unpacks a single RAD note.
//==================================================================================================
bool RADPlayer::UnpackNote(uint8_t *&s, uint8_t &last_instrument) {

    if (Is10)
        return UnpackNote10(s);

    uint8_t chanid = *s++;

    InstNum = 0;
    EffectNum = 0;
    Param = 0;

    // Unpack note data
    uint8_t note = 0;
    if (chanid & 0x40) {
        uint8_t n = *s++;
        note = n & 0x7F;

        // Retrigger last instrument?
        if (n & 0x80)
            InstNum = last_instrument;
    }

    // Do we have an instrument?
    if (chanid & 0x20) {
        InstNum = *s++;
        last_instrument = InstNum;
    }

    // Do we have an effect?
    if (chanid & 0x10) {
        EffectNum = *s++;
        Param = *s++;
    }

    NoteNum = note & 15;
    OctaveNum = note >> 4;

    return ((chanid & 0x80) != 0);
}

//==================================================================================================
// Get current track as indicated by order list.
//==================================================================================================
uint8_t *RADPlayer::GetTrack() {

    // If at end of tune start again from beginning
    if (Order >= OrderListSize)
        Order = 0;

    uint8_t track_num = OrderList[Order];

    // Jump marker?  Note, we don't recognise multiple jump markers as that could put us into an
    // infinite loop
    if (track_num & 0x80) {
        Order = track_num & 0x7F;
        track_num = OrderList[Order] & 0x7F;
    }

#if RAD_DETECT_REPEATS
    // Check for tune repeat, and mark order in order map
    if (Order < 128) {
        int byte = Order >> 5;
        uint32_t bit = uint32_t(1) << (Order & 31);
        if (OrderMap[byte] & bit)
            Repeating = true;
        else
            OrderMap[byte] |= bit;
    }
#endif

    return Tracks[track_num];
}

//==================================================================================================
// Skip through track till we reach the given line or the next higher one.  Returns null if none.
//==================================================================================================
uint8_t *RADPlayer::SkipToLine(uint8_t *trk, uint8_t linenum, bool chan_riff) {

    if (Is10)
        return SkipToLine10(trk, linenum);

    while (1) {

        uint8_t lineid = *trk;
        if ((lineid & 0x7F) >= linenum)
            return trk;
        if (lineid & 0x80)
            break;
        trk++;

        // Skip channel notes
        uint8_t chanid;
        do {
            chanid = *trk++;
            trk += NoteSize[(chanid >> 4) & 7];
        } while (!(chanid & 0x80) && !chan_riff);
    }

    return 0;
}

//==================================================================================================
// Plays one line of current track and advances pointers.
//==================================================================================================
void RADPlayer::PlayLine() {

    SpeedCnt--;
    if (SpeedCnt > 0)
        return;
    SpeedCnt = Speed;

    // Reset channel effects
    for (int i = 0; i < kChannels; i++)
        ResetFX(&Channels[i].FX);

    LineJump = -1;

    // At the right line?
    uint8_t *trk = Track;
    if (trk && (*trk & 0x7F) <= Line) {
        uint8_t lineid = *trk++;

        // Run through channels
        bool last;
        do {
            int channum = *trk & 15;
            CChannel &chan = Channels[channum];
            last = UnpackNote(trk, chan.LastInstrument);
            PlayNote(channum, NoteNum, OctaveNum, InstNum, EffectNum, Param);
        } while (!last);

        // Was this the last line?
        if (lineid & 0x80)
            trk = 0;

        Track = trk;
    }

    // Move to next line
    Line++;
    if (Line >= kTrackLines || LineJump >= 0) {

        if (LineJump >= 0)
            Line = LineJump;
        else
            Line = 0;

        // Move to next track in order list
        Order++;
        Track = GetTrack();

        // NOTE: This fixes a bug where the vanilla copy of this player
        // fails to handle Dxx correctly. See: mindflux.rad order 13. -Lachesis
        if (Line > 0)
            Track = SkipToLine(Track, Line, false);
    }
}

//==================================================================================================
// Play a single note.  Returns the line number in the next pattern to jump to if a jump command was
// found, or -1 if none.
//==================================================================================================
void RADPlayer::PlayNote(int channum, int8_t notenum, int8_t octave, uint16_t instnum, uint8_t cmd, uint8_t param, e_Source src, int op) {
    CChannel &chan = Channels[channum];

    // Recursion detector.  This is needed as riffs can trigger other riffs, and they could end up
    // in a loop
    if (Entrances >= 8)
        return;
    Entrances++;

    // Select which effects source we're using
    CEffects *fx = &chan.FX;
    if (src == SRiff)
        fx = &chan.Riff.FX;
    else if (src == SIRiff)
        fx = &chan.IRiff.FX;

    bool transposing = false;

    // For tone-slides the note is the target
    if (cmd == cmToneSlide) {
        if (notenum > 0 && notenum <= 12) {
            fx->ToneSlideOct = octave;
            fx->ToneSlideFreq = NoteFreq[notenum - 1];
        }
        goto toneslide;
    }

    // Playing a new instrument?
    if (instnum > 0) {
        CInstrument *oldinst = chan.Instrument;
        CInstrument *inst = &Instruments[instnum - 1];
        chan.Instrument = inst;

        // Ignore MIDI instruments
        // NOTE: the vanilla player does this, meaning no effects or new riffs
        // will trigger on this line.
        /*
        if (inst->Algorithm == 7) {
            Entrances--;
            return;
        }
        */
        if (inst->Algorithm < 7) {

            LoadInstrumentOPL3(channum);

            // Bounce the channel
            chan.KeyFlags |= fKeyOff | fKeyOn;

            ResetFX(&chan.IRiff.FX);

            if (src != SIRiff || inst != oldinst) {

                // Instrument riff?
                if (inst->Riff && inst->RiffSpeed > 0) {

                    chan.IRiff.Track = chan.IRiff.TrackStart = inst->Riff;
                    chan.IRiff.Line = 0;
                    chan.IRiff.Speed = inst->RiffSpeed;
                    chan.IRiff.LastInstrument = 0;

                    // Note given with riff command is used to transpose the riff
                    if (notenum >= 1 && notenum <= 12) {
                        chan.IRiff.TransposeOctave = octave;
                        chan.IRiff.TransposeNote = notenum;
                        transposing = true;
                    } else {
                        chan.IRiff.TransposeOctave = 3;
                        chan.IRiff.TransposeNote = 12;
                    }

                    // Do first tick of riff
                    chan.IRiff.SpeedCnt = 1;
                    TickRiff(channum, chan.IRiff, false);

                } else
                    chan.IRiff.SpeedCnt = 0;
            }
        } else
            chan.Instrument = 0;
    }

    // Starting a channel riff?
    if (cmd == cmRiff || cmd == cmTranspose) {

        ResetFX(&chan.Riff.FX);

        uint8_t p0 = param / 10;
        uint8_t p1 = param % 10;
        chan.Riff.Track = p1 > 0 ? Riffs[p0][p1 - 1] : 0;
        if (chan.Riff.Track) {

            chan.Riff.TrackStart = chan.Riff.Track;
            chan.Riff.Line = 0;
            chan.Riff.Speed = Speed;
            chan.Riff.LastInstrument = 0;

            // Note given with riff command is used to transpose the riff
            if (cmd == cmTranspose && notenum >= 1 && notenum <= 12) {
                chan.Riff.TransposeOctave = octave;
                chan.Riff.TransposeNote = notenum;
                transposing = true;
            } else {
                chan.Riff.TransposeOctave = 3;
                chan.Riff.TransposeNote = 12;
            }

            // Do first tick of riff
            chan.Riff.SpeedCnt = 1;
            TickRiff(channum, chan.Riff, true);

        } else
            chan.Riff.SpeedCnt = 0;
    }

    // Play the note
    if (!transposing && notenum > 0) {

        // Key-off?
        if (notenum == 15)
            chan.KeyFlags |= fKeyOff;

        if (!chan.Instrument || chan.Instrument->Algorithm < 7)
            PlayNoteOPL3(channum, octave, notenum);
    }

    // Process effect
    switch (cmd) {

    case cmSetVol:
        SetVolume(channum, param);
        break;

    case cmSetSpeed:
        if (src == SNone) {
            Speed = param;
            SpeedCnt = param;
        } else if (src == SRiff) {
            chan.Riff.Speed = param;
            chan.Riff.SpeedCnt = param;
        } else if (src == SIRiff) {
            chan.IRiff.Speed = param;
            chan.IRiff.SpeedCnt = param;
        }
        break;

    case cmPortamentoUp:
        fx->PortSlide = param;
        break;

    case cmPortamentoDwn:
        fx->PortSlide = -int8_t(param);
        break;

    case cmToneVolSlide:
    case cmVolSlide: {
        int8_t val = param;
        if (val >= 50)
            val = -(val - 50);
        fx->VolSlide = val;
        if (cmd != cmToneVolSlide)
            break;
    }
        // Fall through!

    case cmToneSlide: {
    toneslide:
        uint8_t speed = param;
        if (speed)
            fx->ToneSlideSpeed = speed;
        GetSlideDir(channum, fx);
        break;
    }

    case cmJumpToLine: {
        if (param >= kTrackLines)
            break;

        // Note: jump commands in riffs are checked for within TickRiff()
        if (src == SNone)
            LineJump = param;

        break;
    }

    case cmMultiplier: {
        if (src == SIRiff)
            LoadInstMultiplierOPL3(channum, op, param);
        break;
    }

    case cmVolume: {
        if (src == SIRiff)
            LoadInstVolumeOPL3(channum, op, param);
        break;
    }

    case cmFeedback: {
        if (src == SIRiff) {
            uint8_t which = param / 10;
            uint8_t fb = param % 10;
            LoadInstFeedbackOPL3(channum, which, fb);
        }
        break;
    }
    }

    Entrances--;
}

//==================================================================================================
// Sets the OPL3 registers for a given instrument.
//==================================================================================================
void RADPlayer::LoadInstrumentOPL3(int channum) {
    CChannel &chan = Channels[channum];

    const CInstrument *inst = chan.Instrument;
    if (!inst)
        return;

    uint8_t alg = inst->Algorithm;
    chan.Volume = inst->Volume;
    chan.DetuneA = (inst->Detune + 1) >> 1;
    chan.DetuneB = inst->Detune >> 1;

    // Turn on 4-op mode for algorithms 2 and 3 (algorithms 4 to 6 are simulated with 2-op mode)
    if (channum < 6) {
        uint8_t mask = 1 << channum;
        SetOPL3(0x104, (GetOPL3(0x104) & ~mask) | (alg == 2 || alg == 3 ? mask : 0));
    }

    // Left/right/feedback/algorithm
    SetOPL3(0xC0 + ChanOffsets3[channum], ((inst->Panning[1] ^ 3) << 4) | inst->Feedback[1] << 1 | (alg == 3 || alg == 5 || alg == 6 ? 1 : 0));
    SetOPL3(0xC0 + Chn2Offsets3[channum], ((inst->Panning[0] ^ 3) << 4) | inst->Feedback[0] << 1 | (alg == 1 || alg == 6 ? 1 : 0));

    // Load the operators
    for (int i = 0; i < 4; i++) {

        static const uint8_t blank[] = {0, 0x3F, 0, 0xF0, 0};
        const uint8_t *op = (alg < 2 && i >= 2) ? blank : inst->Operators[i];
        uint16_t reg = OpOffsets3[channum][i];

        uint16_t vol = ~op[1] & 0x3F;

        // Do volume scaling for carriers
        if (AlgCarriers[alg][i]) {
            vol = vol * inst->Volume / 64;
            vol = vol * MasterVol / 64;
        }

        SetOPL3(reg + 0x20, op[0]);
        SetOPL3(reg + 0x40, (op[1] & 0xC0) | ((vol ^ 0x3F) & 0x3F));
        SetOPL3(reg + 0x60, op[2]);
        SetOPL3(reg + 0x80, op[3]);
        SetOPL3(reg + 0xE0, op[4]);
    }
}

//==================================================================================================
// Play note on OPL3 hardware.
//==================================================================================================
void RADPlayer::PlayNoteOPL3(int channum, int8_t octave, int8_t note) {
    CChannel &chan = Channels[channum];

    uint16_t o1 = ChanOffsets3[channum];
    uint16_t o2 = Chn2Offsets3[channum];

    // Key off the channel
    if (chan.KeyFlags & fKeyOff) {
        chan.KeyFlags &= ~(fKeyOff | fKeyedOn);
        SetOPL3(0xB0 + o1, GetOPL3(0xB0 + o1) & ~0x20);
        SetOPL3(0xB0 + o2, GetOPL3(0xB0 + o2) & ~0x20);
    }

    if (note == 15)
        return;

    bool op4 = (chan.Instrument && chan.Instrument->Algorithm >= 2);

    uint16_t freq = NoteFreq[note - 1];
    uint16_t frq2 = freq;

    chan.CurrFreq = freq;
    chan.CurrOctave = octave;

    // Detune.  We detune both channels in the opposite direction so the note retains its tuning
    freq += chan.DetuneA;
    frq2 -= chan.DetuneB;

    // Frequency low byte
    if (op4)
        SetOPL3(0xA0 + o1, frq2 & 0xFF);
    SetOPL3(0xA0 + o2, freq & 0xFF);

    // Frequency high bits + octave + key on
    if (chan.KeyFlags & fKeyOn)
        chan.KeyFlags = (chan.KeyFlags & ~fKeyOn) | fKeyedOn;
    if (op4)
        SetOPL3(0xB0 + o1, (frq2 >> 8) | (octave << 2) | ((chan.KeyFlags & fKeyedOn) ? 0x20 : 0));
    else
        SetOPL3(0xB0 + o1, 0);
    SetOPL3(0xB0 + o2, (freq >> 8) | (octave << 2) | ((chan.KeyFlags & fKeyedOn) ? 0x20 : 0));
}

//==================================================================================================
// Prepare FX for new line.
//==================================================================================================
void RADPlayer::ResetFX(CEffects *fx) {
    fx->PortSlide = 0;
    fx->VolSlide = 0;
    fx->ToneSlideDir = 0;
}

//==================================================================================================
// Tick the channel riff.
//==================================================================================================
void RADPlayer::TickRiff(int channum, CChannel::CRiff &riff, bool chan_riff) {
    uint8_t lineid;

    if (riff.SpeedCnt == 0) {
        ResetFX(&riff.FX);
        return;
    }

    riff.SpeedCnt--;
    if (riff.SpeedCnt > 0)
        return;
    riff.SpeedCnt = riff.Speed;

    uint8_t line = riff.Line++;
    if (riff.Line >= kTrackLines)
        riff.SpeedCnt = 0;

    ResetFX(&riff.FX);

    // Is this the current line in track?
    uint8_t *trk = riff.Track;
    if (trk && (*trk & 0x7F) == line) {
        lineid = *trk++;

        if (chan_riff) {

            // Channel riff: play current note
            UnpackNote(trk, riff.LastInstrument);
            Transpose(riff.TransposeNote, riff.TransposeOctave);
            PlayNote(channum, NoteNum, OctaveNum, InstNum, EffectNum, Param, SRiff);

        } else {

            // Instrument riff: here each track channel is an extra effect that can run, but is not
            // actually a different physical channel
            bool last;
            do {
                int col = *trk & 15;
                last = UnpackNote(trk, riff.LastInstrument);
                if (EffectNum != cmIgnore)
                    Transpose(riff.TransposeNote, riff.TransposeOctave);
                PlayNote(channum, NoteNum, OctaveNum, InstNum, EffectNum, Param, SIRiff, col > 0 ? (col - 1) & 3 : 0);
            } while (!last);
        }

        // Last line?
        if (lineid & 0x80)
            trk = 0;

        riff.Track = trk;
    }

    // Special case; if next line has a jump command, run it now
    if (!trk || (*trk++ & 0x7F) != riff.Line)
        return;

    lineid = 0;              // silence warning
    UnpackNote(trk, lineid); // lineid is just a dummy here
    if (EffectNum == cmJumpToLine && Param < kTrackLines) {
        riff.Line = Param;
        riff.Track = SkipToLine(riff.TrackStart, Param, chan_riff);
    }
}

//==================================================================================================
// This continues any effects that operate continuously (eg. slides).
//==================================================================================================
void RADPlayer::ContinueFX(int channum, CEffects *fx) {
    CChannel &chan = Channels[channum];

    if (fx->PortSlide)
        Portamento(channum, fx, fx->PortSlide, false);

    if (fx->VolSlide) {
        int8_t vol = chan.Volume;
        vol -= fx->VolSlide;
        if (vol < 0)
            vol = 0;
        SetVolume(channum, vol);
    }

    if (fx->ToneSlideDir)
        Portamento(channum, fx, fx->ToneSlideDir, true);
}

//==================================================================================================
// Sets the volume of given channel.
//==================================================================================================
void RADPlayer::SetVolume(int channum, uint8_t vol) {
    CChannel &chan = Channels[channum];

    // Ensure volume is within range
    if (vol > 64)
        vol = 64;

    chan.Volume = vol;

    // Scale volume to master volume
    vol = vol * MasterVol / 64;

    CInstrument *inst = chan.Instrument;
    if (!inst)
        return;
    uint8_t alg = inst->Algorithm;

    // Set volume of all carriers
    for (int i = 0; i < 4; i++) {
        uint8_t *op = inst->Operators[i];

        // Is this operator a carrier?
        if (!AlgCarriers[alg][i])
            continue;

        uint8_t opvol = uint16_t((op[1] & 63) ^ 63) * vol / 64;
        uint16_t reg = 0x40 + OpOffsets3[channum][i];
        SetOPL3(reg, (GetOPL3(reg) & 0xC0) | (opvol ^ 0x3F));
    }
}

//==================================================================================================
// Starts a tone-slide.
//==================================================================================================
void RADPlayer::GetSlideDir(int channum, CEffects *fx) {
    CChannel &chan = Channels[channum];

    int8_t speed = fx->ToneSlideSpeed;
    if (speed > 0) {
        uint8_t oct = fx->ToneSlideOct;
        uint16_t freq = fx->ToneSlideFreq;

        uint16_t oldfreq = chan.CurrFreq;
        uint8_t oldoct = chan.CurrOctave;

        if (oldoct > oct)
            speed = -speed;
        else if (oldoct == oct) {
            if (oldfreq > freq)
                speed = -speed;
            else if (oldfreq == freq)
                speed = 0;
        }
    }

    fx->ToneSlideDir = speed;
}

//==================================================================================================
// Load multiplier value into operator.
//==================================================================================================
void RADPlayer::LoadInstMultiplierOPL3(int channum, int op, uint8_t mult) {
    uint16_t reg = 0x20 + OpOffsets3[channum][op];
    SetOPL3(reg, (GetOPL3(reg) & 0xF0) | (mult & 15));
}

//==================================================================================================
// Load volume value into operator.
//==================================================================================================
void RADPlayer::LoadInstVolumeOPL3(int channum, int op, uint8_t vol) {
    uint16_t reg = 0x40 + OpOffsets3[channum][op];
    SetOPL3(reg, (GetOPL3(reg) & 0xC0) | ((vol & 0x3F) ^ 0x3F));
}

//==================================================================================================
// Load feedback value into instrument.
//==================================================================================================
void RADPlayer::LoadInstFeedbackOPL3(int channum, int which, uint8_t fb) {

    if (which == 0) {

        uint16_t reg = 0xC0 + Chn2Offsets3[channum];
        SetOPL3(reg, (GetOPL3(reg) & 0x31) | ((fb & 7) << 1));

    } else if (which == 1) {

        uint16_t reg = 0xC0 + ChanOffsets3[channum];
        SetOPL3(reg, (GetOPL3(reg) & 0x31) | ((fb & 7) << 1));
    }
}

//==================================================================================================
// This adjusts the pitch of the given channel's note.  There may also be a limiting value on the
// portamento (for tone slides).
//==================================================================================================
void RADPlayer::Portamento(uint16_t channum, CEffects *fx, int8_t amount, bool toneslide) {
    CChannel &chan = Channels[channum];

    uint16_t freq = chan.CurrFreq;
    uint8_t oct = chan.CurrOctave;

    freq += amount;

    if (freq < 0x156) {

        if (oct > 0) {
            oct--;
            freq += 0x2AE - 0x156;
        } else
            freq = 0x156;

    } else if (freq > 0x2AE) {

        if (oct < 7) {
            oct++;
            freq -= 0x2AE - 0x156;
        } else
            freq = 0x2AE;
    }

    if (toneslide) {

        if (amount >= 0) {

            if (oct > fx->ToneSlideOct || (oct == fx->ToneSlideOct && freq >= fx->ToneSlideFreq)) {
                freq = fx->ToneSlideFreq;
                oct = fx->ToneSlideOct;
            }

        } else {

            if (oct < fx->ToneSlideOct || (oct == fx->ToneSlideOct && freq <= fx->ToneSlideFreq)) {
                freq = fx->ToneSlideFreq;
                oct = fx->ToneSlideOct;
            }
        }
    }

    chan.CurrFreq = freq;
    chan.CurrOctave = oct;

    // Apply detunes
    uint16_t frq2 = freq - chan.DetuneB;
    freq += chan.DetuneA;

    // Write value back to OPL3
    uint16_t chan_offset = Chn2Offsets3[channum];
    SetOPL3(0xA0 + chan_offset, freq & 0xFF);
    SetOPL3(0xB0 + chan_offset, (freq >> 8 & 3) | oct << 2 | (GetOPL3(0xB0 + chan_offset) & 0xE0));

    chan_offset = ChanOffsets3[channum];
    SetOPL3(0xA0 + chan_offset, frq2 & 0xFF);
    SetOPL3(0xB0 + chan_offset, (frq2 >> 8 & 3) | oct << 2 | (GetOPL3(0xB0 + chan_offset) & 0xE0));
}

//==================================================================================================
// Transpose the note returned by UnpackNote().
// Note: due to RAD's wonky legacy middle C is octave 3 note number 12.
//==================================================================================================
void RADPlayer::Transpose(int8_t note, int8_t octave) {

    if (NoteNum >= 1 && NoteNum <= 12) {

        int8_t toct = octave - 3;
        if (toct != 0) {
            OctaveNum += toct;
            if (OctaveNum < 0)
                OctaveNum = 0;
            else if (OctaveNum > 7)
                OctaveNum = 7;
        }

        int8_t tnot = note - 12;
        if (tnot != 0) {
            NoteNum += tnot;
            if (NoteNum < 1) {
                NoteNum += 12;
                if (OctaveNum > 0)
                    OctaveNum--;
                else
                    NoteNum = 1;
            }
        }
    }
}

//==================================================================================================
// Compute total time of tune if it didn't repeat.  Note, this stops the tune so should only be done
// prior to initial playback.
//==================================================================================================
#if RAD_DETECT_REPEATS
static void RADPlayerDummyOPL3(void *arg, uint16_t reg, uint8_t data) {}
//--------------------------------------------------------------------------------------------------
uint32_t RADPlayer::ComputeTotalTime() {

    Stop();
    void (*old_opl3)(void *, uint16_t, uint8_t) = OPL3;
    OPL3 = RADPlayerDummyOPL3;

    while (!Update())
        ;
    uint32_t total = PlayTime;

    Stop();
    OPL3 = old_opl3;

    return total / Hertz;
}
#endif

// BEGIN MEGAZEUX ADDITIONS

//==================================================================================================
// Initialise a RAD v1 tune for playback.  This assumes the tune data is valid and does minimal data
// checking. -Lachesis
//==================================================================================================
void RADPlayer::Init10(const void *tune) {
    uint8_t *pos = (uint8_t *)tune + 0x11;

    uint8_t flags = *(pos++);
    Speed = flags & 0x1F;
    Hertz = 50;

    // Slow timer tune?  Return an approximate hz
    if (flags & 0x40)
        Hertz = 18;

    // Skip any description (only present if flag is set)
    if (flags & 0x80)
        while (*(pos++)) {
        }

    // Unpack the instruments
    while (true) {
        // Instrument number, 0 indicates end of list
        uint8_t inst_num = *(pos++);
        if (inst_num == 0)
            break;

        CInstrument &inst = Instruments[inst_num - 1];

        uint8_t c_flags = pos[0];
        uint8_t m_flags = pos[1];
        uint8_t c_ksl_volume = pos[2];
        uint8_t m_ksl_volume = pos[3];
        uint8_t c_attack_decay = pos[4];
        uint8_t m_attack_decay = pos[5];
        uint8_t c_sustain_release = pos[6];
        uint8_t m_sustain_release = pos[7];
        uint8_t feedback_algorithm = pos[8];
        uint8_t c_waveform = pos[9];
        uint8_t m_waveform = pos[10];
        pos += 11;

        inst.Algorithm = (feedback_algorithm & 1);
        inst.Panning[0] = 0;
        inst.Panning[1] = 0;

        inst.Feedback[0] = (feedback_algorithm & 0x0E) >> 1;
        inst.Feedback[1] = (feedback_algorithm & 0x0E) >> 1;

        inst.Volume = (c_ksl_volume & 0x3F) ^ 0x3F;
        inst.Detune = 0;
        inst.RiffSpeed = 6;
        inst.Riff = 0;

        uint8_t *op0 = inst.Operators[0];
        uint8_t *op1 = inst.Operators[1];
        uint8_t *op2 = inst.Operators[2];
        uint8_t *op3 = inst.Operators[3];

        op0[0] = c_flags;
        op0[1] = c_ksl_volume;
        op0[2] = c_attack_decay;
        op0[3] = c_sustain_release;
        op0[4] = c_waveform;

        op1[0] = m_flags;
        op1[1] = m_ksl_volume;
        op1[2] = m_attack_decay;
        op1[3] = m_sustain_release;
        op1[4] = m_waveform;

        memset(op2, 0, 5);
        memset(op3, 0, 5);
    }

    // Get order list
    OrderListSize = *(pos++);
    OrderList = pos;
    pos += OrderListSize;

    // Track offset table
    for (int i = 0; i < 31; i++) {
        int offset = pos[0] | (int(pos[1]) << 8);
        pos += 2;

        if (offset)
            Tracks[i] = (uint8_t *)tune + offset;
    }

    // Done parsing tune, now set up for play
    for (int i = 0; i < 512; i++)
        OPL3Regs[i] = 255;

    Stop();

    Initialised = true;
}

//==================================================================================================
// Unpacks a single RAD note (v1). -Lachesis
//==================================================================================================
bool RADPlayer::UnpackNote10(uint8_t *&pos) {
    uint8_t chanid = *(pos++);
    uint8_t b1 = *(pos++);
    uint8_t b2 = *(pos++);

    // Unpack note data
    NoteNum = b1 & 0x0F;
    OctaveNum = (b1 & 0x70) >> 4;
    InstNum = ((b1 & 0x80) >> 3) | ((b2 & 0xF0) >> 4);
    EffectNum = (b2 & 0x0F);
    Param = 0;

    // Do we have an effect?
    if (EffectNum)
        Param = *(pos++);

    return ((chanid & 0x80) != 0);
}

//==================================================================================================
// Skip through track till we reach the given line or the next higher one (v1).  Returns null if none.
// -Lachesis
//==================================================================================================
uint8_t *RADPlayer::SkipToLine10(uint8_t *trk, uint8_t linenum) {
    while (true) {
        uint8_t lineid = *trk;

        if ((lineid & 0x7F) >= linenum)
            return trk;

        if (lineid & 0x80)
            break;

        trk++;

        // Skip channel notes
        while (true) {
            uint8_t chanid = *(trk++);
            trk++;

            // Do we have an effect?
            if (*(trk++) & 0x0F)
                trk++;

            if (chanid & 0x80)
                break;
        }
    }
    return 0;
}

//==================================================================================================
// KSL is handled different between RAD v1 / DOS RAD v2.1 and Windows/Mac RAD v2.1.
// In DOS, since these are passed directly to the OPL3, KSL 1 is 3dB and KSL 2 is 1.5dB.
// With Opal, these were originally reversed, so flip the KSL bits when loading RAD v2s.
// -Lachesis
//==================================================================================================
uint8_t RADPlayer::FixRadv21KSLVolume(uint8_t val) { return ((val & 0x80) >> 1) | ((val & 0x40) << 1) | (val & 0x3F); }

//==================================================================================================
// Set the current order and line. -Lachesis
//==================================================================================================
void RADPlayer::SetTunePos(uint32_t order, uint32_t line) {
    if (line > kTrackLines)
        line = 0;

    if (order > kTracks || order > OrderListSize)
        order = 0;

    Order = order;
    Line = line;

    Track = GetTrack();

    if (line > 0)
        Track = SkipToLine(Track, Line, false);
}

//==================================================================================================
// Get the effective length of the tune in orders, i.e., the length minus any jumps at the end.
// -Lachesis
//==================================================================================================
int RADPlayer::GetTuneEffectiveLength() {
    if (LastPatternOrder >= 0)
        return LastPatternOrder + 1;

    int i;
    for (i = OrderListSize - 1; i >= 0; i--) {
        if (!(OrderList[i] & 0x80))
            break;
    }

    if (i < 0)
        return 0;

    LastPatternOrder = i;
    return i + 1;
}
