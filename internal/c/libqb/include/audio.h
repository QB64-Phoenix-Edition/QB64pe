//----------------------------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <stdio.h>

#if defined(AUDIO_DEBUG) && AUDIO_DEBUG > 0
#    define AUDIO_DEBUG_FILENAME (std::strrchr(__FILE__, '/') ? std::strrchr(__FILE__, '/') + 1 : __FILE__)
#    define AUDIO_DEBUG_PRINT(_fmt_, _args_...)                                                                                                                \
        fprintf(stderr, "\e[1;37mDEBUG: %s:%d:%s: \e[1;33m" _fmt_ "\e[1;37m\n", AUDIO_DEBUG_FILENAME, __LINE__, __PRETTY_FUNCTION__, ##_args_)
#    define AUDIO_DEBUG_CHECK(_exp_)                                                                                                                           \
        if (!(_exp_))                                                                                                                                          \
        AUDIO_DEBUG_PRINT("\e[0;31mCondition (%s) failed", #_exp_)
#else
#    define AUDIO_DEBUG_PRINT(_fmt_, _args_...) // Don't do anything in release builds
#    define AUDIO_DEBUG_CHECK(_exp_)            // Don't do anything in release builds
#endif

struct qbs;
struct mem_block;

void sub_beep();
double func_play(uint32_t voice, int32_t passed);
void sub_play(const qbs *str1, const qbs *str2, const qbs *str3, const qbs *str4, int32_t passed);
void sub_sound(float frequency, float lengthInClockTicks, float volume, float panPosition, int32_t waveform, float waveformParam, uint32_t voice,
               int32_t option, int32_t passed);
void sub__wave(uint32_t voice, void *waveDefinition, uint32_t frameCount, int32_t passed);

int32_t func__sndrate();
int32_t func__sndopen(qbs *qbsFileName, qbs *qbsRequirements, int32_t passed);
void sub__sndclose(int32_t handle);
int32_t func__sndcopy(int32_t src_handle);
void sub__sndplay(int32_t handle);
void sub__sndplaycopy(int32_t src_handle, float volume, float x, float y, float z, int32_t passed);
void sub__sndplayfile(qbs *fileName, int32_t sync, float volume, int32_t passed);
void sub__sndpause(int32_t handle);
int32_t func__sndplaying(int32_t handle);
int32_t func__sndpaused(int32_t handle);
void sub__sndvol(int32_t handle, float volume);
void sub__sndloop(int32_t handle);
void sub__sndbal(int32_t handle, float x, float y, float z, int32_t channel, int32_t passed);
double func__sndlen(int32_t handle);
double func__sndgetpos(int32_t handle);
void sub__sndsetpos(int32_t handle, double seconds);
void sub__sndlimit(int32_t handle, double limit);
void sub__sndstop(int32_t handle);

int32_t func__sndopenraw();
void sub__sndraw(float left, float right, int32_t handle, int32_t passed);
void sub__sndrawbatch(void *sampleFrameArray, int32_t channels, int32_t handle, uint32_t frameCount, int32_t passed);

static inline void sub__sndrawdone(int32_t handle, int32_t passed) {
    // Dummy function that does nothing
    (void)handle;
    (void)passed;
}

double func__sndrawlen(int32_t handle, int32_t passed);

mem_block func__memsound(int32_t handle, int32_t targetChannel, int32_t passed);
int32_t func__sndnew(uint32_t frames, int32_t channels, int32_t bits);
void sub__midisoundbank(qbs *qbsFileName, qbs *qbsRequirements, int32_t passed);

void snd_init();
void snd_un_init();
void snd_mainloop();
