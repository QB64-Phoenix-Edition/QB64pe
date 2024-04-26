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

#ifdef QB64_MACOSX
#    include <ApplicationServices/ApplicationServices.h>
#endif

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
#if defined(QB64_MACOSX)

    // We'll use our own clipboard get code on macOS since our requirements are different than what clip supports
    PasteboardRef clipboard = nullptr;
    OSStatus err = PasteboardCreate(kPasteboardClipboard, &clipboard);

    if (err == noErr) {
        PasteboardSynchronize(clipboard);
        ItemCount itemCount = 0;

        err = PasteboardGetItemCount(clipboard, &itemCount);
        if (err == noErr) {
            for (ItemCount itemIndex = 1; itemIndex <= itemCount; itemIndex++) {
                PasteboardItemID itemID = nullptr;
                err = PasteboardGetItemIdentifier(clipboard, itemIndex, &itemID);
                if (err != noErr)
                    continue;

                CFDataRef flavorData = nullptr;
                err = PasteboardCopyItemFlavorData(clipboard, itemID, CFSTR("public.utf8-plain-text"), &flavorData);
                if (err == noErr) {
                    g_InternalClipboard.assign(reinterpret_cast<const char *>(CFDataGetBytePtr(flavorData)), CFDataGetLength(flavorData));

                    CFRelease(flavorData);
                    break;
                }
            }
        }

        CFRelease(clipboard);
    }

#elif defined(QB64_WINDOWS)

    // We'll need custom code for Windows because clip does automatic UTF-8 conversions that leads to some undesired behavior when copying extended ASCII
    if (OpenClipboard(NULL)) {
        if (IsClipboardFormatAvailable(CF_TEXT)) {
            HANDLE hClipboardData = GetClipboardData(CF_TEXT);

            if (hClipboardData) {
                auto pchData = reinterpret_cast<const char *>(GlobalLock(hClipboardData));

                if (pchData) {
                    g_InternalClipboard.assign(pchData, strlen(pchData));

                    GlobalUnlock(hClipboardData);
                }
            }
        }

        CloseClipboard();
    }

#else

    // clip works like we want on Linux
    if (clip::has(clip::text_format()))
        clip::get_text(g_InternalClipboard);

#endif

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
#if defined(QB64_MACOSX)

        // We'll use our own clipboard set code on macOS since our requirements are different than what clip supports
        PasteboardRef clipboard;
        if (PasteboardCreate(kPasteboardClipboard, &clipboard) == noErr) {
            if (PasteboardClear(clipboard) == noErr) {
                CFDataRef data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, qbsText->chr, qbsText->len, kCFAllocatorNull);

                if (data) {
                    PasteboardPutItemFlavor(clipboard, nullptr, kUTTypeUTF8PlainText, data, 0);

                    CFRelease(data);
                }
            }

            CFRelease(clipboard);
        }

#elif defined(QB64_WINDOWS)

        // We'll need custom code for Windows because clip does automatic UTF-8 conversions that leads to some undesired behavior when copying extended ASCII
        if (OpenClipboard(NULL)) {
            if (EmptyClipboard()) {
                HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, qbsText->len + 1);

                if (hClipboardData) {
                    auto pchData = reinterpret_cast<uint8_t *>(GlobalLock(hClipboardData));

                    if (pchData) {
                        memcpy(pchData, qbsText->chr, qbsText->len);
                        pchData[qbsText->len] = '\0'; // null terminate

                        GlobalUnlock(hClipboardData);

                        SetClipboardData(CF_TEXT, hClipboardData);
                    }

                    GlobalFree(hClipboardData);
                }
            }

            CloseClipboard();
        }

#else

        // clip works like we want on Linux
        clip::set_text(g_InternalClipboard);

#endif
    }
}

/// @brief Returns an image handle of an image from the clipboard (if present).
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
                                *dst = image_make_bgra((c & spec.red_mask) >> spec.red_shift >> 8, (c & spec.green_mask) >> spec.green_shift >> 8,
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
                                    *dst = image_make_bgra((c & spec.red_mask) >> spec.red_shift, (c & spec.green_mask) >> spec.green_shift,
                                                           (c & spec.blue_mask) >> spec.blue_shift, (c & spec.alpha_mask) >> spec.alpha_shift);

                                    ++dst;
                                }
                            }
                        } else {
                            for (uint32_t y = 0; y < spec.height; y++) {
                                auto src = reinterpret_cast<const uint32_t *>(clipImg.data() + spec.bytes_per_row * y);
                                for (uint32_t x = 0; x < spec.width; x++, src++) {
                                    auto c = *src;
                                    *dst = image_make_bgra((c & spec.red_mask) >> spec.red_shift, (c & spec.green_mask) >> spec.green_shift,
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
                                *dst = image_make_bgra((c & spec.red_mask) >> spec.red_shift, (c & spec.green_mask) >> spec.green_shift,
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
                                *dst = image_make_bgra(image_scale_5bits_to_8bits((c & spec.red_mask) >> spec.red_shift),
                                                       image_scale_6bits_to_8bits((c & spec.green_mask) >> spec.green_shift),
                                                       image_scale_5bits_to_8bits((c & spec.blue_mask) >> spec.blue_shift), 255u);

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

    // Even though we have color mask and shift support, clip needs the RGBA order :(
    clip::image_spec spec;
    spec.bits_per_pixel = 32;
    spec.red_mask = 0x000000ff;
    spec.green_mask = 0x0000ff00;
    spec.blue_mask = 0x00ff0000;
    spec.alpha_mask = 0xff000000;
    spec.red_shift = 0;
    spec.green_shift = 8;
    spec.blue_shift = 16;
    spec.alpha_shift = 24;

    std::vector<uint32_t> pixels; // this will hold our converted BGRA32 pixel data
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
        pixels.resize(spec.width * spec.height);

        uint8_t fc, bc, *c = srcImg.offset; // set to the first codepoint
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
                        pixels[spec.width * dy + dx] = image_swap_red_blue(*builtinFont ? srcImg.pal[fc] : srcImg.pal[bc]);
                        ++builtinFont;
                    }
                }

                ++c; // move to the next codepoint
            }
        }
    } else {
        spec.width = srcImg.width;
        spec.height = srcImg.height;
        spec.bytes_per_row = spec.width * sizeof(uint32_t);
        pixels.resize(spec.width * spec.height);

        if (srcImg.bits_per_pixel == 32) {
            // BGRA32 pixels
            IMAGE_DEBUG_PRINT("Converting BGRA32 image to RGBA32");

            auto p = srcImg.offset32;

            for (size_t i = 0; i < pixels.size(); i++) {
                pixels[i] = image_swap_red_blue(*p);
                ++p;
            }
        } else {
            // Indexed pixels
            IMAGE_DEBUG_PRINT("Converting BGRA32 indexed image to RGBA32");

            auto p = srcImg.offset;

            for (size_t i = 0; i < pixels.size(); i++) {
                pixels[i] = image_swap_red_blue(srcImg.pal[*p]);
                ++p;
            }
        }
    }

    IMAGE_DEBUG_PRINT("Setting clipboard image");

    // Send the image off to the OS clipboard
    clip::image clipImg(pixels.data(), spec);
    clip::set_image(clipImg);
}
