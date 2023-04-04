//----------------------------------------------------------------------------------------------------------------------
//  QB64-PE Font Library
//  Powered by FreeType 2.4.12 (https://github.com/vinniefalco/FreeTypeAmalgam)
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

int32_t FontRenderTextUTF32(int32_t i, uint32_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y,
                            int32_t *out_x_pre_increment, int32_t *out_x_post_increment);
int32_t FontLoad(uint8_t *content_original, int32_t content_bytes, int32_t default_pixel_height, int32_t which_font, int32_t options);
int32_t FontRenderTextASCII(int32_t i, uint8_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y,
                            int32_t *out_x_pre_increment, int32_t *out_x_post_increment);
int32_t FontWidth(int32_t i);
void FontFree(int32_t i);
