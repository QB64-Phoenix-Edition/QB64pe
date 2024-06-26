//-----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___   ___                       _    _ _
//   / _ \| _ ) / /| | || _ \ __| |_ _|_ __  __ _ __ _ ___  | |  (_) |__ _ _ __ _ _ _ _  _
//  | (_) | _ \/ _ \_  _|  _/ _|   | || '  \/ _` / _` / -_) | |__| | '_ \ '_/ _` | '_| || |
//   \__\_\___/\___/ |_||_| |___| |___|_|_|_\__,_\__, \___| |____|_|_.__/_| \__,_|_|  \_, |
//                                               |___/                                |__/
//
//  Powered by:
//      stb_image & stb_image_write (https://github.com/nothings/stb)
//      dr_pcx (https://github.com/mackron/dr_pcx)
//      nanosvg (https://github.com/memononen/nanosvg)
//      qoi (https://qoiformat.org)
//      pixelscalers (https://github.com/janert/pixelscalers)
//      mmpx (https://github.com/ITotalJustice/mmpx)
//
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <cmath>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

// This is returned to the caller if something goes wrong while loading the image
#define INVALID_IMAGE_HANDLE -1

struct qbs;

int32_t func__loadimage(qbs *qbsFileName, int32_t bpp, qbs *qbsRequirements, int32_t passed);
void sub__saveimage(qbs *qbsFileName, int32_t imageHandle, qbs *qbsRequirements, int32_t passed);

static inline constexpr uint8_t image_get_bgra_red(const uint32_t c) { return (uint8_t)((c >> 16) & 0xFFu); }

static inline constexpr uint8_t image_get_bgra_green(const uint32_t c) { return (uint8_t)((c >> 8) & 0xFFu); }

static inline constexpr uint8_t image_get_bgra_blue(const uint32_t c) { return (uint8_t)(c & 0xFFu); }

static inline constexpr uint8_t image_get_bgra_alpha(const uint32_t c) { return (uint8_t)(c >> 24); }

static inline constexpr uint32_t image_get_bgra_bgr(const uint32_t c) { return (uint32_t)(c & 0xFFFFFFu); }

static inline constexpr uint32_t image_make_bgra(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) {
    return (uint32_t)(b) | ((uint32_t)(g) << 8) | ((uint32_t)(r) << 16) | ((uint32_t)(a) << 24);
}

static inline constexpr int image_scale_5bits_to_8bits(const int v) { return (v << 3) | (v >> 2); }

static inline constexpr int image_scale_6bits_to_8bits(const int v) { return (v << 2) | (v >> 4); }

static inline constexpr uint32_t image_swap_red_blue(const uint32_t clr) {
    return ((clr & 0xFF00FF00u) | ((clr & 0x00FF0000u) >> 16) | ((clr & 0x000000FFu) << 16));
}

static inline constexpr uint8_t image_clamp_color_component(const int n) { return n < 0 ? 0 : n > 255 ? 255 : n; }

static inline float image_calculate_rgb_distance(const uint8_t r1, const uint8_t g1, const uint8_t b1, const uint8_t r2, const uint8_t g2, const uint8_t b2) {
    auto delta_r = (float)r2 - (float)r1;
    auto delta_g = (float)g2 - (float)g1;
    auto delta_b = (float)b2 - (float)b1;

    return sqrtf(delta_r * delta_r + delta_g * delta_g + delta_b * delta_b);
}

static inline uint32_t image_get_color_delta(const uint8_t r1, const uint8_t g1, const uint8_t b1, const uint8_t r2, const uint8_t g2, const uint8_t b2) {
    return uint32_t(::abs(long(r1) - long(r2)) + ::abs(long(g1) - long(g2)) + ::abs(long(b1) - long(b2)));
}

// Working with 32bit colors:
static inline constexpr uint32_t func__rgb32(int32_t r, int32_t g, int32_t b, int32_t a) {
    if (r < 0)
        r = 0;
    if (r > 255)
        r = 255;
    if (g < 0)
        g = 0;
    if (g > 255)
        g = 255;
    if (b < 0)
        b = 0;
    if (b > 255)
        b = 255;
    if (a < 0)
        a = 0;
    if (a > 255)
        a = 255;
    return (a << 24) + (r << 16) + (g << 8) + b;
}

static inline constexpr uint32_t func__rgb32(int32_t r, int32_t g, int32_t b) {
    if (r < 0)
        r = 0;
    if (r > 255)
        r = 255;
    if (g < 0)
        g = 0;
    if (g > 255)
        g = 255;
    if (b < 0)
        b = 0;
    if (b > 255)
        b = 255;
    return (r << 16) + (g << 8) + b | 0xFF000000;
}

static inline constexpr uint32_t func__rgb32(int32_t i, int32_t a) {
    if (i < 0)
        i = 0;
    if (i > 255)
        i = 255;
    if (a < 0)
        a = 0;
    if (a > 255)
        a = 255;
    return (a << 24) + (i << 16) + (i << 8) + i;
}

static inline constexpr uint32_t func__rgb32(int32_t i) {
    if (i < 0)
        i = 0;
    if (i > 255)
        i = 255;
    return (i << 16) + (i << 8) + i | 0xFF000000;
}

static inline constexpr uint32_t func__rgba32(int32_t r, int32_t g, int32_t b, int32_t a) {
    if (r < 0)
        r = 0;
    if (r > 255)
        r = 255;
    if (g < 0)
        g = 0;
    if (g > 255)
        g = 255;
    if (b < 0)
        b = 0;
    if (b > 255)
        b = 255;
    if (a < 0)
        a = 0;
    if (a > 255)
        a = 255;
    return (a << 24) + (r << 16) + (g << 8) + b;
}

static inline constexpr int32_t func__alpha32(uint32_t col) { return col >> 24; }

static inline constexpr int32_t func__red32(uint32_t col) { return col >> 16 & 0xFF; }

static inline constexpr int32_t func__green32(uint32_t col) { return col >> 8 & 0xFF; }

static inline constexpr int32_t func__blue32(uint32_t col) { return col & 0xFF; }
