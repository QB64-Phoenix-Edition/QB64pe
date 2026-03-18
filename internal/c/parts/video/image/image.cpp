//-----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___   ___                       _    _ _
//   / _ \| _ ) / /| | || _ \ __| |_ _|_ __  __ _ __ _ ___  | |  (_) |__ _ _ __ _ _ _ _  _
//  | (_) | _ \/ _ \_  _|  _/ _|   | || '  \/ _` / _` / -_) | |__| | '_ \ '_/ _` | '_| || |
//   \__\_\___/\___/ |_||_| |___| |___|_|_|_\__,_\__, \___| |____|_|_.__/_| \__,_|_|  \_, |
//                                               |___/                                |__/
//
//  Powered by:
//      stb_image & stb_image_write (https://github.com/nothings/stb)
//      tiny_webp (https://github.com/justus2510/tiny-webp)
//      nanosvg (https://github.com/memononen/nanosvg)
//      qoi (https://qoiformat.org)
//      sg_curico & sg_pcx (https://github.com/a740g)
//      jo_gif (https://www.jonolick.com/code)
//      pixelscalers (https://github.com/janert/pixelscalers)
//      mmpx (https://github.com/ITotalJustice/mmpx)
//
//-----------------------------------------------------------------------------------------------------

#include "../../../libqb.h"
#include "error_handle.h"
#include "framework.hpp"
#include "graphics.h"
#include "jo_gif/jo_gif.h"
#include "libqb-common.h"
#include "qbs.h"
#include "stb/stb_image_write.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

extern const img_struct *img;                 // used by func__loadimage
extern const img_struct *write_page;          // used by func__loadimage
extern const uint32_t palette_256[];          // used by func__loadimage
extern const int32_t *page;                   // used by func__saveimage
extern const int32_t nextimg;                 // used by func__saveimage
extern const uint8_t charset8x8[256][8][8];   // used by func__saveimage
extern const uint8_t charset8x16[256][16][8]; // used by func__saveimage

/// @brief This takes in a 32bpp (BGRA) image raw data and spits out an 8bpp raw image along with it's 256 color (BGRA) palette.
/// @param src32 The source raw image data. This must be in BGRA format and not NULL.
/// @param srcPalette An optional source palette to use (256 colors in BGRA format). If NULL, an adaptive palette is generated.
/// @param w The width of the image in pixels.
/// @param h The height of the image in pixels.
/// @param dst A pointer to the destination 8bpp raw image data. This cannot be NULL.
/// @param dstPalette A 256 color palette if the operation was successful. This cannot be NULL.
static void image_convert_8bpp(const uint32_t *src32, const uint32_t *srcPalette, int32_t w, int32_t h, uint8_t *dst, uint32_t *dstPalette) {
    image_log_info("Converting 32bpp image (%i, %i) to 8bpp", w, h);

    if (srcPalette) {
        image_log_trace("Using provided palette");

        memcpy(dstPalette, srcPalette, IMAGE_8BPP_MAX_COLORS * sizeof(uint32_t));
    } else {
        image_log_trace("Generating adaptive palette");

        const size_t maxUnique = 4096u;
        std::vector<uint32_t> histColors;
        histColors.reserve(maxUnique);
        std::vector<size_t> histCounts;
        histCounts.reserve(maxUnique);
        std::unordered_map<uint32_t, size_t> uniqueColorMap;

        auto stepX = std::max(w / 64, 1);
        auto stepY = std::max(h / 64, 1);

        for (int32_t y = 0; y < h; y += stepY) {
            for (int32_t x = 0; x < w; x += stepX) {
                auto c = src32[size_t(y) * size_t(w) + size_t(x)];
                auto it = uniqueColorMap.find(c);
                if (it != uniqueColorMap.end()) {
                    histCounts[it->second]++;
                } else if (uniqueColorMap.size() < maxUnique) {
                    auto newIndex = uniqueColorMap.size();
                    uniqueColorMap[c] = newIndex;
                    histColors.push_back(c);
                    histCounts.push_back(1);
                }
            }
        }

        auto uniqueCount = uniqueColorMap.size();
        if (uniqueCount == 0) {
            for (size_t i = 0; i < IMAGE_8BPP_MAX_COLORS; i++) {
                auto gray = uint8_t((255u * i) / (IMAGE_8BPP_MAX_COLORS - 1u));
                dstPalette[i] = image_make_bgr_gray(gray);
            }
        } else {
            size_t maxCount = 1;
            for (size_t i = 0; i < uniqueCount; i++) {
                if (histCounts[i] > maxCount) {
                    maxCount = histCounts[i];
                }
            }

            std::vector<bool> chosen(uniqueCount, false);
            std::vector<float> minDist2(uniqueCount, -1.0f);

            size_t firstIdx = 0;
            for (size_t i = 1; i < uniqueCount; i++) {
                if (histCounts[i] > histCounts[firstIdx]) {
                    firstIdx = i;
                }
            }

            dstPalette[0] = histColors[firstIdx];
            chosen[firstIdx] = true;
            size_t paletteUsed = 1;

            auto r1 = image_get_bgra_red(histColors[firstIdx]);
            auto g1 = image_get_bgra_green(histColors[firstIdx]);
            auto b1 = image_get_bgra_blue(histColors[firstIdx]);

            for (size_t j = 0; j < uniqueCount; j++) {
                if (!chosen[j]) {
                    minDist2[j] = image_get_rgb_redmean_dist_sq(r1, g1, b1, image_get_bgra_red(histColors[j]), image_get_bgra_green(histColors[j]),
                                                                image_get_bgra_blue(histColors[j]));
                }
            }

            while (paletteUsed < IMAGE_8BPP_MAX_COLORS) {
                auto bestScore = -1.0f;
                int32_t bestIdx = -1;

                for (size_t j = 0; j < uniqueCount; j++) {
                    if (!chosen[j]) {
                        auto popWeight = 1 + (histCounts[j] * 255) / maxCount;
                        auto d2 = minDist2[j] < 0.0f ? 0.0f : minDist2[j];
                        auto score = d2 * popWeight;

                        if (score > bestScore) {
                            bestScore = score;
                            bestIdx = int32_t(j);
                        }
                    }
                }

                if (bestIdx < 0) {
                    break;
                }

                dstPalette[paletteUsed] = histColors[bestIdx];
                chosen[bestIdx] = true;
                ++paletteUsed;

                r1 = image_get_bgra_red(histColors[bestIdx]);
                g1 = image_get_bgra_green(histColors[bestIdx]);
                b1 = image_get_bgra_blue(histColors[bestIdx]);

                for (size_t j = 0; j < uniqueCount; j++) {
                    if (!chosen[j]) {
                        float d2 = image_get_rgb_redmean_dist_sq(r1, g1, b1, image_get_bgra_red(histColors[j]), image_get_bgra_green(histColors[j]),
                                                                 image_get_bgra_blue(histColors[j]));
                        if (minDist2[j] < 0 || d2 < minDist2[j]) {
                            minDist2[j] = d2;
                        }
                    }
                }
            }

            for (size_t i = paletteUsed; i < IMAGE_8BPP_MAX_COLORS; i++) {
                dstPalette[i] = dstPalette[paletteUsed - 1];
            }
        }
    }

    image_log_trace("Applying Floyd-Steinberg dithering");

    struct ErrorPixel {
        float r, g, b;
    };

    std::vector<ErrorPixel> err(w, {0.0f, 0.0f, 0.0f});
    std::vector<ErrorPixel> nextErr(w, {0.0f, 0.0f, 0.0f});

    for (int32_t y = 0; y < h; y++) {
        std::fill(nextErr.begin(), nextErr.end(), ErrorPixel{0.0f, 0.0f, 0.0f});

        auto isSerpentine = (y & 1) != 0;
        int32_t xStart = isSerpentine ? (w - 1) : 0;
        int32_t xEnd = isSerpentine ? -1 : w;
        int32_t xStep = isSerpentine ? -1 : 1;

        for (int32_t x = xStart; x != xEnd; x += xStep) {
            auto i = size_t(y) * size_t(w) + size_t(x);
            auto srcPixel = src32[i];

            auto r = std::clamp(float(image_get_bgra_red(srcPixel)) + err[x].r, 0.0f, 255.0f);
            auto g = std::clamp(float(image_get_bgra_green(srcPixel)) + err[x].g, 0.0f, 255.0f);
            auto b = std::clamp(float(image_get_bgra_blue(srcPixel)) + err[x].b, 0.0f, 255.0f);

            auto closestIndex = image_find_closest_palette_color(uint8_t(r), uint8_t(g), uint8_t(b), dstPalette, image_get_rgb_redmean_dist_sq);

            dst[i] = uint8_t(closestIndex);

            auto qPixel = dstPalette[closestIndex];
            auto qR = float(image_get_bgra_red(qPixel));
            auto qG = float(image_get_bgra_green(qPixel));
            auto qB = float(image_get_bgra_blue(qPixel));

            auto er = r - qR;
            auto eg = g - qG;
            auto eb = b - qB;

            auto distribute = [&](int32_t xOffset, std::vector<ErrorPixel> &errorRow, float factor) {
                auto j = x + xOffset;
                if (j >= 0 && j < w) {
                    errorRow[j].r += er * factor;
                    errorRow[j].g += eg * factor;
                    errorRow[j].b += eb * factor;
                }
            };

            if (xStep == 1) {
                distribute(1, err, 7.0f / 16.0f);
                if (y + 1 < h) {
                    distribute(-1, nextErr, 3.0f / 16.0f);
                    distribute(0, nextErr, 5.0f / 16.0f);
                    distribute(1, nextErr, 1.0f / 16.0f);
                }
            } else {
                distribute(-1, err, 7.0f / 16.0f);
                if (y + 1 < h) {
                    distribute(1, nextErr, 3.0f / 16.0f);
                    distribute(0, nextErr, 5.0f / 16.0f);
                    distribute(-1, nextErr, 1.0f / 16.0f);
                }
            }
        }
        err.swap(nextErr);
    }
}

/// @brief This takes in a 32bpp (BGRA) image raw data and spits out an 8bpp raw image along with it's 256 color (BGRA) palette.
/// Unlike image_convert_8bpp(), no 'real' conversion takes place.
/// @param src32 The source raw image data. This must be in BGRA format and not NULL.
/// @param srcPalette An optional source palette to map colors from (256 colors in BGRA format). If NULL, no mapping is done.
/// @param w The width of the image in pixels.
/// @param h The height of the image in pixels.
/// @param dst A pointer to the destination 8bpp raw image data. This cannot be NULL.
/// @param dstPalette A 256 color palette if the operation was successful. This cannot be NULL.
/// @return true if successful, or false if the image has more than 256 unique colors.
static bool image_extract_8bpp(const uint32_t *src32, const uint32_t *srcPalette, int32_t w, int32_t h, uint8_t *dst, uint32_t *dstPalette) {
    image_log_info("Extracting 8bpp image (%i, %i) from 32bpp", w, h);

    auto imageSize = size_t(w) * size_t(h);

    std::unordered_map<uint32_t, uint8_t> colorMap;
    size_t uniqueColors = 0;

    for (size_t i = 0; i < imageSize; i++) {
        auto c = src32[i];
        auto it = colorMap.find(c);

        if (it == colorMap.end()) {
            if (uniqueColors >= IMAGE_8BPP_MAX_COLORS) {
                image_log_info("Image has more than %i unique colors", uniqueColors);
                return false;
            }

            dstPalette[uniqueColors] = c;
            it = colorMap.emplace_hint(it, c, uniqueColors);
            ++uniqueColors;
        }

        dst[i] = it->second;
    }

    image_log_trace("Unique colors = %i", uniqueColors);

    if (srcPalette) {
        image_log_trace("Remapping 8bpp image (%i, %i) palette", w, h);

        std::vector<uint8_t> palMap(IMAGE_8BPP_MAX_COLORS, 0);

        for (size_t i = 0; i < uniqueColors; i++) {
            palMap[i] = uint8_t(image_find_closest_palette_color(dstPalette[i], srcPalette, image_get_rgb_euclidean_dist_sq));
        }

        auto p = dst;
        auto pEnd = dst + imageSize;
        while (p < pEnd) {
            *p = palMap[*p];
            ++p;
        }

        memcpy(dstPalette, srcPalette, IMAGE_8BPP_MAX_COLORS * sizeof(uint32_t));
    }

    return true;
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

    auto isLoadFromMemory = false;                // should the image be loaded from memory?
    auto isHardwareImage = false;                 // should the image be converted to a hardware image?
    auto srcPalette = palette_256;                // use the QB64 256 color palette by default for 8bpp images
    const ImageScalerBase *scaler = &gScalerNone; // default to no scaling

    // Handle special cases and set the above flags if required
    image_log_trace("bpp = %i, passed = 0x%X", bpp, passed);
    if (passed & 1) {
        if (bpp == 33) { // hardware image?
            isHardwareImage = true;
            bpp = 32;
            image_log_trace("bpp = %i", bpp);
        } else if (bpp == 257) { // adaptive palette?
            srcPalette = nullptr;
            bpp = 256;
            image_log_trace("bpp = %i", bpp);
        }

        if ((bpp != 32) && (bpp != 256)) { // invalid BPP?
            image_log_error("Invalid bpp (%i)", bpp);
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return INVALID_IMAGE_HANDLE;
        }
    } else {
        if (write_page->bits_per_pixel < 32) { // default to 8bpp for all legacy screen modes
            bpp = 256;
            image_log_trace("Defaulting to 8bpp");
        } else { // default to 32bpp for everything else
            bpp = 32;
            image_log_trace("Defaulting to 32bpp");
        }
    }

    // Check requirements string and set appropriate flags
    if ((passed & 2) && qbsRequirements->len) {
        // Parse the requirements string and setup save settings
        std::string requirements(reinterpret_cast<char *>(qbsRequirements->chr), qbsRequirements->len);
        std::transform(requirements.begin(), requirements.end(), requirements.begin(), ::toupper);

        image_log_trace("Parsing requirements string: %s", requirements.c_str());

        if (requirements.find("HARDWARE") != std::string::npos && bpp == 32) {
            isHardwareImage = true;
            image_log_trace("Hardware image selected");
        } else if (requirements.find("ADAPTIVE") != std::string::npos && bpp == 256) {
            srcPalette = nullptr;
            image_log_trace("Adaptive palette selected");
        }

        if (requirements.find("MEMORY") != std::string::npos) {
            isLoadFromMemory = true;
            image_log_trace("Loading image from memory");
        }

        // Parse scaler string
        for (size_t i = 0; i < ScalerCount; i++) {
            image_log_trace("Checking for: %s", gScalerRegistry[i].name);
            if (requirements.find(gScalerRegistry[i].name) != std::string::npos) {
                scaler = gScalerRegistry[i].scaler;
                image_log_trace("%s scaler selected", scaler->GetName());
                break;
            }
        }
    }

    Image loadedImage;

    if (isLoadFromMemory) {
        auto src = ImageSource::FromMemory(qbsFileName->chr, qbsFileName->len);
        loadedImage = Image::Load(src, scaler);
    } else {
        std::string fileName(reinterpret_cast<char *>(qbsFileName->chr), qbsFileName->len);
        auto src = ImageSource::FromFile(filepath_fix_directory(fileName));
        loadedImage = Image::Load(src, scaler);
    }

    if (!loadedImage.IsValid())
        return INVALID_IMAGE_HANDLE; // Return invalid handle if loading the image failed

    auto x = loadedImage.GetWidth();
    auto y = loadedImage.GetHeight();
    auto pixels = loadedImage.GetData();

    // Convert RGBA to BGRA
    size_t size = size_t(x) * size_t(y);
    image_swap_red_blue_buffer(pixels, size);

    auto i = func__newimage(x, y, bpp, 1);
    if (i == INVALID_IMAGE_HANDLE)
        return INVALID_IMAGE_HANDLE;

    // Convert image to 8bpp if requested by the user
    if (bpp == 256) {
        // Try to simply 'extract' the 8bpp image first. If that fails, then 'convert' it to 8bpp
        if (!image_extract_8bpp(pixels, srcPalette, x, y, img[-i].offset, img[-i].pal)) {
            image_convert_8bpp(pixels, srcPalette, x, y, img[-i].offset, img[-i].pal);
        }
    } else {
        memcpy(img[-i].offset32, pixels, size * sizeof(uint32_t));
    }

    // loadedImage goes out of scope here and frees pixel memory automatically

    // This only executes if bpp is 32
    if (isHardwareImage) {
        image_log_trace("Making hardware image");

        auto iHardware = func__copyimage(i, 33, 1);
        sub__freeimage(i, 1);
        i = iHardware;
    }

    image_log_trace("Returning handle value = %i", i);

    return i;
}

/// @brief Saves an image to the disk from a QB64-PE image handle
/// @param qbsFileName The file path name to save to
/// @param imageHandle Optional: The image handle. If omitted, then this is _DISPLAY()
/// @param qbsRequirements Optional: Extra format and setting arguments
/// @param passed Optional parameters
void sub__saveimage(qbs *qbsFileName, int32_t imageHandle, qbs *qbsRequirements, int32_t passed) {
    enum struct SaveFormat { PNG = 0, QOI, BMP, TGA, JPG, HDR, GIF, ICO };
    static const char *formatName[] = {"png", "qoi", "bmp", "tga", "jpg", "hdr", "gif", "ico"};

    if (new_error) // leave if there was an error
        return;

    if (!qbsFileName->len) { // empty file names not allowed
        image_log_error("Empty file name");
        error(QB_ERROR_BAD_FILE_NAME);
        return;
    }

    if (passed & 1) {
        // Check and validate image handle
        image_log_trace("Validating handle %i", imageHandle);

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
        image_log_trace("Using default handle");

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

    image_log_trace("Using image handle %i", imageHandle);

    auto format = SaveFormat::PNG; // we always default to PNG

    if ((passed & 2) && qbsRequirements->len) {
        // Parse the requirements string and setup save settings
        std::string requirements(reinterpret_cast<char *>(qbsRequirements->chr), qbsRequirements->len);
        std::transform(requirements.begin(), requirements.end(), requirements.begin(), ::tolower);

        image_log_trace("Parsing requirements string: %s", requirements.c_str());

        for (size_t i = 0; i < _countof(formatName); i++) {
            image_log_trace("Checking for: %s", formatName[i]);
            if (requirements.find(formatName[i]) != std::string::npos) {
                format = (SaveFormat)i;
                image_log_trace("Found: %s", formatName[size_t(format)]);
                break;
            }
        }
    }

    image_log_trace("Format selected: %s", formatName[size_t(format)]);

    std::string fileName(reinterpret_cast<char *>(qbsFileName->chr), qbsFileName->len);
    filepath_fix_directory(fileName);

    // Check if fileName has a valid extension and add one if it does not have one
    if (fileName.length() > 4) { // must be at least n.ext
        auto fileExtension = fileName.substr(fileName.length() - 4);
        std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

        image_log_trace("File extension: %s", fileExtension.c_str());

        size_t i;
        for (i = 0; i < _countof(formatName); i++) {
            std::string formatExtension;

            formatExtension = ".";
            formatExtension.append(formatName[i]);

            image_log_trace("Check extension name: %s", formatExtension.c_str());

            if (fileExtension == formatExtension) {
                image_log_trace("Extension (%s) matches with format %i", formatExtension.c_str(), i);
                format = (SaveFormat)i;
                image_log_trace("Format selected by extension: %s", formatName[size_t(format)]);
                break;
            }
        }

        if (i >= _countof(formatName)) { // no matches
            image_log_trace("No matching extension. Adding .%s", formatName[size_t(format)]);

            fileName.append(".");
            fileName.append(formatName[size_t(format)]);
        }
    } else {
        // Simply add the selected format's extension
        image_log_trace("Adding extension: .%s", formatName[size_t(format)]);

        fileName.append(".");
        fileName.append(formatName[size_t(format)]);
    }

    // This will hold our raw RGBA pixel data
    std::vector<uint32_t> pixels;
    int32_t width, height;

    if (img[imageHandle].text) {
        image_log_trace("Rendering text surface to RGBA");

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
            image_log_trace("Converting BGRA surface to RGBA");

            auto p = img[imageHandle].offset32;

            for (size_t i = 0; i < pixels.size(); i++) {
                pixels[i] = image_swap_red_blue(*p);
                ++p;
            }
        } else { // indexed pixels
            image_log_trace("Converting BGRA indexed surface to RGBA");
            auto p = img[imageHandle].offset;

            for (size_t i = 0; i < pixels.size(); i++) {
                pixels[i] = image_swap_red_blue(img[imageHandle].pal[*p]);
                ++p;
            }
        }
    }

    image_log_info("Saving to: %s (%i x %i), %llu pixels, %s", fileName.c_str(), width, height, pixels.size(), formatName[size_t(format)]);

    switch (format) {
    case SaveFormat::PNG: {
        stbi_write_png_compression_level = 100;
        if (!stbi_write_png(fileName.c_str(), width, height, sizeof(uint32_t), pixels.data(), 0)) {
            image_log_error("stbi_write_png() failed");
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
            image_log_error("qoi_write() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::BMP: {
        if (!stbi_write_bmp(fileName.c_str(), width, height, sizeof(uint32_t), pixels.data())) {
            image_log_error("stbi_write_bmp() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::TGA: {
        if (!stbi_write_tga(fileName.c_str(), width, height, sizeof(uint32_t), pixels.data())) {
            image_log_error("stbi_write_tga() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::JPG: {
        if (!stbi_write_jpg(fileName.c_str(), width, height, sizeof(uint32_t), pixels.data(), 100)) {
            image_log_error("stbi_write_jpg() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::HDR: {
        image_log_trace("Converting RGBA to linear float data");

        const auto hdrComponents = 4;

        std::vector<float> hdrPixels;
        hdrPixels.resize(pixels.size() * hdrComponents);

        for (size_t j = 0, i = 0; i < pixels.size(); i++) {
            hdrPixels[j] = pow((pixels[i] & 0xFFu) / 255.0f, 2.2f);
            ++j;
            hdrPixels[j] = pow(((pixels[i] >> 8) & 0xFFu) / 255.0f, 2.2f);
            ++j;
            hdrPixels[j] = pow(((pixels[i] >> 16) & 0xFFu) / 255.0f, 2.2f);
            ++j;
            hdrPixels[j] = (pixels[i] >> 24) / 255.0f;
            ++j;
        }

        if (!stbi_write_hdr(fileName.c_str(), width, height, hdrComponents, hdrPixels.data())) {
            image_log_error("stbi_write_hdr() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::GIF: {
        auto gif = jo_gif_start(fileName.c_str(), short(width), short(height), 0, 255);
        if (gif.fp) {
            jo_gif_frame(&gif, reinterpret_cast<unsigned char *>(pixels.data()), 0, false);
            jo_gif_end(&gif);
        } else {
            image_log_error("jo_gif_start() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    case SaveFormat::ICO: {
        if (!curico_save_file(fileName.c_str(), width, height, pixels.data())) {
            image_log_error("curico_save_file() failed");
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        }
    } break;

    default:
        image_log_error("Save handler not implemented");
        error(QB_ERROR_INTERNAL_ERROR);
    }
}
