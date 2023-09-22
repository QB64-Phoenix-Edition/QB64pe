//----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <stdio.h>

#if defined(AUDIO_DEBUG) && AUDIO_DEBUG > 0
#    ifdef _MSC_VER
#        define AUDIO_DEBUG_PRINT(_fmt_, ...) fprintf(stderr, "DEBUG: %s:%d:%s(): " _fmt_ "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
#    else
#        define AUDIO_DEBUG_PRINT(_fmt_, _args_...) fprintf(stderr, "DEBUG: %s:%d:%s(): " _fmt_ "\n", __FILE__, __LINE__, __func__, ##_args_)
#    endif
#    define AUDIO_DEBUG_CHECK(_exp_)                                                                                                                           \
        if (!(_exp_))                                                                                                                                          \
        AUDIO_DEBUG_PRINT("Condition (%s) failed", #_exp_)
#else
#    ifdef _MSC_VER
#        define AUDIO_DEBUG_PRINT(_fmt_, ...) // Don't do anything in release builds
#    else
#        define AUDIO_DEBUG_PRINT(_fmt_, _args_...) // Don't do anything in release builds
#    endif
#    define AUDIO_DEBUG_CHECK(_exp_) // Don't do anything in release builds
#endif

// We always use 48000
#define MA_DEFAULT_SAMPLE_RATE 48000

struct qbs;
struct mem_block;

void sub_sound(double frequency, double lengthInClockTicks, double volume, double panning, int32_t waveform, int32_t passed);
void sub_beep();
void sub_play(const qbs *str);
int32_t func_play(int32_t ignore);

int32_t func__sndrate();
int32_t func__sndopen(qbs *fileName, qbs *requirements, int32_t passed);
void sub__sndclose(int32_t handle);
int32_t func__sndcopy(int32_t src_handle);
void sub__sndplay(int32_t handle);
void sub__sndplaycopy(int32_t src_handle, double volume, double x, double y, double z, int32_t passed);
void sub__sndplayfile(qbs *fileName, int32_t sync, double volume, int32_t passed);
void sub__sndpause(int32_t handle);
int32_t func__sndplaying(int32_t handle);
int32_t func__sndpaused(int32_t handle);
void sub__sndvol(int32_t handle, float volume);
void sub__sndloop(int32_t handle);
void sub__sndbal(int32_t handle, double x, double y, double z, int32_t channel, int32_t passed);
double func__sndlen(int32_t handle);
double func__sndgetpos(int32_t handle);
void sub__sndsetpos(int32_t handle, double seconds);
void sub__sndlimit(int32_t handle, double limit);
void sub__sndstop(int32_t handle);

int32_t func__sndopenraw();
void sub__sndraw(float left, float right, int32_t handle, int32_t passed);
void sub__sndrawdone(int32_t handle, int32_t passed);
double func__sndrawlen(int32_t handle, int32_t passed);

mem_block func__memsound(int32_t handle, int32_t targetChannel, int32_t passed);
int32_t func__sndnew(int32_t frames, int32_t channels, int32_t bits);

void snd_init();
void snd_un_init();
void snd_mainloop();
