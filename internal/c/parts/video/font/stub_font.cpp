// Font stubs

#include "font.h"

int32_t FontRenderTextUTF32(int32_t i, uint32_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y,
                            int32_t *out_x_pre_increment, int32_t *out_x_post_increment) {
    (void)i;
    (void)codepoint;
    (void)codepoints;
    (void)options;
    (void)out_data;
    (void)out_x;
    (void)out_y;
    (void)out_x_pre_increment;
    (void)out_x_post_increment;
    return 0;
}

int32_t FontLoad(uint8_t *content_original, int32_t content_bytes, int32_t default_pixel_height, int32_t which_font, int32_t options) {
    (void)content_original;
    (void)content_bytes;
    (void)default_pixel_height;
    (void)which_font;
    (void)options;
    return 0;
}

int32_t FontRenderTextASCII(int32_t i, uint8_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y,
                            int32_t *out_x_pre_increment, int32_t *out_x_post_increment) {
    (void)i;
    (void)codepoint;
    (void)codepoints;
    (void)options;
    (void)out_data;
    (void)out_x;
    (void)out_y;
    (void)out_x_pre_increment;
    (void)out_x_post_increment;
    return 0;
}

int32_t FontWidth(int32_t i) {
    (void)i;
    return 0;
}

void FontFree(int32_t i) { (void)i; }
