//----------------------------------------------------------------------------------------------------
//    ___  ___ ___ ___     _          _ _       ___           _
//   / _ \| _ ) _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                      |___/
//
//	QBPE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//	Copyright (c) 2022 Samuel Gomes
//	https://github.com/a740g
//
//-----------------------------------------------------------------------------------------------------

#pragma once

//-----------------------------------------------------------------------------------------------------
// HEADER FILES
//-----------------------------------------------------------------------------------------------------
#include <stdint.h>
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//-----------------------------------------------------------------------------------------------------
struct qbs;
struct mem_block;
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------------------------------
void sub_sound(double frequency, double lengthInClockTicks);
void sub_beep();
void sub_play(qbs *str);
int32_t func_play(int32_t ignore);

int32_t func__sndrate();
int32_t func__sndopen(qbs *fileName, qbs *requirements, int32_t passed);
void sub__sndclose(int32_t handle);
int32_t func__sndcopy(int32_t src_handle);
void sub__sndplay(int32_t handle);
void sub__sndplaycopy(int32_t src_handle, double volume, int32_t passed);
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

mem_block func__memsound(int32_t handle, int32_t targetChannel);

void snd_init();
void snd_un_init();
void snd_mainloop();
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
