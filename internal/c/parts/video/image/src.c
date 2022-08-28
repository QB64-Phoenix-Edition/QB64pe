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

#ifndef DEPENDENCY_IMAGE_CODEC
// Stub(s):
int32_t func__loadimage(qbs *f, int32_t bpp, int32_t passed);

#else
#    define DR_PCX_IMPLEMENTATION
#    include "dr_pcx.h"
#    define STB_IMAGE_IMPLEMENTATION
#    include "stb_image.h"

// The byte ordering here are straight from libqb.cpp. So, if libqb.cpp is wrong, then we are wrong! ;)
#    define IMAGE_GET_BGRA_RED(c) (uint32_t(c) >> 16 & 0xFF)
#    define IMAGE_GET_BGRA_GREEN(c) (uint32_t(c) >> 8 & 0xFF)
#    define IMAGE_GET_BGRA_BLUE(c) (uint32_t(c) & 0xFF)
#    define IMAGE_GET_BGRA_ALPHA(c) (uint32_t(c) >> 24)
#    define IMAGE_MAKE_BGRA(r, g, b, a) (uint32_t((uint8_t(b) | (uint16_t(uint8_t(g)) << 8)) | (uint32_t(uint8_t(r)) << 16) | (uint32_t(uint8_t(a)) << 24)))
// Calculates the RGB distance in the RGB color cube
#    define IMAGE_CALCULATE_RGB_DISTANCE(r1, g1, b1, r2, g2, b2)                                                                                               \
        sqrt(((float(r2) - float(r1)) * (float(r2) - float(r1))) + ((float(g2) - float(g1)) * (float(g2) - float(g1))) +                                       \
             ((float(b2) - float(b1)) * (float(b2) - float(b1))))

/// <summary>
/// Decodes a PCX image using the dr_pcx library.
/// </summary>
/// <param name="content">A pointer to the file in memory</param>
/// <param name="bytes">The length of the file</param>
/// <param name="result">Out: bit 1=Success, bit 2=32bit. This cannot be NULL[BGRA]</param>
/// <param name="x">Out: width in pixels. This cannot be NULL</param>
/// <param name="y">Out: height in pixels. This cannot be NULL</param>
/// <returns>A pointer to the raw pixel data in RGBA format or NULL on failure</returns>
static uint8_t *image_decode_drpcx(uint8_t *content, int32_t bytes, int32_t *result, int32_t *x, int32_t *y) {
    auto h = 0, w = 0, comp = 0;
    *result = 0;

    auto out = drpcx_load_memory(content, bytes, DRPCX_FALSE, &w, &h, &comp, 4);
    if (!out)
        return nullptr;

    *result = 1 + 2;
    *x = w;
    *y = h;
    return out;
}

/// <summary>
/// Decodes an image using the stb_image library.
/// </summary>
/// <param name="content">A pointer to the file in memory</param>
/// <param name="bytes">The length of the file</param>
/// <param name="result">Out: bit 1=Success, bit 2=32bit[BGRA]. This cannot be NULL</param>
/// <param name="x">Out: width in pixels. This cannot be NULL</param>
/// <param name="y">Out: height in pixels. This cannot be NULL</param>
/// <returns>A pointer to the raw pixel data in RGBA format or NULL on failure</returns>
static uint8_t *image_decode_stbi(uint8_t *content, int32_t bytes, int32_t *result, int32_t *x, int32_t *y) {
    auto h = 0, w = 0, comp = 0;
    *result = 0;

    auto out = stbi_load_from_memory(content, bytes, &w, &h, &comp, 4);
    if (!out)
        return nullptr;

    *result = 1 + 2;
    *x = w;
    *y = h;
    return out;
}

/// <summary>
/// Clamps a color channel to the range 0 - 255.
/// </summary>
/// <param name="n">The color component</param>
/// <returns>The clamped value</returns>
static inline uint8_t image_clamp_component(int32_t n) {
    n &= -(n >= 0);
    return n | ((255 - n) >> 31);
}

/// <summary>
/// This takes in a 32bpp (BGRA) image raw data and spits out an 8bpp raw image along with it's 256 color (BGRA) palette.
/// </summary>
/// <param name="src">The source raw image data. This must be in BGRA format and not NULL</param>
/// <param name="w">The widht of the image in pixels</param>
/// <param name="h">The height of the image in pixels</param>
/// <param name="paletteOut">A 256 color palette if the operation was successful. This cannot be NULL</param>
/// <returns>A pointer to a 8bpp raw image or NULL if operation failed</returns>
static uint8_t *image_convert_8bpp(uint8_t *src, int32_t w, int32_t h, uint32_t *paletteOut) {
    static struct {
        uint32_t r, g, b;
        uint32_t count;
    } cubes[256];

    // https://en.wikipedia.org/wiki/Ordered_dithering
    static uint8_t bayerMatrix[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

    // Allocate memory for new image (8-bit indexed)
    auto pixels = (uint8_t *)malloc(w * h);
    if (!pixels) {
        return nullptr;
    }

    memset(cubes, NULL, sizeof(cubes));

    // Quantization phase
    auto dst = pixels;
    for (auto y = 0; y < h; y++) {
        for (auto x = 0; x < w; x++) {
            int32_t t = bayerMatrix[((y & 3) << 2) + (x & 3)];
            int32_t b = image_clamp_component((*src++) + (t << 1));
            int32_t g = image_clamp_component((*src++) + (t << 1));
            int32_t r = image_clamp_component((*src++) + (t << 1));
            ++src; // Ignore alpha

            // Quantize
            uint8_t k = ((r >> 5) << 5) + ((g >> 5) << 2) + (b >> 6);
            (*dst++) = k;

            // Prepare RGB cubes for CLUT
            cubes[k].r += r;
            cubes[k].g += g;
            cubes[k].b += b;
            cubes[k].count++;
        }
    }

    // Generate a uniform CLUT based on the quantized colors
    for (auto i = 0; i < 256; i++) {
        if (cubes[i].count) {
            paletteOut[i] = IMAGE_MAKE_BGRA(cubes[i].r / cubes[i].count, cubes[i].g / cubes[i].count, cubes[i].b / cubes[i].count, 0xFF);
        } else {
            paletteOut[i] = IMAGE_MAKE_BGRA(0, 0, 0, 0xFF);
        }
    }

    return pixels;
}

/// <summary>
/// This modifies an *8bpp* image 'src' to use 'dst_pal' instead of 'src_pal'
/// </summary>
/// <param name="src">A pointer to the 8bpp image pixel data. This modifies data 'src' points to and cannot be NULL</param>
/// <param name="w">The width of the image in pixels</param>
/// <param name="h">The height of the image in pixels</param>
/// <param name="src_pal">The image's original palette. This cannot be NULL</param>
/// <param name="dst_pal">The destination palette. This cannot be NULL</param>
static void image_remap_palette(uint8_t *src, int32_t w, int32_t h, uint32_t *src_pal, uint32_t *dst_pal) {
    static uint32_t palMap[256];

    memset(palMap, NULL, sizeof(palMap));

    // Match the palette
    for (auto x = 0; x < 256; x++) {
        auto oldDist = IMAGE_CALCULATE_RGB_DISTANCE(0, 0, 0, 255, 255, 255); // The farthest we can go in the color cube
        for (auto y = 0; y < 256; y++) {
            auto newDist = IMAGE_CALCULATE_RGB_DISTANCE(IMAGE_GET_BGRA_RED(src_pal[x]), IMAGE_GET_BGRA_GREEN(src_pal[x]), IMAGE_GET_BGRA_BLUE(src_pal[x]),
                                                        IMAGE_GET_BGRA_RED(dst_pal[y]), IMAGE_GET_BGRA_GREEN(dst_pal[y]), IMAGE_GET_BGRA_BLUE(dst_pal[y]));

            if (oldDist > newDist) {
                oldDist = newDist;
                palMap[x] = y;
            }
        }
    }

    // Update the bitmap to use the matched palette
    for (auto c = 0; c < (w * h); c++) {
        src[c] = palMap[src[c]];
    }
}

/// <summary>
/// This function loads an image into memory and returns valid LONG image handle values that are less than -1.
/// </summary>
/// <param name="f">The filename of the image</param>
/// <param name="bpp">Mode: 32=32bpp, 33=hardware acclerated 32bpp, 256=8bpp or 257=8bpp without palette remap</param>
/// <param name="passed">How many parameters were passed?</param>
/// <returns>Valid LONG image handle values that are less than -1 or -1 on failure</returns>
int32_t func__loadimage(qbs *f, int32_t bpp, int32_t passed) {
    if (new_error)
        return 0;

    auto isHardware = false;
    auto dontRemapPalette = false;

    // Handle special cases
    if (bpp == 33) {
        bpp = 32;
        isHardware = true;
    } else if (bpp == 257) {
        bpp = 256;
        dontRemapPalette = true;
    }

    // Validate bpp
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

    // Load the file
    auto fh = gfs_open(f, 1, 0, 0);
    if (fh < 0)
        return -1;
    auto lof = gfs_lof(fh);
    auto content = (uint8 *)malloc(lof);
    if (!content) {
        gfs_close(fh);
        return -1;
    }
    auto result = gfs_read(fh, -1, content, lof);
    gfs_close(fh);
    if (result < 0) {
        free(content);
        return -1;
    }

    int32_t x, y;
    // Try to load the image using dr_pcx
    auto pixels = image_decode_drpcx(content, lof, &result, &x, &y);
    // If that failed try loading via stb_image
    if (!(result & 1)) {
        pixels = image_decode_stbi(content, lof, &result, &x, &y);
    }

    // Free the memory holding the file
    free(content);

    // Return failure if nothing was able to load the image
    if (!(result & 1))
        return -1;

    // Convert RGBA to BGRA
    auto cp = pixels;
    for (auto y2 = 0; y2 < y; y2++) {
        for (auto x2 = 0; x2 < x; x2++) {
            auto r = cp[0];
            auto b = cp[2];
            cp[0] = b;
            cp[2] = r;
            cp += 4;
        }
    }

    int32_t i; // Image handle to be returned

    // Convert image to 8bpp if requested by the user
    if (bpp == 256) {
        i = func__newimage(x, y, 256, 1);
        if (i == -1) {
            free(pixels);
            return -1;
        }

        auto palette = (uint32_t *)malloc(256 * sizeof(uint32_t)); // 3 bytes for bgr + 1 for alpha (basically a uint32_t)
        if (!palette) {
            free(pixels);
            return -1;
        }

        auto pixels256 = image_convert_8bpp(pixels, x, y, palette);
        if (!pixels256) {
            free(palette);
            free(pixels);
            return -1;
        }

        if (dontRemapPalette) {
            // Copy the 8bpp pixel data and then free it
            memcpy(img[-i].offset, pixels256, x * y);
            free(pixels256);

            // Copy the palette and then free it
            memcpy(img[-i].pal, palette, 256 * sizeof(uint32_t));
            free(palette);
        } else {
            // Remap the image indexes to QB64 default palette and then free our palette
            image_remap_palette(pixels256, x, y, palette, palette_256);
            free(palette);

            // Copy the 8bpp pixel data and then free it
            memcpy(img[-i].offset, pixels256, x * y);
            free(pixels256);

            // Copy the default QB64 palette
            memcpy(img[-i].pal, palette_256, 256 * sizeof(uint32_t));
        }
    } else {
        i = func__newimage(x, y, 32, 1);
        if (i == -1) {
            free(pixels);
            return -1;
        }
        memcpy(img[-i].offset, pixels, x * y * sizeof(uint32_t));
    }

    // Free pixel memory. We can do this because both dr_pcx and stb_image uses free()
    free(pixels);

    // This only executes if bpp is 32
    if (isHardware) {
        auto iHardware = func__copyimage(i, 33, 1);
        sub__freeimage(i, 1);
        i = iHardware;
    }

    return i;
}

#endif
