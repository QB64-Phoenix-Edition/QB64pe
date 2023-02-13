//----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___   ___                       _    _ _
//   / _ \| _ ) / /| | || _ \ __| |_ _|_ __  __ _ __ _ ___  | |  (_) |__ _ _ __ _ _ _ _  _
//  | (_) | _ \/ _ \_  _|  _/ _|   | || '  \/ _` / _` / -_) | |__| | '_ \ '_/ _` | '_| || |
//   \__\_\___/\___/ |_||_| |___| |___|_|_|_\__,_\__, \___| |____|_|_.__/_| \__,_|_|  \_, |
//                                               |___/                                |__/
//
//  QB64-PE Image Library
//  Powered by stb_image (https://github.com/nothings/stb) & dr_pcx (https://github.com/mackron/dr_pcx)
//
//  Copyright (c) 2022 Samuel Gomes
//  https://github.com/a740g
//
//-----------------------------------------------------------------------------------------------------

#pragma once

//-----------------------------------------------------------------------------------------------------
// HEADER FILES
//-----------------------------------------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------------------------------
#if defined(IMAGE_DEBUG) && IMAGE_DEBUG > 0
#    ifdef _MSC_VER
#        define IMAGE_DEBUG_PRINT(_fmt_, ...) fprintf(stderr, "DEBUG: %s:%d:%s(): " _fmt_ "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
#    else
#        define IMAGE_DEBUG_PRINT(_fmt_, _args_...) fprintf(stderr, "DEBUG: %s:%d:%s(): " _fmt_ "\n", __FILE__, __LINE__, __func__, ##_args_)
#    endif
#    define IMAGE_DEBUG_CHECK(_exp_)                                                                                                                           \
        if (!(_exp_))                                                                                                                                          \
        IMAGE_DEBUG_PRINT("Condition (%s) failed", #_exp_)
#else
#    ifdef _MSC_VER
#        define IMAGE_DEBUG_PRINT(_fmt_, ...) // Don't do anything in release builds
#    else
#        define IMAGE_DEBUG_PRINT(_fmt_, _args_...) // Don't do anything in release builds
#    endif
#    define IMAGE_DEBUG_CHECK(_exp_) // Don't do anything in release builds
#endif

// The byte ordering here are straight from libqb.cpp. So, if libqb.cpp is wrong, then we are wrong! ;)
#define IMAGE_GET_BGRA_RED(c) ((uint8_t)((uint32_t)(c) >> 16 & 0xFF))
#define IMAGE_GET_BGRA_GREEN(c) ((uint8_t)((uint32_t)(c) >> 8 & 0xFF))
#define IMAGE_GET_BGRA_BLUE(c) ((uint8_t)((uint32_t)(c) & 0xFF))
#define IMAGE_GET_BGRA_ALPHA(c) ((uint8_t)(uint32_t(c) >> 24))
#define IMAGE_MAKE_BGRA(r, g, b, a)                                                                                                                            \
    ((uint32_t)(((uint8_t)(b) | ((uint16_t)((uint8_t)(g)) << 8)) | ((uint32_t)((uint8_t)(r)) << 16) | ((uint32_t)((uint8_t)(a)) << 24)))
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//-----------------------------------------------------------------------------------------------------
struct qbs;
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------------------------------
int32_t func__loadimage(qbs *fileName, int32_t bpp, qbs *requirements, int32_t passed);
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
