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

//-----------------------------------------------------------------------------------------------------
// HEADER FILES
//-----------------------------------------------------------------------------------------------------
// Set this to 1 if we want to print debug messages to stderr
#define IMAGE_DEBUG 0
#include "image.h"
#include <unordered_map>
#define DR_PCX_IMPLEMENTATION
#include "dr_pcx.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// The below include is a bad idea because of reasons mentioned in https://github.com/QB64-Phoenix-Edition/QB64pe/issues/172
// However, we need a bunch of things like the 'qbs' and 'image' structs and some more
// We'll likely keep the 'include' this way because I do not want to duplicate stuff and cause issues
// Matt is already doing work to separate and modularize libqb
// So, this will be replaced with relevant stuff once that work is done
#include "../../libqb.h"
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------------------------------
// This is returned to the caller if something goes wrong while loading the image
#define INVALID_IMAGE_HANDLE -1
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------------------------------
// The byte ordering here are straight from libqb.cpp. So, if libqb.cpp is wrong, then we are wrong! ;)
#define IMAGE_GET_BGRA_RED(c) (uint32_t(c) >> 16 & 0xFF)
#define IMAGE_GET_BGRA_GREEN(c) (uint32_t(c) >> 8 & 0xFF)
#define IMAGE_GET_BGRA_BLUE(c) (uint32_t(c) & 0xFF)
#define IMAGE_GET_BGRA_ALPHA(c) (uint32_t(c) >> 24)
#define IMAGE_MAKE_BGRA(r, g, b, a) (uint32_t((uint8_t(b) | (uint16_t(uint8_t(g)) << 8)) | (uint32_t(uint8_t(r)) << 16) | (uint32_t(uint8_t(a)) << 24)))
// Calculates the RGB distance in the RGB color cube
#define IMAGE_CALCULATE_RGB_DISTANCE(r1, g1, b1, r2, g2, b2)                                                                                                   \
    sqrt(((float(r2) - float(r1)) * (float(r2) - float(r1))) + ((float(g2) - float(g1)) * (float(g2) - float(g1))) +                                           \
         ((float(b2) - float(b1)) * (float(b2) - float(b1))))

#ifdef QB64_WINDOWS
#    define ZERO_VARIABLE(_v_) ZeroMemory(&(_v_), sizeof(_v_))
#else
#    define ZERO_VARIABLE(_v_) memset(&(_v_), 0, sizeof(_v_))
#endif
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//-----------------------------------------------------------------------------------------------------
// These should be replaced with appropriate header files when Matt finishes cleaning up libqb
qbs *qbs_new_txt_len(const char *, int32); // Not declared in libqb.h
void sub__freeimage(int32, int32);         // Not declared in libqb.h

extern img_struct *img;        // Required by func__loadimage
extern img_struct *write_page; // Required by func__loadimage
extern uint32 palette_256[];   // Required by func__loadimage
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------------------------------
/// <summary>
/// Decodes an image file using the dr_pcx & stb_image libraries.
/// </summary>
/// <param name="fileName">A valid filename</param>
/// <param name="xOut">Out: width in pixels. This cannot be NULL</param>
/// <param name="yOut">Out: height in pixels. This cannot be NULL</param>
/// <returns>A pointer to the raw pixel data in RGBA format or NULL on failure</returns>
static uint8_t *image_decode(const char *fileName, int *xOut, int *yOut) {
    auto compOut = 0;

    IMAGE_DEBUG_PRINT("Image dimensions (passed) = (%i, %i)", *xOut, *yOut);

    // Attempt to load file as a PCX first using dr_pcx
    auto pixels = drpcx_load_file(fileName, DRPCX_FALSE, xOut, yOut, &compOut, 4);
    IMAGE_DEBUG_PRINT("Image dimensions (dr_pcx) = (%i, %i)", *xOut, *yOut);
    if (!pixels) {
        // If dr_pcx failed to load, then use stb_image
        pixels = stbi_load(fileName, xOut, yOut, &compOut, 4);
        IMAGE_DEBUG_PRINT("Image dimensions (stb_image) = (%i, %i)", *xOut, *yOut);
        if (!pixels)
            return nullptr; // Return NULL if all attempts failed
    }

    IMAGE_DEBUG_CHECK(compOut > 2); // Returned component should always be 3 or more

    return pixels;
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
static uint8_t *image_convert_8bpp(uint8_t *src, int w, int h, uint32_t *paletteOut) {
    static struct {
        uint32_t r, g, b;
        uint32_t count;
    } cubes[256];

    // https://en.wikipedia.org/wiki/Ordered_dithering
    static uint8_t bayerMatrix[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

    IMAGE_DEBUG_PRINT("Converting 32bpp image (%i, %i) to 8bpp", w, h);

    // Allocate memory for new image (8-bit indexed)
    auto pixels = (uint8_t *)malloc(w * h);
    if (!pixels) {
        return nullptr;
    }

    ZERO_VARIABLE(cubes);

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
/// This takes in a 32bpp (BGRA) image raw data and spits out an 8bpp raw image along with it's 256 color (BGRA) palette.
/// If the number of unique colors in the 32bpp image > 256, then the functions returns a NULL.
/// Unlike image_convert_8bpp(), no 'real' conversion takes place.
/// </summary>
/// <param name="src">The source raw image data. This must be in BGRA format and not NULL</param>
/// <param name="w">The widht of the image in pixels</param>
/// <param name="h">The height of the image in pixels</param>
/// <param name="paletteOut">A 256 color palette if the operation was successful. This cannot be NULL</param>
/// <returns>A pointer to a 8bpp raw image or NULL if operation failed</returns>
static uint8_t *image_make_8bpp(uint8_t *src, int w, int h, uint32_t *paletteOut) {
    IMAGE_DEBUG_PRINT("Extracting 8bpp image (%i, %i) from 32bpp", w, h);

    unordered_map<uint32_t, int> colorMap;

    // Allocate memory for new image (8-bit indexed)
    auto pixels = (uint8_t *)malloc(w * h);
    if (!pixels) {
        return nullptr;
    }

    auto uniqueColors = 0; // As long as this is <= 256 we will keep going until we are done
    auto src32bpp = (uint32_t *)src;
    for (auto i = 0; i < w * h; i++) {
        auto srcColor = src32bpp[i];

        // Check if the src color exists in our palette
        if (colorMap.find(srcColor) == colorMap.end()) {
            // If we reached here, then the color is not in our table
            ++uniqueColors;
            if (uniqueColors > 256) {
                IMAGE_DEBUG_PRINT("Image has more than 256 unique colors (%i)", uniqueColors);
                free(pixels);
                return nullptr; // Exit with failure if we have > 256 colors
            }

            paletteOut[uniqueColors - 1] = srcColor; // Store the color as unique
            colorMap[srcColor] = uniqueColors - 1;   // Add this color to the map
            pixels[i] = uniqueColors - 1;
        } else {
            // If we reached here, then the color is in our table
            pixels[i] = colorMap[srcColor]; // Simply fetch the index from the map
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
static void image_remap_palette(uint8_t *src, int w, int h, uint32_t *src_pal, uint32_t *dst_pal) {
    static uint32_t palMap[256];

    IMAGE_DEBUG_PRINT("Remapping 8bpp image (%i, %i) palette", w, h);

    ZERO_VARIABLE(palMap);

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
/// <param name="fileName">The filename of the image</param>
/// <param name="bpp">Mode: 32=32bpp, 33=hardware acclerated 32bpp, 256=8bpp or 257=8bpp without palette remap</param>
/// <param name="passed">How many parameters were passed?</param>
/// <returns>Valid LONG image handle values that are less than -1 or -1 on failure</returns>
int32_t func__loadimage(qbs *fileName, int32_t bpp, int32_t passed) {
    // QB string that we'll need null terminate the filename
    static qbs *fileNameZ = nullptr;

    if (new_error)
        return 0;

    if (!fileNameZ)
        fileNameZ = qbs_new(0, 0);

    auto isHardware = false;
    auto dontRemapPalette = false;

    // Handle special cases
    if (bpp == 33) {
        bpp = 32;
        isHardware = true;
        IMAGE_DEBUG_PRINT("Hardware image requested");
    } else if (bpp == 257) {
        bpp = 256;
        dontRemapPalette = true;
        IMAGE_DEBUG_PRINT("No palette remap requested");
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
        IMAGE_DEBUG_PRINT("BPP was not spcified. Defaulting to 32bpp");
    }

    qbs_set(fileNameZ, qbs_add(fileName, qbs_new_txt_len("\0", 1))); // s1 = filename + CHR$(0)
    if (fileNameZ->len == 1)
        return INVALID_IMAGE_HANDLE; // Return invalid handle if null length string

    int x, y;
    // Try to load the image
    auto pixels = image_decode((const char *)fileNameZ->chr, &x, &y);
    if (!pixels)
        return INVALID_IMAGE_HANDLE; // Return invalid handle if loading the image failed

    IMAGE_DEBUG_PRINT("'%s' successfully loaded", fileNameZ->chr);

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
        IMAGE_DEBUG_PRINT("Entering 8bpp path");

        i = func__newimage(x, y, 256, 1);
        if (i == -1) {
            free(pixels);
            return INVALID_IMAGE_HANDLE;
        }

        auto palette = (uint32_t *)malloc(256 * sizeof(uint32_t)); // 3 bytes for bgr + 1 for alpha (basically a uint32_t)
        if (!palette) {
            free(pixels);
            return INVALID_IMAGE_HANDLE;
        }

        auto pixels256 = image_make_8bpp(pixels, x, y, palette); // Try to simply 'extract' the 8bpp image first
        if (!pixels256) {
            pixels256 = image_convert_8bpp(pixels, x, y, palette); // If that fails, then 'convert' it to 8bpp
            if (!pixels256) {
                free(palette);
                free(pixels);
                return INVALID_IMAGE_HANDLE;
            }
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
        IMAGE_DEBUG_PRINT("Entering 32bpp path");

        i = func__newimage(x, y, 32, 1);
        if (i == -1) {
            free(pixels);
            return INVALID_IMAGE_HANDLE;
        }
        memcpy(img[-i].offset, pixels, x * y * sizeof(uint32_t));
    }

    // Free pixel memory. We can do this because both dr_pcx and stb_image uses free()
    free(pixels);

    // This only executes if bpp is 32
    if (isHardware) {
        IMAGE_DEBUG_PRINT("Making hardware image");

        auto iHardware = func__copyimage(i, 33, 1);
        sub__freeimage(i, 1);
        i = iHardware;
    }

    IMAGE_DEBUG_PRINT("Returning handle value = %i", i);

    return i;
}
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
