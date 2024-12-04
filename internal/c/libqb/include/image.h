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

#include <algorithm>
#include <cmath>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "logging.h"

#define image_log_trace(...) \
    libqb_log_with_scope_trace(logscope::Image, __VA_ARGS__)

#define image_log_info(...) \
    libqb_log_with_scope_info(logscope::Image, __VA_ARGS__)

#define image_log_warn(...) \
    libqb_log_with_scope_warn(logscope::Image, __VA_ARGS__)

#define image_log_error(...) \
    libqb_log_with_scope_error(logscope::Image, __VA_ARGS__)

#define IMAGE_DEBUG_CHECK(_exp_) \
    do { \
        if (!(_exp_)) \
            image_log_warn("Condition (%s) failed", #_exp_); \
    } while (0)

// This is returned to the caller if something goes wrong while loading the image
#define INVALID_IMAGE_HANDLE -1

struct qbs;

int32_t func__loadimage(qbs *qbsFileName, int32_t bpp, qbs *qbsRequirements, int32_t passed);
void sub__saveimage(qbs *qbsFileName, int32_t imageHandle, qbs *qbsRequirements, int32_t passed);

static inline constexpr uint8_t image_get_bgra_red(uint32_t c) {
    return uint8_t((c >> 16) & 0xFFu);
}

static inline constexpr uint8_t image_get_bgra_green(uint32_t c) {
    return uint8_t((c >> 8) & 0xFFu);
}

static inline constexpr uint8_t image_get_bgra_blue(uint32_t c) {
    return uint8_t(c & 0xFFu);
}

static inline constexpr uint8_t image_get_bgra_alpha(uint32_t c) {
    return uint8_t(c >> 24);
}

static inline constexpr uint32_t image_get_bgra_bgr(uint32_t c) {
    return c & 0xFFFFFFu;
}

static inline constexpr uint32_t image_set_bgra_alpha(uint32_t c, uint8_t a = 0xFFu) {
    return (c & 0xFFFFFFu) | (uint32_t(a) << 24);
}

static inline constexpr uint32_t image_make_bgra(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFFu) {
    return uint32_t(b) | (uint32_t(g) << 8) | (uint32_t(r) << 16) | (uint32_t(a) << 24);
}

static inline constexpr int image_scale_5bits_to_8bits(int v) {
    return (v << 3) | (v >> 2);
}

static inline constexpr int image_scale_6bits_to_8bits(int v) {
    return (v << 2) | (v >> 4);
}

static inline constexpr uint32_t image_swap_red_blue(uint32_t clr) {
    return ((clr & 0xFF00FF00u) | ((clr & 0x00FF0000u) >> 16) | ((clr & 0x000000FFu) << 16));
}

static inline constexpr uint8_t image_clamp_color_component(int n) {
    return uint8_t(std::clamp(n, 0, 255));
}

static inline float image_calculate_rgb_distance(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
    auto delta_r = float(r2) - float(r1);
    auto delta_g = float(g2) - float(g1);
    auto delta_b = float(b2) - float(b1);

    return sqrtf(delta_r * delta_r + delta_g * delta_g + delta_b * delta_b);
}

static inline uint32_t image_get_color_delta(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
    return uint32_t(::abs(long(r1) - long(r2)) + ::abs(long(g1) - long(g2)) + ::abs(long(b1) - long(b2)));
}

static inline constexpr uint32_t func__rgb32(int32_t r, int32_t g, int32_t b, int32_t a) {
    return (image_clamp_color_component(a) << 24) | (image_clamp_color_component(r) << 16) | (image_clamp_color_component(g) << 8) |
           image_clamp_color_component(b);
}

static inline constexpr uint32_t func__rgb32(int32_t r, int32_t g, int32_t b) {
    return 0xFF000000u | (image_clamp_color_component(r) << 16) | (image_clamp_color_component(g) << 8) | image_clamp_color_component(b);
}

static inline constexpr uint32_t func__rgb32(int32_t i, int32_t a) {
    i = image_clamp_color_component(i);
    return (image_clamp_color_component(a) << 24) | (uint32_t(i) << 16) | (uint32_t(i) << 8) | uint32_t(i);
}

static inline constexpr uint32_t func__rgb32(int32_t i) {
    i = image_clamp_color_component(i);
    return 0xFF000000u | (uint32_t(i) << 16) | (uint32_t(i) << 8) | uint32_t(i);
}

static inline constexpr uint32_t func__rgba32(int32_t r, int32_t g, int32_t b, int32_t a) {
    return (image_clamp_color_component(a) << 24) | (image_clamp_color_component(r) << 16) | (image_clamp_color_component(g) << 8) |
           image_clamp_color_component(b);
}

static inline constexpr int32_t func__alpha32(uint32_t col) {
    return col >> 24;
}

static inline constexpr int32_t func__red32(uint32_t col) {
    return (col >> 16) & 0xFF;
}

static inline constexpr int32_t func__green32(uint32_t col) {
    return (col >> 8) & 0xFF;
}

static inline constexpr int32_t func__blue32(uint32_t col) {
    return col & 0xFF;
}
