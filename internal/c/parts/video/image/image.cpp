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

#include <unordered_map>
#define DR_PCX_IMPLEMENTATION
#include "dr_pcx.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// Set this to 1 if we want to print debug messages to stderr
#define IMAGE_DEBUG 0
#include "image.h"
// We need 'qbs' and 'image' structs stuff from here. This should eventually change when things are moved to smaller, logical and self-contained files
#include "../../../libqb.h"

// This is returned to the caller if something goes wrong while loading the image
#define INVALID_IMAGE_HANDLE -1
// Various requirement strings for func__imageload
#define REQUIREMENT_STRING_HARDWARE "HARDWARE"
#define REQUIREMENT_STRING_MEMORY "MEMORY"
#define REQUIREMENT_STRING_ADAPTIVE "ADAPTIVE"

// Calculates the RGB distance in the RGB color cube
#define IMAGE_CALCULATE_RGB_DISTANCE(r1, g1, b1, r2, g2, b2)                                                                                                   \
    sqrt(((float(r2) - float(r1)) * (float(r2) - float(r1))) + ((float(g2) - float(g1)) * (float(g2) - float(g1))) +                                           \
         ((float(b2) - float(b1)) * (float(b2) - float(b1))))

#ifdef QB64_WINDOWS
#    define ZERO_VARIABLE(_v_) ZeroMemory(&(_v_), sizeof(_v_))
#else
#    define ZERO_VARIABLE(_v_) memset(&(_v_), 0, sizeof(_v_))
#endif

// These should be replaced with appropriate header files when Matt finishes cleaning up libqb
void sub__freeimage(int32, int32);                                  // Not declared in libqb.h
int32 func_instr(int32 start, qbs *str, qbs *substr, int32 passed); // Did not find this declared anywhere

extern img_struct *img;        // Required by func__loadimage
extern img_struct *write_page; // Required by func__loadimage
extern uint32 palette_256[];   // Required by func__loadimage

/// @brief Decodes an image file freom a file using the dr_pcx & stb_image libraries.
/// @param fileName A valid filename
/// @param xOut Out: width in pixels. This cannot be NULL
/// @param yOut Out: height in pixels. This cannot be NULL
/// @return A pointer to the raw pixel data in RGBA format or NULL on failure
static uint8_t *image_decode_from_file(const char *fileName, int *xOut, int *yOut) {
    auto compOut = 0;

    IMAGE_DEBUG_PRINT("Loading image from file %s", fileName);

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

    IMAGE_DEBUG_CHECK(compOut > 2);

    return pixels;
}

/// @brief Decodes an image file from memory using the dr_pcx & stb_image libraries
/// @param data The raw pointer to the file in memory
/// @param size The size of the file in memory
/// @param xOut Out: width in pixels. This cannot be NULL
/// @param yOut Out: height in pixels. This cannot be NULL
/// @return A pointer to the raw pixel data in RGBA format or NULL on failure
static uint8_t *image_decode_from_memory(const void *data, size_t size, int *xOut, int *yOut) {
    auto compOut = 0;

    IMAGE_DEBUG_PRINT("Loading image from memory");

    // Attempt to load file as a PCX first using dr_pcx
    auto pixels = drpcx_load_memory(data, size, DRPCX_FALSE, xOut, yOut, &compOut, 4);
    IMAGE_DEBUG_PRINT("Image dimensions (dr_pcx) = (%i, %i)", *xOut, *yOut);
    if (!pixels) {
        // If dr_pcx failed to load, then use stb_image
        pixels = stbi_load_from_memory((stbi_uc const *)data, size, xOut, yOut, &compOut, 4);
        IMAGE_DEBUG_PRINT("Image dimensions (stb_image) = (%i, %i)", *xOut, *yOut);
        if (!pixels)
            return nullptr; // Return NULL if all attempts failed
    }

    IMAGE_DEBUG_CHECK(compOut > 2);

    return pixels;
}

/// @brief Clamps a color channel to the range 0 - 255
/// @param n The color component
/// @return The clamped value
static inline uint8_t image_clamp_component(int32_t n) { return n < 0 ? 0 : n > 255 ? 255 : n; }

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

    std::unordered_map<uint32_t, int> colorMap;

    // Allocate memory for new image (8-bit indexed)
    auto pixels = (uint8_t *)malloc(w * h);
    if (!pixels) {
        return nullptr;
    }

    auto uniqueColors = 0;           // as long as this is < 256 we will keep going until we are done
    auto src32bpp = (uint32_t *)src; // get a 32-bit int pointer to the image data
    for (auto i = 0; i < w * h; i++) {
        auto srcColor = src32bpp[i]; // get the 32bpp pixel

        // Check if the src color exists in our palette
        if (colorMap.find(srcColor) == colorMap.end()) {
            // If we reached here, then the color is not in our table
            if (uniqueColors > 255) {
                IMAGE_DEBUG_PRINT("Image has more than %i unique colors", uniqueColors);
                free(pixels);
                return nullptr; // Exit with failure if we have > 256 colors
            }

            paletteOut[uniqueColors] = srcColor; // Store the color as unique
            colorMap[srcColor] = uniqueColors;   // Add this color to the map
            pixels[i] = uniqueColors;            // set the pixel to the color index
            ++uniqueColors;                      // increment unique colors
        } else {
            // If we reached here, then the color is in our table
            pixels[i] = colorMap[srcColor]; // Simply fetch the index from the map
        }
    }

    IMAGE_DEBUG_PRINT("Unique colors = %i", uniqueColors);

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

/// @brief This function loads an image into memory and returns valid LONG image handle values that are less than -1.
/// @param fileName The filename or memory buffer of the image
/// @param bpp bpp">Mode: 32=32bpp, 33=hardware acclerated 32bpp, 256=8bpp or 257=8bpp without palette remap
/// @param requirements A qbs that can contain one or more of: hardware, memory, 32bpp, 8bpp, adaptive
/// @param passed How many parameters were passed?
/// @return Valid LONG image handle values that are less than -1 or -1 on failure
int32_t func__loadimage(qbs *fileName, int32_t bpp, qbs *requirements, int32_t passed) {
    static qbs *fileNameZ = nullptr; // QB string that we'll need null terminate the filename
    static qbs *reqs = nullptr;      // QB strign that we'll need to convert requirements to uppercase

    if (new_error || !fileName->len) // leave if we do not have a file name, data or there was an error
        return INVALID_IMAGE_HANDLE;

    if (!fileNameZ)
        fileNameZ = qbs_new(0, 0);

    if (!reqs)
        reqs = qbs_new(0, 0);

    auto isLoadFromMemory = false; // should the image be loaded from memory?
    auto isHardwareImage = false;  // should the image be converted to a hardware image?
    auto isRemapPalette = true;    // should the palette be re-mapped to the QB64 default palette?

    // Handle special cases and set the above flags if required
    IMAGE_DEBUG_PRINT("bpp = 0x%X, passed = 0x%X", bpp, passed);
    if (passed & 1) {
        if (bpp == 33) { // hardware image?
            isHardwareImage = true;
            bpp = 32;
            IMAGE_DEBUG_PRINT("bpp = 0x%X", bpp);
        } else if (bpp == 257) { // adaptive palette?
            isRemapPalette = false;
            bpp = 256;
            IMAGE_DEBUG_PRINT("bpp = 0x%X", bpp);
        }

        if ((bpp != 32) && (bpp != 256)) { // invalid BPP?
            IMAGE_DEBUG_PRINT("Invalid bpp (0x%X)", bpp);
            error(5);
            return INVALID_IMAGE_HANDLE;
        }
    } else {
        if (write_page->bits_per_pixel < 32) { // default to 8bpp for all legacy screen modes
            bpp = 256;
            IMAGE_DEBUG_PRINT("Defaulting to 8bpp");
        } else { // default to 32bpp for everything else
            bpp = 32;
            IMAGE_DEBUG_PRINT("Defaulting to 32bpp");
        }
    }

    // Check requirements string and set appropriate flags
    if ((passed & 2) && requirements->len) {
        IMAGE_DEBUG_PRINT("Parsing requirements");

        qbs_set(reqs, qbs_ucase(requirements)); // Convert tmp str to perm str

        if (func_instr(1, reqs, qbs_new_txt(REQUIREMENT_STRING_HARDWARE), 1) && bpp == 32) {
            isHardwareImage = true;
            IMAGE_DEBUG_PRINT("Generating hardware image");
        } else if (func_instr(1, reqs, qbs_new_txt(REQUIREMENT_STRING_ADAPTIVE), 1) && bpp == 256) {
            isRemapPalette = false;
            IMAGE_DEBUG_PRINT("Generating adaptive palette");
        }

        if (func_instr(1, reqs, qbs_new_txt(REQUIREMENT_STRING_MEMORY), 1)) {
            isLoadFromMemory = true;
            IMAGE_DEBUG_PRINT("Loading image from memory");
        }
    }

    int x, y;
    uint8_t *pixels;

    if (isLoadFromMemory) {
        pixels = image_decode_from_memory(fileName->chr, fileName->len, &x, &y);
    } else {
        qbs_set(fileNameZ, qbs_add(fileName, qbs_new_txt_len("\0", 1))); // s1 = filename + CHR$(0)
        pixels = image_decode_from_file((const char *)fileNameZ->chr, &x, &y);
    }

    if (!pixels)
        return INVALID_IMAGE_HANDLE; // Return invalid handle if loading the image failed

    // Convert RGBA to BGRA
    auto cp = pixels;
    for (auto y2 = 0; y2 < y; y2++) {
        for (auto x2 = 0; x2 < x; x2++) {
            auto r = cp[0];
            auto b = cp[2];
            cp[0] = b;
            cp[2] = r;
            cp += sizeof(uint32_t);
        }
    }

    int32_t i; // Image handle to be returned

    // Convert image to 8bpp if requested by the user
    if (bpp == 256) {
        IMAGE_DEBUG_PRINT("Entering 8bpp path");

        i = func__newimage(x, y, 256, 1);
        if (i == INVALID_IMAGE_HANDLE) {
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

        if (isRemapPalette) {
            // Remap the image indexes to QB64 default palette and then free our palette
            image_remap_palette(pixels256, x, y, palette, palette_256);
            free(palette);

            // Copy the 8bpp pixel data and then free it
            memcpy(img[-i].offset, pixels256, x * y);
            free(pixels256);

            // Copy the default QB64 palette
            memcpy(img[-i].pal, palette_256, 256 * sizeof(uint32_t));
        } else {
            // Copy the 8bpp pixel data and then free it
            memcpy(img[-i].offset, pixels256, x * y);
            free(pixels256);

            // Copy the palette and then free it
            memcpy(img[-i].pal, palette, 256 * sizeof(uint32_t));
            free(palette);
        }
    } else {
        IMAGE_DEBUG_PRINT("Entering 32bpp path");

        i = func__newimage(x, y, 32, 1);
        if (i == INVALID_IMAGE_HANDLE) {
            free(pixels);
            return INVALID_IMAGE_HANDLE;
        }
        memcpy(img[-i].offset, pixels, x * y * sizeof(uint32_t));
    }

    // Free pixel memory. We can do this because both dr_pcx and stb_image uses free()
    free(pixels);

    // This only executes if bpp is 32
    if (isHardwareImage) {
        IMAGE_DEBUG_PRINT("Making hardware image");

        auto iHardware = func__copyimage(i, 33, 1);
        sub__freeimage(i, 1);
        i = iHardware;
    }

    IMAGE_DEBUG_PRINT("Returning handle value = %i", i);

    return i;
}
