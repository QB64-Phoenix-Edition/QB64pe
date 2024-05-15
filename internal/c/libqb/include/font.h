//----------------------------------------------------------------------------------------------------------------------
// QB64-PE Font Library
// Powered by FreeType 2.4.12 (https://github.com/vinniefalco/FreeTypeAmalgam)
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

#define INVALID_FONT_HANDLE 0

// Font load options
#define FONT_LOAD_DONTBLEND 8
#define FONT_LOAD_MONOSPACE 16
#define FONT_LOAD_UNICODE 32
#define FONT_LOAD_AUTOMONO 64
// Font render options
#define FONT_RENDER_MONOCHROME 1

struct qbs;

/// @brief CP437 to UTF-16 LUT
extern uint16_t codepage437_to_unicode16[];

uint8_t *FontLoadFileToMemory(const char *file_path_name, int32_t *out_bytes);
int32_t FontLoad(const uint8_t *content_original, int32_t content_bytes, int32_t default_pixel_height, int32_t which_font, int32_t &options);
void FontFree(int32_t fh);
int32_t FontWidth(int32_t fh);
bool FontRenderTextUTF32(int32_t fh, const uint32_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y);
bool FontRenderTextASCII(int32_t fh, const uint8_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y);
int32_t FontPrintWidthUTF32(int32_t fh, const uint32_t *codepoint, int32_t codepoints);
int32_t FontPrintWidthASCII(int32_t fh, const uint8_t *codepoint, int32_t codepoints);

qbs *func__md5(qbs *text);
int32_t func__UFontHeight(int32_t qb64_fh, int32_t passed);
int32_t func__UPrintWidth(const qbs *text, int32_t utf_encoding, int32_t qb64_fh, int32_t passed);
int32_t func__ULineSpacing(int32_t qb64_fh, int32_t passed);
void sub__UPrintString(int32_t start_x, int32_t start_y, const qbs *text, int32_t max_width, int32_t utf_encoding, int32_t qb64_fh, int32_t passed);
int32_t func__UCharPos(const qbs *text, void *arr, int32_t utf_encoding, int32_t qb64_fh, int32_t passed);
