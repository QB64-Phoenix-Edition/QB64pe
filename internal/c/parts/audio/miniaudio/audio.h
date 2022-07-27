//----------------------------------------------------------------------------------------------------
//    ___  ___ ___ ___     _          _ _       ___           _
//   / _ \| _ ) _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                      |___/
//
//	QBPE Audio engine powered by miniaudio (https://miniaud.io/)
//
//	Copyright (c) 2022 Samuel Gomes
//	https://github.com/a740g
//
//-----------------------------------------------------------------------------------------------------

#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

//-----------------------------------------------------------------------------------------------------
// HEADER FILES
//-----------------------------------------------------------------------------------------------------
#include "libqb-common.h"
// Although Matt says we should not be doing this, this has worked out to be ok so far
// We need 'qbs' and also the 'mem' stuff from here
// I am not using 'list' anymore and have migrated the code to use C++ vectors instead
// We'll likely keep the 'include' this way because I do not want to duplicate stuff and cause issues
// For now, we'll wait for Matt until he sorts out things to smaller and logical files
#include "../../../libqb.h"
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------------------------------
void sub_sound(double frequency, double lengthInClockTicks);
void sub_beep();
void sub_play(qbs *str);

int32 func__sndrate();
int32 func__sndopen(qbs *fileName, qbs *requirements, int32 passed);
void sub__sndclose(int32 handle);
int32 func__sndcopy(int32 src_handle);
void sub__sndplay(int32 handle);
void sub__sndplaycopy(int32 src_handle, double volume, int32 passed);
void sub__sndplayfile(qbs *fileName, int32 sync, double volume, int32 passed);
void sub__sndpause(int32 handle);
int32 func__sndplaying(int32 handle);
int32 func__sndpaused(int32 handle);
void sub__sndvol(int32 handle, float volume);
void sub__sndloop(int32 handle);
void sub__sndbal(int32 handle, double x, double y, double z, int32 channel, int32 passed);
double func__sndlen(int32 handle);
double func__sndgetpos(int32 handle);
void sub__sndsetpos(int32 handle, double seconds);
void sub__sndlimit(int32 handle, double limit);
void sub__sndstop(int32 handle);

int32 func__sndopenraw();
void sub__sndraw(float left, float right, int32 handle, int32 passed);
void sub__sndrawdone(int32 handle, int32 passed);
double func__sndrawlen(int32 handle, int32 passed);

mem_block func__memsound(int32 handle, int32 targetChannel);
int32 func__newsound(int32 frames, int32 rate, int32 channels, int32 bits, int32 passed);

void snd_init();
void snd_un_init();
void snd_mainloop();
//-----------------------------------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------------------------------
