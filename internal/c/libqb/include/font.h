//----------------------------------------------------------------------------------------------------------------------
// QB64-PE Font Library
// Powered by stb_truetype (https://github.com/nothings/stb)
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <stdio.h>

#if defined(FONT_DEBUG) && FONT_DEBUG > 0
#    ifdef _MSC_VER
#        define FONT_DEBUG_PRINT(_fmt_, ...) fprintf(stderr, "DEBUG: %s:%d:%s(): " _fmt_ "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
#    else
#        define FONT_DEBUG_PRINT(_fmt_, _args_...) fprintf(stderr, "DEBUG: %s:%d:%s(): " _fmt_ "\n", __FILE__, __LINE__, __func__, ##_args_)
#    endif
#    define FONT_DEBUG_CHECK(_exp_)                                                                                                                            \
        if (!(_exp_))                                                                                                                                          \
        FONT_DEBUG_PRINT("Condition (%s) failed", #_exp_)
#else
#    ifdef _MSC_VER
#        define FONT_DEBUG_PRINT(_fmt_, ...) // Don't do anything in release builds
#    else
#        define FONT_DEBUG_PRINT(_fmt_, _args_...) // Don't do anything in release builds
#    endif
#    define FONT_DEBUG_CHECK(_exp_) // Don't do anything in release builds
#endif

extern uint16_t codepage437_to_unicode16[]; // ASCII to UTF-16 LUT

int32_t FontLoad(uint8_t *content_original, int32_t content_bytes, int32_t default_pixel_height, int32_t which_font, int32_t options);
void FontFree(int32_t fh);
int32_t FontWidth(int32_t fh);
int32_t FontRenderTextUTF32(int32_t fh, uint32_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y,
                            int32_t *out_x_pre_increment, int32_t *out_x_post_increment);
int32_t FontRenderTextASCII(int32_t fh, uint8_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y,
                            int32_t *out_x_pre_increment, int32_t *out_x_post_increment);
int32_t FontPrintWidthASCII(int32_t fh, uint8_t *codepoint, int32_t codepoints);