//----------------------------------------------------------------------------------------------------------------------
// QB64-PE cross-platform clipboard support
// Powered by clip (https://github.com/dacap/clip)
//----------------------------------------------------------------------------------------------------------------------

#include "libqb-common.h"

// We need 'qbs' and 'image' structs stuff from here. Stop using this when image and friends are refactored
#include "../../../libqb.h"

// This is not strictly needed. But we'll leave it here for VSCode to do it's magic
#define CLIP_ENABLE_IMAGE 1
#include "clip/clip.h"
#include "clipboard.h"
#include "error_handle.h"
#define IMAGE_DEBUG 0
#include "image.h"
#include "qbs.h"
#include <vector>

extern const img_struct *img;                 // used by sub__clipboardimage()
extern const img_struct *write_page;          // used by func__clipboardimage()
extern const int32_t *page;                   // used by sub__clipboardimage()
extern const int32_t nextimg;                 // used by sub__clipboardimage()
extern const uint8_t charset8x8[256][8][8];   // used by sub__clipboardimage()
extern const uint8_t charset8x16[256][16][8]; // used by sub__clipboardimage()

// This is used as a fallback internal clipboard should any text clipboard functions below fail
static std::string g_InternalClipboard;

/// @brief Gets text (if present) in the OS clipboard.
/// @return A qbs string.
qbs *func__clipboard() {
    if (clip::has(clip::text_format()))
        clip::get_text(g_InternalClipboard);

    auto qbsText = qbs_new(g_InternalClipboard.length(), 1);
    if (qbsText->len)
        memcpy(qbsText->chr, g_InternalClipboard.data(), qbsText->len);

    return qbsText;
}

/// @brief Sets text to the OS clipboard.
/// @param qbsText A qbs string.
void sub__clipboard(const qbs *qbsText) {
    g_InternalClipboard.assign(reinterpret_cast<const char *>(qbsText->chr), qbsText->len);

    if (qbsText->len) {
        clip::set_text(g_InternalClipboard);
    }
}

static constexpr inline int clipboard_scale_5bits_to_8bits(const int v) { return (v << 3) | (v >> 2); }

static constexpr inline int clipboard_scale_6bits_to_8bits(const int v) { return (v << 2) | (v >> 4); }

/// @brief Retuns an image handle of an image from the clipboard (if present).
/// @return A valid image handle. Returns -1 if clipboard format is not supported or if there is nothing.
int32_t func__clipboardimage() {
    int32_t qb64Img = INVALID_IMAGE_HANDLE; // assume failure

    if (is_error_pending())
        return qb64Img;

    if (clip::has(clip::image_format())) {
        clip::image clipImg;

        IMAGE_DEBUG_PRINT("Clipboard image found");

        if (clip::get_image(clipImg)) {
            auto spec = clipImg.spec();

            IMAGE_DEBUG_PRINT("Image (%lu x %lu) @ %lubpp", spec.width, spec.height, spec.bits_per_pixel);

            if (spec.width && spec.height) {
                auto oldDest = func__dest();

                // We only support 32bpp images. Images in other formats are converted to 32bpp BGRA
                qb64Img = func__newimage(spec.width, spec.height, 32, 1);

                if (qb64Img < INVALID_IMAGE_HANDLE) {
                    sub__dest(qb64Img);
                    auto dst = write_page->offset32;

                    IMAGE_DEBUG_PRINT("Converting and copying image");

                    // Convert and copy the image based on the bpp
                    switch (spec.bits_per_pixel) {
                    case 64:
                        for (uint32_t y = 0; y < spec.height; y++) {
                            auto src = reinterpret_cast<const uint64_t *>(clipImg.data() + spec.bytes_per_row * y);
                            for (uint32_t x = 0; x < spec.width; x++, src++) {
                                auto c = *src;
                                *dst = IMAGE_MAKE_BGRA((c & spec.red_mask) >> spec.red_shift >> 8, (c & spec.green_mask) >> spec.green_shift >> 8,
                                                       (c & spec.blue_mask) >> spec.blue_shift >> 8, (c & spec.alpha_mask) >> spec.alpha_shift >> 8);

                                ++dst;
                            }
                        }
                        break;

                    case 32:
                        // The alpha mask can be zero (which means that the image is just RGB)
                        if (spec.alpha_mask) {
                            for (uint32_t y = 0; y < spec.height; y++) {
                                auto src = reinterpret_cast<const uint32_t *>(clipImg.data() + spec.bytes_per_row * y);
                                for (uint32_t x = 0; x < spec.width; x++, src++) {
                                    auto c = *src;
                                    *dst = IMAGE_MAKE_BGRA((c & spec.red_mask) >> spec.red_shift, (c & spec.green_mask) >> spec.green_shift,
                                                           (c & spec.blue_mask) >> spec.blue_shift, (c & spec.alpha_mask) >> spec.alpha_shift);

                                    ++dst;
                                }
                            }
                        } else {
                            for (uint32_t y = 0; y < spec.height; y++) {
                                auto src = reinterpret_cast<const uint32_t *>(clipImg.data() + spec.bytes_per_row * y);
                                for (uint32_t x = 0; x < spec.width; x++, src++) {
                                    auto c = *src;
                                    *dst = IMAGE_MAKE_BGRA((c & spec.red_mask) >> spec.red_shift, (c & spec.green_mask) >> spec.green_shift,
                                                           (c & spec.blue_mask) >> spec.blue_shift, 255u);

                                    ++dst;
                                }
                            }
                        }
                        break;

                    case 24:
                        for (uint32_t y = 0; y < spec.height; y++) {
                            auto src = reinterpret_cast<const uint8_t *>(clipImg.data() + spec.bytes_per_row * y);
                            for (uint32_t x = 0; x < spec.width; x++, src += 3) {
                                auto c = *reinterpret_cast<const uint32_t *>(src);
                                *dst = IMAGE_MAKE_BGRA((c & spec.red_mask) >> spec.red_shift, (c & spec.green_mask) >> spec.green_shift,
                                                       (c & spec.blue_mask) >> spec.blue_shift, 255u);

                                ++dst;
                            }
                        }
                        break;

                    case 16:
                        for (uint32_t y = 0; y < spec.height; y++) {
                            auto src = reinterpret_cast<const uint16_t *>(clipImg.data() + spec.bytes_per_row * y);
                            for (uint32_t x = 0; x < spec.width; x++, src++) {
                                auto c = *src;
                                *dst = IMAGE_MAKE_BGRA(clipboard_scale_5bits_to_8bits((c & spec.red_mask) >> spec.red_shift),
                                                       clipboard_scale_6bits_to_8bits((c & spec.green_mask) >> spec.green_shift),
                                                       clipboard_scale_5bits_to_8bits((c & spec.blue_mask) >> spec.blue_shift), 255u);

                                ++dst;
                            }
                        }
                    }
                }

                sub__dest(oldDest);
            }
        }
    }

    return qb64Img;
}

/// @brief Set the clipboard image using a QB64 image handle.
/// @param src A valid QB64 image handle.
void sub__clipboardimage(int32_t src) {
    if (is_error_pending())
        return;

    // Validation
    if (src >= 0) {
        validatepage(src);
        src = page[src];
    } else {
        src = -src;
        if (src >= nextimg) {
            error(QB_ERROR_INVALID_HANDLE);
            return;
        }
        if (!img[src].valid) {
            error(QB_ERROR_INVALID_HANDLE);
            return;
        }
    }
    // End of validation

    // We'll set this up like QB64's 32bpp BGRA to avoid conversions at our end
    clip::image_spec spec;
    spec.bits_per_pixel = 32;
    spec.red_mask = 0xff0000;
    spec.green_mask = 0xff00;
    spec.blue_mask = 0xff;
    spec.alpha_mask = 0xff000000;
    spec.red_shift = 16;
    spec.green_shift = 8;
    spec.blue_shift = 0;
    spec.alpha_shift = 24;

    auto srcImg = img[src];

    if (srcImg.text) {
        IMAGE_DEBUG_PRINT("Rendering text surface to BGRA32");

        uint32_t const fontWidth = 8;
        uint32_t fontHeight = 16;
        if (srcImg.font == 8 || srcImg.font == 14)
            fontHeight = srcImg.font;

        spec.width = fontWidth * srcImg.width;
        spec.height = fontHeight * srcImg.height;
        spec.bytes_per_row = spec.width * sizeof(uint32_t);

        std::vector<uint32_t> pixels(spec.width * spec.height); // this will hold our converted BGRA pixel data
        uint8_t fc, bc, *c = srcImg.offset;                     // set to the first codepoint
        uint8_t const *builtinFont = nullptr;

        // Render all text to the raw pixel array
        for (uint32_t y = 0; y < spec.height; y += fontHeight) {
            for (uint32_t x = 0; x < spec.width; x += fontWidth) {
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
                for (uint32_t dy = y, py = 0; py < fontHeight; dy++, py++) {
                    for (uint32_t dx = x, px = 0; px < fontWidth; dx++, px++) {
                        pixels[spec.width * dy + dx] = (*builtinFont ? srcImg.pal[fc] : srcImg.pal[bc]);
                        ++builtinFont;
                    }
                }

                ++c; // move to the next codepoint
            }
        }

        IMAGE_DEBUG_PRINT("Setting clipboard image (rendered)");
        clip::image clipImg(pixels.data(), spec);
        clip::set_image(clipImg);
    } else {
        spec.width = srcImg.width;
        spec.height = srcImg.height;
        spec.bytes_per_row = spec.width * sizeof(uint32_t);

        if (srcImg.bits_per_pixel == 32) {
            // BGRA pixels
            IMAGE_DEBUG_PRINT("Setting clipboard image (raw)");

            clip::image clipImg(srcImg.offset32, spec);
            clip::set_image(clipImg);
        } else {
            // Indexed pixels
            IMAGE_DEBUG_PRINT("Converting BGRA indexed surface to BGRA32");

            std::vector<uint32_t> pixels(spec.width * spec.height); // this will hold our converted BGRA pixel data
            auto p = srcImg.offset;

            for (size_t i = 0; i < pixels.size(); i++) {
                pixels[i] = srcImg.pal[*p];
                ++p;
            }

            IMAGE_DEBUG_PRINT("Setting clipboard image (converted)");
            clip::image clipImg(pixels.data(), spec);
            clip::set_image(clipImg);
        }
    }
}
