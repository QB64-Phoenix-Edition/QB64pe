//----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//  hvl_replay.c + hvl_replay.h is a modified AHX & HVL replayer from
//  https://github.com/pete-gordon/hivelytracker (BSD 3-Clause License)
//
//  Some modifications were required to make this suitable for QB64-PE. The points below lists the
//  changes that were made. We may need to redo these if we need to reconstruct these files from
//  newer versions of the HivelyTracker replayer code.
//
//  1. Use the replayer code in 'Replayer_Windows'
//  2. Comment out all printfs from hvl_replay.c and hvl_tables.c
//  3. hvl_tables.c and hvl_replay.c can be amalgamated like in 'hvl2wav'
//  4. hvl_tables.h and hvl_replay.h can be amalgamated iike in 'hvl2wav'
//  5. Add 'extern "C"' block around the function declarations in hvl_replay.h
//  6. Change 'typedef char int8' to 'typedef signed char int8' in hvl_replay.h
//
//-----------------------------------------------------------------------------------------------------

#pragma once

typedef signed char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef double float64;
typedef char TEXT;
typedef int BOOL;

#define TRUE 1
#define FALSE 0

// Woohoo!
#define MAX_CHANNELS 16

// Some handy constants. Thanks eightbitbubsy.
#define AMIGA_PAL_XTAL 28375160
#define AMIGA_NTSC_XTAL 28636360
#define AMIGA_CPU_PAL_CLK ((AMIGA_PAL_XTAL / 4))
#define AMIGA_CPU_NTSC_CLK ((AMIGA_NTSC_XTAL / 4))
#define AMIGA_CIA_PAL_CLK ((AMIGA_CPU_PAL_CLK / 10))
#define AMIGA_CIA_NTSC_CLK ((AMIGA_CPU_NTSC_CLK / 10))
#define AMIGA_PAULA_PAL_CLK ((AMIGA_CPU_PAL_CLK / 2))
#define AMIGA_PAULA_NTSC_CLK ((AMIGA_CPU_NTSC_CLK / 2))

#define Period2Freq(period) ((AMIGA_PAULA_PAL_CLK * 65536.f) / (period))

#ifdef __cplusplus
extern "C" {
#endif

struct hvl_envelope {
    int16 aFrames, aVolume;
    int16 dFrames, dVolume;
    int16 sFrames;
    int16 rFrames, rVolume;
    int16 pad;
};

struct hvl_plsentry {
    uint8 ple_Note;
    uint8 ple_Waveform;
    int16 ple_Fixed;
    int8 ple_FX[2];
    int8 ple_FXParam[2];
};

struct hvl_plist {
    int16 pls_Speed;
    int16 pls_Length;
    struct hvl_plsentry *pls_Entries;
};

struct hvl_instrument {
    TEXT ins_Name[128];
    uint8 ins_Volume;
    uint8 ins_WaveLength;
    uint8 ins_FilterLowerLimit;
    uint8 ins_FilterUpperLimit;
    uint8 ins_FilterSpeed;
    uint8 ins_SquareLowerLimit;
    uint8 ins_SquareUpperLimit;
    uint8 ins_SquareSpeed;
    uint8 ins_VibratoDelay;
    uint8 ins_VibratoSpeed;
    uint8 ins_VibratoDepth;
    uint8 ins_HardCutRelease;
    uint8 ins_HardCutReleaseFrames;
    struct hvl_envelope ins_Envelope;
    struct hvl_plist ins_PList;
};

struct hvl_position {
    uint8 pos_Track[MAX_CHANNELS];
    int8 pos_Transpose[MAX_CHANNELS];
};

struct hvl_step {
    uint8 stp_Note;
    uint8 stp_Instrument;
    uint8 stp_FX;
    uint8 stp_FXParam;
    uint8 stp_FXb;
    uint8 stp_FXbParam;
};

struct hvl_voice {
    int16 vc_Track;
    int16 vc_NextTrack;
    int16 vc_Transpose;
    int16 vc_NextTranspose;
    int16 vc_OverrideTranspose; // 1.5
    int32 vc_ADSRVolume;
    struct hvl_envelope vc_ADSR;
    struct hvl_instrument *vc_Instrument;
    uint32 vc_SamplePos;
    uint32 vc_Delta;
    uint16 vc_InstrPeriod;
    uint16 vc_TrackPeriod;
    uint16 vc_VibratoPeriod;
    uint16 vc_WaveLength;
    int16 vc_NoteMaxVolume;
    uint16 vc_PerfSubVolume;
    uint8 vc_NewWaveform;
    uint8 vc_Waveform;
    uint8 vc_PlantPeriod;
    uint8 vc_VoiceVolume;
    uint8 vc_PlantSquare;
    uint8 vc_IgnoreSquare;
    uint8 vc_FixedNote;
    int16 vc_VolumeSlideUp;
    int16 vc_VolumeSlideDown;
    int16 vc_HardCut;
    uint8 vc_HardCutRelease;
    int16 vc_HardCutReleaseF;
    uint8 vc_PeriodSlideOn;
    int16 vc_PeriodSlideSpeed;
    int16 vc_PeriodSlidePeriod;
    int16 vc_PeriodSlideLimit;
    int16 vc_PeriodSlideWithLimit;
    int16 vc_PeriodPerfSlideSpeed;
    int16 vc_PeriodPerfSlidePeriod;
    uint8 vc_PeriodPerfSlideOn;
    int16 vc_VibratoDelay;
    int16 vc_VibratoSpeed;
    int16 vc_VibratoCurrent;
    int16 vc_VibratoDepth;
    int16 vc_SquareOn;
    int16 vc_SquareInit;
    int16 vc_SquareWait;
    int16 vc_SquareLowerLimit;
    int16 vc_SquareUpperLimit;
    int16 vc_SquarePos;
    int16 vc_SquareSign;
    int16 vc_SquareSlidingIn;
    int16 vc_SquareReverse;
    uint8 vc_FilterOn;
    uint8 vc_FilterInit;
    int16 vc_FilterWait;
    int16 vc_FilterSpeed;
    int16 vc_FilterUpperLimit;
    int16 vc_FilterLowerLimit;
    int16 vc_FilterPos;
    int16 vc_FilterSign;
    int16 vc_FilterSlidingIn;
    int16 vc_IgnoreFilter;
    int16 vc_PerfCurrent;
    int16 vc_PerfSpeed;
    int16 vc_PerfWait;
    struct hvl_plist *vc_PerfList;
    const int8 *vc_AudioPointer;
    const int8 *vc_AudioSource;
    uint8 vc_NoteDelayOn;
    uint8 vc_NoteCutOn;
    int16 vc_NoteDelayWait;
    int16 vc_NoteCutWait;
    int16 vc_AudioPeriod;
    int16 vc_AudioVolume;
    int32 vc_WNRandom;
    const int8 *vc_MixSource;
    int8 vc_SquareTempBuffer[0x80];
    int8 vc_VoiceBuffer[0x282 * 4];
    uint8 vc_VoiceNum;
    uint8 vc_TrackMasterVolume;
    uint8 vc_TrackOn;
    int16 vc_VoicePeriod;
    uint32 vc_Pan;
    uint32 vc_SetPan; // New for 1.4
    uint32 vc_PanMultLeft;
    uint32 vc_PanMultRight;
    uint32 vc_RingSamplePos;
    uint32 vc_RingDelta;
    const int8 *vc_RingMixSource;
    uint8 vc_RingPlantPeriod;
    int16 vc_RingInstrPeriod;
    int16 vc_RingBasePeriod;
    int16 vc_RingAudioPeriod;
    const int8 *vc_RingAudioSource;
    uint8 vc_RingNewWaveform;
    uint8 vc_RingWaveform;
    uint8 vc_RingFixedPeriod;
    int8 vc_RingVoiceBuffer[0x282 * 4];
};

struct hvl_tune {
    TEXT ht_Name[128];
    uint16 ht_SongNum;
    uint32 ht_Frequency;
    float64 ht_FreqF;
    const int8 *ht_WaveformTab[MAX_CHANNELS];
    uint16 ht_Restart;
    uint16 ht_PositionNr;
    uint8 ht_SpeedMultiplier;
    uint8 ht_TrackLength;
    uint8 ht_TrackNr;
    uint8 ht_InstrumentNr;
    uint8 ht_SubsongNr;
    uint16 ht_PosJump;
    uint32 ht_PlayingTime;
    int16 ht_Tempo;
    int16 ht_PosNr;
    uint16 ht_StepWaitFrames;
    int16 ht_NoteNr;
    uint16 ht_PosJumpNote;
    uint8 ht_GetNewPosition;
    uint8 ht_PatternBreak;
    uint8 ht_SongEndReached;
    uint8 ht_Stereo;
    uint16 *ht_Subsongs;
    uint16 ht_Channels;
    struct hvl_position *ht_Positions;
    struct hvl_step ht_Tracks[256][64];
    struct hvl_instrument *ht_Instruments;
    struct hvl_voice ht_Voices[MAX_CHANNELS];
    int32 ht_defstereo;
    int32 ht_defpanleft;
    int32 ht_defpanright;
    int32 ht_mixgain;
    uint8 ht_Version;
};

void hvl_DecodeFrame(struct hvl_tune *ht, int8 *buf1, int8 *buf2, int32 bufmod);
void hvl_InitReplayer(void);
BOOL hvl_InitSubsong(struct hvl_tune *ht, uint32 nr);
struct hvl_tune *hvl_LoadTune(const TEXT *name, uint32 freq, uint32 defstereo);
struct hvl_tune *hvl_ParseTune(const uint8 *buf, uint32 buflen, uint32 freq, uint32 defstereo);
void hvl_FreeTune(struct hvl_tune *ht);

#ifdef __cplusplus
}
#endif
