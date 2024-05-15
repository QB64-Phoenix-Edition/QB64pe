// Font stubs

#include "font.h"

uint8_t *FontLoadFileToMemory(const char *file_path_name, int32_t *out_bytes) {
    (void)file_path_name;
    (void)out_bytes;
    return nullptr;
}

int32_t FontLoad(const uint8_t *content_original, int32_t content_bytes, int32_t default_pixel_height, int32_t which_font, int32_t &options) {
    (void)content_original;
    (void)content_bytes;
    (void)default_pixel_height;
    (void)which_font;
    (void)options;
    return 0;
}

void FontFree(int32_t fh) { (void)fh; }

int32_t FontWidth(int32_t fh) {
    (void)fh;
    return 0;
}

bool FontRenderTextUTF32(int32_t fh, const uint32_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y) {
    (void)fh;
    (void)codepoint;
    (void)codepoints;
    (void)options;
    (void)out_data;
    (void)out_x;
    (void)out_y;
    return 0;
}

bool FontRenderTextASCII(int32_t fh, const uint8_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y) {
    (void)fh;
    (void)codepoint;
    (void)codepoints;
    (void)options;
    (void)out_data;
    (void)out_x;
    (void)out_y;
    return 0;
}

int32_t FontPrintWidthUTF32(int32_t fh, const uint32_t *codepoint, int32_t codepoints) {
    (void)fh;
    (void)codepoint;
    (void)codepoints;
    return 0;
}

int32_t FontPrintWidthASCII(int32_t fh, const uint8_t *codepoint, int32_t codepoints) {
    (void)fh;
    (void)codepoint;
    (void)codepoints;
    return 0;
}
