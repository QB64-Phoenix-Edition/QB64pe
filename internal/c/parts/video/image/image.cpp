//-----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___   ___                       _    _ _
//   / _ \| _ ) / /| | || _ \ __| |_ _|_ __  __ _ __ _ ___  | |  (_) |__ _ _ __ _ _ _ _  _
//  | (_) | _ \/ _ \_  _|  _/ _|   | || '  \/ _` / _` / -_) | |__| | '_ \ '_/ _` | '_| || |
//   \__\_\___/\___/ |_||_| |___| |___|_|_|_\__,_\__, \___| |____|_|_.__/_| \__,_|_|  \_, |
//                                               |___/                                |__/
//
//  Powered by:
//      stb_image & stb_image_write (https://github.com/nothings/stb)
//      dr_pcx (https://github.com/mackron/dr_pcx)
//      nanosvg (https://github.com/memononen/nanosvg)
//      qoi (https://qoiformat.org)
//      pixelscalers (https://github.com/janert/pixelscalers)
//      mmpx (https://github.com/ITotalJustice/mmpx)
//
//-----------------------------------------------------------------------------------------------------

// Set this to 1 if we want to print debug messages to stderr
#define IMAGE_DEBUG 0
#include "image.h"
// We need 'qbs' and 'image' structs stuff from here
// This should eventually change when things are moved to smaller, logical and self-contained files
#include "../../../libqb.h"
#include "filepath.h"
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>
#define DR_PCX_IMPLEMENTATION
#include "dr_pcx.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define QOI_IMPLEMENTATION
#include "qoi.h"
#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"
#define SXBR_IMPLEMENTATION
#include "pixelscalers/sxbr.hpp"
#define MMPX_IMPLEMENTATION
#include "pixelscalers/mmpx.hpp"
#define HQX_IMPLEMENTATION
#include "pixelscalers/hqx.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#ifdef QB64_WINDOWS
#    define ZERO_VARIABLE(_v_) ZeroMemory(&(_v_), sizeof(_v_))
#else
#    define ZERO_VARIABLE(_v_) memset(&(_v_), 0, sizeof(_v_))
#endif

#define GET_ARRAY_SIZE(_x_) (sizeof(_x_) / sizeof(_x_[0]))

extern const img_struct *img;                 // used by func__loadimage
extern const img_struct *write_page;          // used by func__loadimage
extern const uint32_t palette_256[];          // used by func__loadimage
extern const int32_t *page;                   // used by func__saveimage
extern const int32_t nextimg;                 // used by func__saveimage
extern const uint8_t charset8x8[256][8][8];   // used by func__saveimage
extern const uint8_t charset8x16[256][16][8]; // used by func__saveimage

/// @brief Pixel scaler algorithms
enum class ImageScaler { NONE = 0, SXBR2, MMPX2, HQ2XA, HQ2XB, HQ3XA, HQ3XB };
/// @brief This is the scaling factors for ImageScaler enum
static const int g_ImageScaleFactor[] = {1, 2, 2, 2, 2, 3, 3};
/// @brief Pixel scaler names for ImageScaler enum
static const char *g_ImageScalerName[] = {"NONE", "SXBR2", "MMPX2", "HQ2XA", "HQ2XB", "HQ3XA", "HQ3XB"};

/// @brief Runs a pixel scaler algorithm on raw image pixels. It will free 'data' if scaling occurs!
/// @param data In + Out: The source raw image data in RGBA format
/// @param xOut In + Out: The image width
/// @param yOut In + Out: The image height
/// @param scaler The scaler algorithm to use
/// @return A pointer to the scaled image or 'data' if there is no change
static uint32_t *image_scale(uint32_t *data, int32_t *xOut, int32_t *yOut, ImageScaler scaler) {
    if (scaler > ImageScaler::NONE) {
        auto newX = *xOut * g_ImageScaleFactor[(int)(scaler)];
        auto newY = *yOut * g_ImageScaleFactor[(int)(scaler)];

        auto pixels = (uint32_t *)malloc(sizeof(uint32_t) * newX * newY);
        if (pixels) {
            IMAGE_DEBUG_PRINT("Scaler %i: (%i x %i) -> (%i x %i)", scaler, *xOut, *yOut, newX, newY);

            switch (scaler) {
            case ImageScaler::SXBR2:
                scaleSuperXBR2(data, *xOut, *yOut, pixels);
                break;

            case ImageScaler::MMPX2:
                mmpx_scale2x(data, pixels, *xOut, *yOut);
                break;

            case ImageScaler::HQ2XA:
                hq2xA(data, *xOut, *yOut, pixels);
                break;

            case ImageScaler::HQ2XB:
                hq2xB(data, *xOut, *yOut, pixels);
                break;

            case ImageScaler::HQ3XA:
                hq3xA(data, *xOut, *yOut, pixels);
                break;

            case ImageScaler::HQ3XB:
                hq3xB(data, *xOut, *yOut, pixels);
                break;

            default:
                IMAGE_DEBUG_PRINT("Unsupported scaler %i", scaler);
                free(pixels);
                return data;
            }

            free(data);
            data = pixels;
            *xOut = newX;
            *yOut = newY;
        }
    }

    return data;
}

/// @brief This is internally used by image_svg_load_from_file() and image_svg_load_fron_memory(). It always frees 'image' once done!
/// @param image nanosvg image object pointer
/// @param xOut Out: width in pixels. This cannot be NULL
/// @param yOut Out: height in pixels. This cannot be NULL
/// @param scaler An optional pixel scaler to use (it just used this to scale internally)
/// @param components Out: color channels. This cannot be NULL
/// @param isVG Out: vector graphics? Always set to true
/// @return A pointer to the raw pixel data in RGBA format or NULL on failure
static uint32_t *image_svg_load(NSVGimage *image, int32_t *xOut, int32_t *yOut, ImageScaler scaler, int *components, bool *isVG) {
    auto rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        return nullptr;
    }

    auto w = (int32_t)image->width * g_ImageScaleFactor[(int)(scaler)];
    auto h = (int32_t)image->height * g_ImageScaleFactor[(int)(scaler)];

    auto pixels = (uint32_t *)malloc(sizeof(uint32_t) * w * h);
    if (!pixels) {
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
        return nullptr;
    }

    nsvgRasterize(rast, image, 0, 0, g_ImageScaleFactor[(int)(scaler)], reinterpret_cast<unsigned char *>(pixels), w, h, sizeof(uint32_t) * w);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    *xOut = w;
    *yOut = h;
    *components = sizeof(uint32_t);
    *isVG = true;
    return pixels;
}

/// @brief Loads an SVG image file from disk
/// @param fileName The file path name to load
/// @param xOut Out: width in pixels. This cannot be NULL
/// @param yOut Out: height in pixels. This cannot be NULL
/// @param scaler An optional pixel scaler to use (it just used this to scale internally)
/// @param components Out: color channels. This cannot be NULL
/// @param isVG Out: vector graphics? Always set to true
/// @return A pointer to the raw pixel data in RGBA format or NULL on failure
static uint32_t *image_svg_load_from_file(const char *fileName, int32_t *xOut, int32_t *yOut, ImageScaler scaler, int *components, bool *isVG) {
    if (!filepath_has_extension(fileName, "svg"))
        return nullptr;

    auto fp = fopen(fileName, "rb");
    if (!fp)
        return nullptr;

    fseek(fp, 0, SEEK_END);
    auto size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    auto svgString = (char *)malloc(size + 1);
    if (!svgString) {
        fclose(fp);
        return nullptr;
    }

    if (fread(svgString, 1, size, fp) != size) {
        free(svgString);
        fclose(fp);
        return nullptr;
    }
    svgString[size] = '\0'; // must be null terminated

    fclose(fp);

    // Check if it has a valid SVG start tag
    if (!strstr(svgString, "<svg")) {
        free(svgString);
        return nullptr;
    }

    auto image = nsvgParse(svgString, "px", 96.0f); // important note: changes the string
    if (!image) {
        free(svgString);
        return nullptr;
    }

    auto pixels = image_svg_load(image, xOut, yOut, scaler, components, isVG); // this is where everything else is freed
    free(svgString);

    return pixels;
}

/// @brief Loads an SVG image file from memory
/// @param buffer The raw pointer to the file in memory
/// @param size The size of the file in memory
/// @param xOut Out: width in pixels. This cannot be NULL
/// @param yOut Out: height in pixels. This cannot be NULL
/// @param scaler An optional pixel scaler to use (it just used this to scale internally)
/// @param components Out: color channels. This cannot be NULL
/// @param isVG Out: vector graphics? Always set to true
/// @return A pointer to the raw pixel data in RGBA format or NULL on failure
static uint32_t *image_svg_load_from_memory(const uint8_t *buffer, size_t size, int32_t *xOut, int32_t *yOut, ImageScaler scaler, int *components, bool *isVG) {
    auto svgString = (char *)malloc(size + 1);
    if (!svgString)
        return nullptr;

    memcpy(svgString, buffer, size);
    svgString[size] = '\0'; // must be null terminated

    // Check if it has a valid SVG start tag
    if (!strstr(svgString, "<svg")) {
        free(svgString);
        return nullptr;
    }

    auto image = nsvgParse(svgString, "px", 96.0f); // important note: changes the string
    if (!image) {
        free(svgString);
        return nullptr;
    }

    auto pixels = image_svg_load(image, xOut, yOut, scaler, components, isVG); // this is where everything else is freed
    free(svgString);

    return pixels;
}

/// @brief Loads a QOI image file from disk
/// @param fileName The file path name to load
/// @param xOut Out: width in pixels. This cannot be NULL
/// @param yOut Out: height in pixels. This cannot be NULL
/// @param components Out: color channels. This cannot be NULL
/// @return A pointer to the raw pixel data in RGBA format or NULL on failure
static uint32_t *image_qoi_load_from_file(const char *fileName, int32_t *xOut, int32_t *yOut, int *components) {
    qoi_desc desc;
    auto pixels = reinterpret_cast<uint32_t *>(qoi_read(fileName, &desc, sizeof(uint32_t)));
    if (pixels) {
        *xOut = desc.width;
        *yOut = desc.height;
        *components = desc.channels;
    }
    return pixels;
}

/// @brief Loads a QOI image file from memory
/// @param buffer The raw pointer to the file in memory
/// @param size The size of the file in memory
/// @param xOut Out: width in pixels. This cannot be NULL
/// @param yOut Out: height in pixels. This cannot be NULL
/// @param components Out: color channels. This cannot be NULL
/// @return A pointer to the raw pixel data in RGBA format or NULL on failure
static uint32_t *image_qoi_load_from_memory(const uint8_t *buffer, size_t size, int32_t *xOut, int32_t *yOut, int *components) {
    qoi_desc desc;
    auto pixels = reinterpret_cast<uint32_t *>(qoi_decode(buffer, size, &desc, sizeof(uint32_t)));
    if (pixels) {
        *xOut = desc.width;
        *yOut = desc.height;
        *components = desc.channels;
    }
    return pixels;
}

/// @brief Decodes an image file from a file using the dr_pcx & stb_image libraries.
/// @param fileName A valid filename
/// @param xOut Out: width in pixels. This cannot be NULL
/// @param yOut Out: height in pixels. This cannot be NULL
/// @param scaler An optional pixel scaler to use
/// @return A pointer to the raw pixel data in RGBA format or NULL on failure
static uint32_t *image_decode_from_file(const char *fileName, int32_t *xOut, int32_t *yOut, ImageScaler scaler) {
    auto compOut = 0;
    auto isVG = false; // we will not use scalers for vector graphics

    IMAGE_DEBUG_PRINT("Loading image from file %s", fileName);

    // Attempt to load file as a PCX first using dr_pcx
    auto pixels = reinterpret_cast<uint32_t *>(drpcx_load_file(fileName, DRPCX_FALSE, xOut, yOut, &compOut, 4));
    IMAGE_DEBUG_PRINT("Image dimensions (dr_pcx) = (%i, %i)", *xOut, *yOut);
    if (!pixels) {
        // If dr_pcx failed to load, then use stb_image
        pixels = reinterpret_cast<uint32_t *>(stbi_load(fileName, xOut, yOut, &compOut, 4));
        IMAGE_DEBUG_PRINT("Image dimensions (stb_image) = (%i, %i)", *xOut, *yOut);

        if (!pixels) {
            pixels = image_qoi_load_from_file(fileName, xOut, yOut, &compOut);
            IMAGE_DEBUG_PRINT("Image dimensions (qoi) = (%i, %i)", *xOut, *yOut);

            if (!pixels) {
                pixels = image_svg_load_from_file(fileName, xOut, yOut, scaler, &compOut, &isVG);
                IMAGE_DEBUG_PRINT("Image dimensions (nanosvg) = (%i, %i)", *xOut, *yOut);

                if (!pixels)
                    return nullptr; // Return NULL if all attempts failed
            }
        }
    }

    IMAGE_DEBUG_CHECK(compOut > 2);

    if (!isVG)
        pixels = image_scale(pixels, xOut, yOut, scaler);

    return pixels;
}

/// @brief Decodes an image file from memory using the dr_pcx & stb_image libraries
/// @param data The raw pointer to the file in memory
/// @param size The size of the file in memory
/// @param xOut Out: width in pixels. This cannot be NULL
/// @param yOut Out: height in pixels. This cannot be NULL
/// @param scaler An optional pixel scaler to use
/// @return A pointer to the raw pixel data in RGBA format or NULL on failure
static uint32_t *image_decode_from_memory(const uint8_t *data, size_t size, int32_t *xOut, int32_t *yOut, ImageScaler scaler) {
    auto compOut = 0;
    auto isVG = false; // we will not use scalers for vector graphics

    IMAGE_DEBUG_PRINT("Loading image from memory");

    // Attempt to load file as a PCX first using dr_pcx
    auto pixels = reinterpret_cast<uint32_t *>(drpcx_load_memory(data, size, DRPCX_FALSE, xOut, yOut, &compOut, 4));
    IMAGE_DEBUG_PRINT("Image dimensions (dr_pcx) = (%i, %i)", *xOut, *yOut);
    if (!pixels) {
        // If dr_pcx failed to load, then use stb_image
        pixels = reinterpret_cast<uint32_t *>(stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(data), size, xOut, yOut, &compOut, 4));
        IMAGE_DEBUG_PRINT("Image dimensions (stb_image) = (%i, %i)", *xOut, *yOut);

        if (!pixels) {
            pixels = image_qoi_load_from_memory(data, size, xOut, yOut, &compOut);
            IMAGE_DEBUG_PRINT("Image dimensions (qoi) = (%i, %i)", *xOut, *yOut);

            if (!pixels) {
                pixels = image_svg_load_from_memory(data, size, xOut, yOut, scaler, &compOut, &isVG);
                IMAGE_DEBUG_PRINT("Image dimensions (nanosvg) = (%i, %i)", *xOut, *yOut);

                if (!pixels)
                    return nullptr; // Return NULL if all attempts failed
            }
        }
    }

    IMAGE_DEBUG_CHECK(compOut > 2);

    if (!isVG)
        pixels = image_scale(pixels, xOut, yOut, scaler);

    return pixels;
}

/// @brief This takes in a 32bpp (BGRA) image raw data and spits out an 8bpp raw image along with it's 256 color (BGRA) palette.
/// @param src32 The source raw image data. This must be in BGRA format and not NULL
/// @param w The width of the image in pixels
/// @param h The height of the image in pixels
/// @param paletteOut A 256 color palette if the operation was successful. This cannot be NULL
/// @return A pointer to a 8bpp raw image or NULL if operation failed
static uint8_t *image_convert_8bpp(const uint32_t *src32, int32_t w, int32_t h, uint32_t *paletteOut) {
    static struct {
        uint32_t r, g, b;
        uint32_t count;
    } cubes[256];

    // https://en.wikipedia.org/wiki/Ordered_dithering
    static const uint8_t bayerMatrix[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

    IMAGE_DEBUG_PRINT("Converting 32bpp image (%i, %i) to 8bpp", w, h);

    // Allocate memory for new image (8-bit indexed)
    auto pixels = (uint8_t *)malloc(w * h);
    if (!pixels) {
        return nullptr;
    }

    ZERO_VARIABLE(cubes);

    // Quantization phase
    auto src = reinterpret_cast<const uint8_t *>(src32);
    auto dst = pixels;
    for (auto y = 0; y < h; y++) {
        for (auto x = 0; x < w; x++) {
            int32_t t = bayerMatrix[((y & 3) << 2) + (x & 3)];
            int32_t b = image_clamp_color_component((*src++) + (t << 1));
            int32_t g = image_clamp_color_component((*src++) + (t << 1));
            int32_t r = image_clamp_color_component((*src++) + (t << 1));
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
            paletteOut[i] = image_make_bgra(cubes[i].r / cubes[i].count, cubes[i].g / cubes[i].count, cubes[i].b / cubes[i].count, 0xFF);
        } else {
            paletteOut[i] = image_make_bgra(0, 0, 0, 0xFF);
        }
    }

    return pixels;
}

/// @brief This takes in a 32bpp (BGRA) image raw data and spits out an 8bpp raw image along with it's 256 color (BGRA) palette.
/// If the number of unique colors in the 32bpp image > 256, then the functions returns a NULL.
/// Unlike image_convert_8bpp(), no 'real' conversion takes place.
/// @param src The source raw image data. This must be in BGRA format and not NULL
/// @param w The width of the image in pixels
/// @param h The height of the image in pixels
/// @param paletteOut A 256 color palette if the operation was successful. This cannot be NULL
/// @return A pointer to a 8bpp raw image or NULL if operation failed
static uint8_t *image_extract_8bpp(const uint32_t *src, int32_t w, int32_t h, uint32_t *paletteOut) {
    IMAGE_DEBUG_PRINT("Extracting 8bpp image (%i, %i) from 32bpp", w, h);

    std::unordered_map<uint32_t, int> colorMap;

    // Allocate memory for new image (8-bit indexed)
    auto pixels = (uint8_t *)malloc(w * h);
    if (!pixels)
        return nullptr;

    auto uniqueColors = 0; // as long as this is < 256 we will keep going until we are done
    size_t size = w * h;
    for (auto i = 0; i < size; i++) {
        auto srcColor = src[i]; // get the 32bpp pixel

        // Check if the src color exists in our palette
        if (colorMap.count(srcColor) == 0) {
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

/// @brief This modifies an *8bpp* image 'src' to use 'dst_pal' instead of 'src_pal'
/// @param src A pointer to the 8bpp image pixel data. This modifies data 'src' points to and cannot be NULL
/// @param w The width of the image in pixels
/// @param h The height of the image in pixels
/// @param src_pal The image's original palette. This cannot be NULL
/// @param dst_pal The destination palette. This cannot be NULL
static void image_remap_palette(uint8_t *src, int32_t w, int32_t h, const uint32_t *src_pal, const uint32_t *dst_pal) {
    auto const maxRGBDist = image_calculate_rgb_distance(0, 0, 0, 255, 255, 255); // The farthest we can go in the color cube
    static uint32_t palMap[256];
    ZERO_VARIABLE(palMap);

    IMAGE_DEBUG_PRINT("Remapping 8bpp image (%i, %i) palette", w, h);

    // Match the palette
    for (auto x = 0; x < 256; x++) {
        auto oldDist = maxRGBDist;

        for (auto y = 0; y < 256; y++) {
            auto newDist = image_calculate_rgb_distance(image_get_bgra_red(src_pal[x]), image_get_bgra_green(src_pal[x]), image_get_bgra_blue(src_pal[x]),
                                                        image_get_bgra_red(dst_pal[y]), image_get_bgra_green(dst_pal[y]), image_get_bgra_blue(dst_pal[y]));

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

/// @brief This function loads an image into memory and returns valid LONG image handle values that are less than -1
/// @param qbsFileName The filename or memory buffer (see requirements below) of the image
/// @param bpp 32 = 32bpp, 33 = 32bpp (hardware accelerated), 256=8bpp or 257=8bpp (without palette remap)
/// @param qbsRequirements A qbs that can contain one or more of: hardware, memory, adaptive
/// @param passed How many parameters were passed?
/// @return Valid LONG image handle values that are less than -1 or -1 on failure
int32_t func__loadimage(qbs *qbsFileName, int32_t bpp, qbs *qbsRequirements, int32_t passed) {
    if (new_error || !qbsFileName->len) // leave if we do not have a file name, data or there was an error
        return INVALID_IMAGE_HANDLE;

    auto isLoadFromMemory = false;   // should the image be loaded from memory?
    auto isHardwareImage = false;    // should the image be converted to a hardware image?
    auto isRemapPalette = true;      // should the palette be re-mapped to the QB64 default palette?
    auto scaler = ImageScaler::NONE; // default to no scaling

    // Handle special cases and set the above flags if required
    IMAGE_DEBUG_PRINT("bpp = %i, passed = 0x%X", bpp, passed);
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
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
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
    if ((passed & 2) && qbsRequirements->len) {
        // Parse the requirements string and setup save settings
        std::string requirements(reinterpret_cast<char *>(qbsRequirements->chr), qbsRequirements->len);
        std::transform(requirements.begin(), requirements.end(), requirements.begin(), [](unsigned char c) { return std::toupper(c); });

        IMAGE_DEBUG_PRINT("Parsing requirements string: %s", requirements.c_str());

        if (requirements.find("HARDWARE") != std::string::npos && bpp == 32) {
            isHardwareImage = true;
            IMAGE_DEBUG_PRINT("Hardware image selected");
        } else if (requirements.find("ADAPTIVE") != std::string::npos && bpp == 256) {
            isRemapPalette = false;
            IMAGE_DEBUG_PRINT("Adaptive palette selected");
        }

        if (requirements.find("MEMORY") != std::string::npos) {
            isLoadFromMemory = true;
            IMAGE_DEBUG_PRINT("Loading image from memory");
        }

        // Parse scaler string
        for (auto i = 0; i < GET_ARRAY_SIZE(g_ImageScalerName); i++) {
            IMAGE_DEBUG_PRINT("Checking for: %s", g_ImageScalerName[i]);
            if (requirements.find(g_ImageScalerName[i]) != std::string::npos) {
                scaler = (ImageScaler)i;
                IMAGE_DEBUG_PRINT("%s scaler selected", g_ImageScalerName[(int)scaler]);
                break;
            }
        }
    }

    auto x = 0, y = 0;
    uint32_t *pixels;

    if (isLoadFromMemory) {
        pixels = image_decode_from_memory(qbsFileName->chr, qbsFileName->len, &x, &y, scaler);
    } else {
        std::string fileName(reinterpret_cast<char *>(qbsFileName->chr), qbsFileName->len);
        pixels = image_decode_from_file(filepath_fix_directory(fileName), &x, &y, scaler);
    }

    if (!pixels)
        return INVALID_IMAGE_HANDLE; // Return invalid handle if loading the image failed

    // Convert RGBA to BGRA
    size_t size = x * y;
    for (auto i = 0; i < size; i++)
        pixels[i] = image_swap_red_blue(pixels[i]);

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

        auto pixels256 = image_extract_8bpp(pixels, x, y, palette); // Try to simply 'extract' the 8bpp image first
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
        memcpy(img[-i].offset32, pixels, size * sizeof(uint32_t));
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

/// @brief Saves an image to the disk from a QB64-PE image handle
/// @param qbsFileName The file path name to save to
/// @param imageHandle Optional: The image handle. If omitted, then this is _DISPLAY()
/// @param qbsRequirements Optional: Extra format and setting arguments
/// @param passed Optional parameters
void sub__saveimage(qbs *qbsFileName, int32_t imageHandle, qbs *qbsRequirements, int32_t passed) {
    enum struct SaveFormat { PNG = 0, QOI, BMP, TGA, JPG, HDR };
    static const char *formatName[] = {"png", "qoi", "bmp", "tga", "jpg", "hdr"};

    if (new_error) // leave if there was an error
        return;

    if (!qbsFileName->len) { // empty file names not allowed
        IMAGE_DEBUG_PRINT("Empty file name");
        error(QB_ERROR_BAD_FILE_NAME);
        return;
    }

    if (passed & 1) {
        // Check and validate image handle
        IMAGE_DEBUG_PRINT("Validating handle %i", imageHandle);

        if (imageHandle >= 0) {
            validatepage(imageHandle);
            imageHandle = page[imageHandle];
        } else {
            imageHandle = -imageHandle;

            if (imageHandle >= nextimg) {
                error(QB_ERROR_INVALID_HANDLE);
                return;
            }
            if (!img[imageHandle].valid) {
                error(QB_ERROR_INVALID_HANDLE);
                return;
            }
        }
    } else {
        // Use default image handle
        IMAGE_DEBUG_PRINT("Using default handle");

        imageHandle = -func__display();

        // Safety
        if (imageHandle >= nextimg) {
            error(QB_ERROR_INVALID_HANDLE);
            return;
        }
        if (!img[imageHandle].valid) {
            error(QB_ERROR_INVALID_HANDLE);
            return;
        }
    }

    IMAGE_DEBUG_PRINT("Using image handle %i", imageHandle);

    auto format = SaveFormat::PNG; // we always default to PNG

    if ((passed & 2) && qbsRequirements->len) {
        // Parse the requirements string and setup save settings
        std::string requirements(reinterpret_cast<char *>(qbsRequirements->chr), qbsRequirements->len);
        std::transform(requirements.begin(), requirements.end(), requirements.begin(), [](unsigned char c) { return std::tolower(c); });

        IMAGE_DEBUG_PRINT("Parsing requirements string: %s", requirements.c_str());

        for (auto i = 0; i < GET_ARRAY_SIZE(formatName); i++) {
            IMAGE_DEBUG_PRINT("Checking for: %s", formatName[i]);
            if (requirements.find(formatName[i]) != std::string::npos) {
                format = (SaveFormat)i;
                IMAGE_DEBUG_PRINT("Found: %s", formatName[(int)format]);
                break;
            }
        }
    }

    IMAGE_DEBUG_PRINT("Format selected: %s", formatName[(int)format]);

    std::string fileName(reinterpret_cast<char *>(qbsFileName->chr), qbsFileName->len);
    filepath_fix_directory(fileName);

    // Check if fileName has a valid extension and add one if it does not have one
    if (fileName.length() > 4) { // must be at least n.ext
        auto fileExtension = fileName.substr(fileName.length() - 4);
        std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), [](unsigned char c) { return std::tolower(c); });

        IMAGE_DEBUG_PRINT("File extension: %s", fileExtension.c_str());

        int i;
        for (i = 0; i < GET_ARRAY_SIZE(formatName); i++) {
            std::string formatExtension;

            formatExtension = ".";
            formatExtension.append(formatName[i]);

            IMAGE_DEBUG_PRINT("Check extension name: %s", formatExtension.c_str());

            if (fileExtension == formatExtension) {
                IMAGE_DEBUG_PRINT("Extension (%s) matches with format %i", formatExtension.c_str(), i);
                format = (SaveFormat)i;
                IMAGE_DEBUG_PRINT("Format selected by extension: %s", formatName[(int)format]);
                break;
            }
        }

        if (i >= GET_ARRAY_SIZE(formatName)) { // no matches
            IMAGE_DEBUG_PRINT("No matching extension. Adding .%s", formatName[(int)format]);

            fileName.append(".");
            fileName.append(formatName[(int)format]);
        }
    } else {
        // Simply add the selected format's extension
        IMAGE_DEBUG_PRINT("Adding extension: .%s", formatName[(int)format]);

        fileName.append(".");
        fileName.append(formatName[(int)format]);
    }

    // This will hold our raw RGBA pixel data
    std::vector<uint32_t> pixels;
    int32_t width, height;

    if (img[imageHandle].text) {
        IMAGE_DEBUG_PRINT("Rendering text surface to RGBA");

        auto const fontWidth = 8;
        auto fontHeight = 16;
        if (img[imageHandle].font == 8 || img[imageHandle].font == 14)
            fontHeight = img[imageHandle].font;

        width = fontWidth * img[imageHandle].width;
        height = fontHeight * img[imageHandle].height;

        pixels.resize(width * height);

        uint8_t fc, bc, *c = img[imageHandle].offset; // set to the first codepoint
        uint8_t const *builtinFont = nullptr;

        // Render all text to the raw pixel array
        for (auto y = 0; y < height; y += fontHeight) {
            for (auto x = 0; x < width; x += fontWidth) {
                switch (fontHeight) {
                case 8:
                    builtinFont = &charset8x8[*c][0][0];
                    break;

                case 14:
                    builtinFont = &charset8x16[*c][1][0];
                    break;

                default: // 16
                    builtinFont = &charset8x16[*c][0][0];
                }

                ++c; // move to the attribute
                fc = *c & 0x0F;
                bc = ((*c >> 4) & 7) + ((*c >> 7) << 3);

                // Inner codepoint rendering loop
                for (auto dy = y, py = 0; py < fontHeight; dy++, py++) {
                    for (auto dx = x, px = 0; px < fontWidth; dx++, px++) {
                        pixels[width * dy + dx] = image_swap_red_blue(*builtinFont ? img[imageHandle].pal[fc] : img[imageHandle].pal[bc]);
                        ++builtinFont;
                    }
                }

                ++c; // move to the next codepoint
            }
        }
    } else {
        width = img[imageHandle].width;
        height = img[imageHandle].height;

        pixels.resize(width * height);

        if (img[imageHandle].bits_per_pixel == 32) { // BGRA pixels
            IMAGE_DEBUG_PRINT("Converting BGRA surface to RGBA");

            auto p = img[imageHandle].offset32;

            for (size_t i = 0; i < pixels.size(); i++) {
                pixels[i] = image_swap_red_blue(*p);
                ++p;
            }
        } else { // indexed pixels
            IMAGE_DEBUG_PRINT("Converting BGRA indexed surface to RGBA");
            auto p = img[imageHandle].offset;

            for (size_t i = 0; i < pixels.size(); i++) {
                pixels[i] = image_swap_red_blue(img[imageHandle].pal[*p]);
                ++p;
            }
        }
    }

    IMAGE_DEBUG_PRINT("Saving to: %s (%i x %i), %llu pixels, %s", fileName.c_str(), width, height, pixels.size(), formatName[(int)format]);

    switch (format) {
    case SaveFormat::PNG: {
        stbi_write_png_compression_level = 100;
        if (!stbi_write_png(fileName.c_str(), width, height, sizeof(uint32_t), pixels.data(), 0)) {
            IMAGE_DEBUG_PRINT("stbi_write_png() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::QOI: {
        qoi_desc desc;
        desc.width = width;
        desc.height = height;
        desc.channels = sizeof(uint32_t);
        desc.colorspace = QOI_SRGB;

        if (!qoi_write(fileName.c_str(), pixels.data(), &desc)) {
            IMAGE_DEBUG_PRINT("qoi_write() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::BMP: {
        if (!stbi_write_bmp(fileName.c_str(), width, height, sizeof(uint32_t), pixels.data())) {
            IMAGE_DEBUG_PRINT("stbi_write_bmp() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::TGA: {
        if (!stbi_write_tga(fileName.c_str(), width, height, sizeof(uint32_t), pixels.data())) {
            IMAGE_DEBUG_PRINT("stbi_write_tga() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::JPG: {
        if (!stbi_write_jpg(fileName.c_str(), width, height, sizeof(uint32_t), pixels.data(), 100)) {
            IMAGE_DEBUG_PRINT("stbi_write_jpg() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::HDR: {
        IMAGE_DEBUG_PRINT("Converting RGBA to linear float data");

        const auto HDRComponents = 4;

        std::vector<float> HDRPixels;
        HDRPixels.resize(pixels.size() * HDRComponents);

        for (size_t j = 0, i = 0; i < pixels.size(); i++) {
            HDRPixels[j] = pow((pixels[i] & 0xFFu) / 255.0f, 2.2f);
            ++j;
            HDRPixels[j] = pow(((pixels[i] >> 8) & 0xFFu) / 255.0f, 2.2f);
            ++j;
            HDRPixels[j] = pow(((pixels[i] >> 16) & 0xFFu) / 255.0f, 2.2f);
            ++j;
            HDRPixels[j] = (pixels[i] >> 24) / 255.0f;
            ++j;
        }

        if (!stbi_write_hdr(fileName.c_str(), width, height, HDRComponents, HDRPixels.data())) {
            IMAGE_DEBUG_PRINT("stbi_write_hdr() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    default:
        IMAGE_DEBUG_PRINT("Save handler not implemented");
        error(QB_ERROR_INTERNAL_ERROR);
    }
}
