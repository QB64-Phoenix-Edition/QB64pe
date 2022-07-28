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

//-----------------------------------------------------------------------------------------------------
// HEADER FILES
//-----------------------------------------------------------------------------------------------------
// Enable Ogg Vorbis decoding
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
// PulseAudio has serious stuttering issues in ChromeOS Linux (Crostini) and possibly others
// This may be due to this - https://github.com/mackron/miniaudio/issues/427
// And https://wiki.archlinux.org/title/PulseAudio/Troubleshooting#Glitches,_skips_or_crackling
// We'll have to look at this closely later. If this is fixed, then remove this define from here & audio.cpp
#define MA_NO_PULSEAUDIO
// The main miniaudio header
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
// The stb_vorbis implementation must come after the implementation of miniaudio
#undef STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
