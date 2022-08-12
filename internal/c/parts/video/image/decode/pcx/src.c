#define DR_PCX_IMPLEMENTATION
#include "src/dr_pcx.h "

uint8 *image_decode_pcx(uint8 *content, int32 bytes, int32 *result, int32 *x, int32 *y) {
    // Result:bit 1=Success,bit 2=32bit[BGRA]
    *result = 0;

    int32 h = 0, w = 0;
    uint8 *out;
    int comp = 0;

    out = drpcx_load_memory(content, bytes, DRPCX_FALSE, &w, &h, &comp, 4);

    if (out == NULL)
        return NULL;

    // RGBA->BGRA
    uint8 *cp = out;
    int32 x2, y2;
    int32 r, g, b, a;
    for (y2 = 0; y2 < h; y2++) {
        for (x2 = 0; x2 < w; x2++) {
            r = cp[0];
            b = cp[2];
            cp[0] = b;
            cp[2] = r;
            cp += 4;
        }
    }

    *result = 1 + 2;
    *x = w;
    *y = h;
    return out;
}
