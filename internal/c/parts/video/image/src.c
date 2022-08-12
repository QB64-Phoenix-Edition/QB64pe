extern uint32 matchcol(int32 r, int32 g, int32 b);

#ifndef DEPENDENCY_IMAGE_CODEC
// Stub(s):
int32 func__loadimage(qbs *f, int32 bpp, int32 passed);
#else
#    include "decode/pcx/src.c"
#    include "decode/stb/src.c"

int32 func__loadimage(qbs *f, int32 bpp, int32 passed) {
    if (new_error)
        return 0;

    static int32 isHardware;
    isHardware = 0;
    if (bpp == 33) {
        bpp = 32;
        isHardware = 1;
    }

    // validate bpp
    if (passed) {
        if ((bpp != 32) && (bpp != 256)) {
            error(5);
            return 0;
        }
    } else {
        if (write_page->text) {
            error(5);
            return 0;
        }
        bpp = -1;
    }
    if (!f->len)
        return -1; // return invalid handle if null length string
    if (bpp == 256)
        return -1; // return invalid handle if 256-color mode requested (not valid in this version)

    // load the file
    int32 fh, result = 0;
    int64 lof;
    fh = gfs_open(f, 1, 0, 0);
    if (fh < 0)
        return -1;
    lof = gfs_lof(fh);
    static uint8 *content;
    content = (uint8 *)malloc(lof);
    if (!content) {
        gfs_close(fh);
        return -1;
    }
    result = gfs_read(fh, -1, content, lof);
    gfs_close(fh);
    if (result < 0) {
        free(content);
        return -1;
    }

    static uint8 *pixels;
    static int32 x, y;

    // Try to load the image using dr_pcx
    pixels = image_decode_pcx(content, lof, &result, &x, &y);
    // If that failed try loading via stb_image
    if (!(result & 1)) {
        pixels = image_decode_stb(content, lof, &result, &x, &y);
    }

    // Free the memory holding the file
    free(content);

    // Return failure if nothing was able to load the image
    if (!(result & 1))
        return -1;

    static int32 i;

    i = func__newimage(x, y, 32, 1);
    if (i == -1) {
        free(pixels);
        return -1;
    }
    memcpy(img[-i].offset, pixels, x * y * 4);

    free(pixels);

    if (isHardware) {
        static int32 iHardware;
        iHardware = func__copyimage(i, 33, 1);
        sub__freeimage(i, 1);
        i = iHardware;
    }

    return i;
}

#endif
