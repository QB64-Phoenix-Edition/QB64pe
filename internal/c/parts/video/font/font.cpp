//----------------------------------------------------------------------------------------------------------------------
// QB64-PE Font Library
// Powered by FreeType 2.4.12 (https://github.com/vinniefalco/FreeTypeAmalgam)
//----------------------------------------------------------------------------------------------------------------------

#define FONT_DEBUG 0
#include "font.h"
#include "../../../libqb.h"
#include "error_handle.h"
#include "gui.h"
#include "image.h"
#include "libqb-common.h"
#include "mutex.h"
#include "rounding.h"
#include <cstdio>
#include <ft2build.h>
#include FT_FREETYPE_H
extern "C" {
#include "freetype/md5.h"
}
#include <unordered_map>
#include <vector>

// Note: QB64 expects invalid font handles to be zero
#define IS_VALID_FONT_HANDLE(_h_) ((_h_) > INVALID_FONT_HANDLE && (_h_) < fontManager.fonts.size() && fontManager.fonts[_h_]->isUsed)
#define IS_VALID_QB64_FONT_HANDLE(_h_) ((_h_) <= lastfont && ((fontwidth[_h_] && fontheight[_h_]) || ((_h_) >= 32 && font[_h_])))
#define IS_VALID_UTF_ENCODING(_e_) ((_e_) == 0 || (_e_) == 8 || (_e_) == 16 || (_e_) == 32)

// These are from libqb.cpp
extern const img_struct *write_page;
extern const int32_t *font;
extern const int32_t *fontwidth;
extern const int32_t *fontheight;
extern const int32_t *fontflags;
extern const int32_t lastfont;
extern const uint8_t charset8x8[256][8][8];
extern const uint8_t charset8x16[256][16][8];

void pset_and_clip(int32_t x, int32_t y, uint32_t col);

/// @brief A simple class that manages conversions from various encodings to UTF32
class UTF32 {
  private:
    // See DecodeUTF8() below for more details
    enum UTF8DecoderState { ACCEPT = 0, REJECT = 1 };

    /// @brief See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
    /// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
    /// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
    /// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
    /// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
    /// is furnished to do so, subject to the following conditions:
    /// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
    /// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    /// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    /// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
    /// IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
    /// @param state The current state of the decoder
    /// @param codep The decoded codepoint after state changes to UTF8DecodeState::ACCEPT
    /// @param byte The next UTF-8 byte in the input stream
    /// @return UTF8DecodeState::ACCEPT if enough bytes have been read for a character,
    /// UTF8DecodeState::REJECT if the byte is not allowed to occur at its position,
    /// and some other positive value if more bytes have to be read
    uint32_t DecodeUTF8(uint32_t *state, uint32_t *codep, uint8_t byte) {
        // clang-format off
        static const uint8_t utf8d[] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
            7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
            8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
            0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
            0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
            0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
            1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
            1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
            1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
        };
        // clang-format on

        uint32_t type = utf8d[byte];
        *codep = (*state != UTF8DecoderState::ACCEPT) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);
        *state = utf8d[256 + *state * 16 + type];

        return *state;
    }

  public:
    UTF32 &operator=(const UTF32 &) = delete;
    UTF32 &operator=(UTF32 &&) = delete;

    std::vector<uint32_t> codepoints; // UTF32 codepoint dynamic array

    /// @brief Converts an ASCII array to UTF-32
    /// @param str The ASCII array
    /// @param byte_len The size of the array in bytes
    /// @return The number of codepoints that were converted
    size_t ConvertASCII(const uint8_t *str, size_t byte_len) {
        // Clear the codepoint vector
        codepoints.clear();

        // Convert the ASCII string
        for (size_t i = 0; i < byte_len; i++)
            codepoints.push_back(codepage437_to_unicode16[str[i]]);

        return codepoints.size();
    }

    /// @brief Converts an UTF-8 array to UTF-32. This does not check for BOM
    /// @param str The UTF-8 array
    /// @param byte_len The size of the array in bytes
    /// @return The number of codepoints that were converted
    size_t ConvertUTF8(const uint8_t *str, size_t byte_len) {
        // Clear the codepoint vector
        codepoints.clear();

        uint32_t prevState = UTF8DecoderState::ACCEPT, currentState = UTF8DecoderState::ACCEPT;
        uint32_t cp;

        for (size_t i = 0; i < byte_len; i++, prevState = currentState) {
            switch (DecodeUTF8(&currentState, &cp, str[i])) {
            case UTF8DecoderState::ACCEPT:
                // Good codepoint
                codepoints.push_back(cp);
                break;

            case UTF8DecoderState::REJECT:
                // Codepoint would be U+FFFD (replacement character)
                cp = 0xFFFD;
                currentState = UTF8DecoderState::ACCEPT;
                if (prevState != UTF8DecoderState::ACCEPT)
                    --i;
                codepoints.push_back(cp);
                break;

            default:
                // Need to read continuation bytes
                continue;
            }
        }

        return codepoints.size();
    }

    /// @brief Converts an UTF-16LE array to UTF-32. This does not check for BOM
    /// @param str The UTF-16LE array
    /// @param byte_len The size of the array in bytes
    /// @return The number of codepoints that were converted
    size_t ConvertUTF16(const uint8_t *str, size_t byte_len) {
        // Clear the codepoint vector
        codepoints.clear();

        // We'll assume the worst case scenario and allocate a buffer that is byte_len / 2 codepoints long
        auto len16 = byte_len / sizeof(uint16_t);
        auto str16 = (const uint16_t *)str;
        uint32_t cp;

        for (size_t i = 0; i < len16; i++) {
            auto ch = str16[i];

            // If the character is a surrogate, we need to combine it with the next character to get the actual codepoint
            if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < len16) {
                auto ch2 = str16[i + 1];
                if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
                    cp = ((ch - 0xD800) << 10) + (ch2 - 0xDC00) + 0x10000;
                    ++i; // skip the second surrogate
                } else {
                    cp = 0xFFFD; // invalid surrogate pair
                }
            } else if (ch >= 0xDC00 && ch <= 0xDFFF) {
                cp = 0xFFFD; // invalid surrogate pair
            } else {
                cp = ch;
            }

            codepoints.push_back(cp);
        }

        return codepoints.size();
    }
};

/// @brief This class manages all font handles, bitmaps, hashmaps of glyph bitmaps etc.
struct FontManager {
    FT_Library library;       // FreeType library object
    int32_t lowestFreeHandle; // the lowest free handle that can be allocated
    int32_t reservedHandle;   // this is set to handle 0 so that it is not returned to QB64

    /// @brief Manages a single font
    struct Font {
        bool isUsed;           // is this handle in use?
        uint8_t *fontData;     // raw font data (we always store a copy as long as the font is in use)
        FT_Face face;          // FreeType face object
        FT_Pos monospaceWidth; // the monospace width (if font was loaded as monospace, else zero)
        FT_Pos defaultHeight;  // default (max) pixel height the user wants
        FT_Pos baseline;       // font baseline in pixels
        int32_t options;       // fonts options that were passed by QB64 while loading the font

        /// @brief Manages a single glyph in a font
        struct Glyph {
            // Usually the bitmap size & metrics returned by FT for mono and gray can be the same
            // But it's a bad idea to assume that is the case every time
            struct Bitmap {
                uint8_t *data;       // pointer to the raw pixels
                FT_Vector size;      // bitmap width & height in pixels
                FT_Pos advanceWidth; // glyph advance width in pixels
                FT_Vector bearing;   // glyph left and top side bearing in pixels
            };

            FT_UInt index;  // glyph index
            Bitmap bmpMono; // monochrome bitmap in 8-bit format
            Bitmap bmpGray; // anti-aliased bitmap in 8-bit format
            Bitmap *bitmap; // pointer to the currently selected bitmap (mono / gray)

            // Delete copy and move constructors and assignments
            Glyph(const Glyph &) = delete;
            Glyph &operator=(const Glyph &) = delete;
            Glyph(Glyph &&) = delete;
            Glyph &operator=(Glyph &&) = delete;

            /// @brief Just initializes everything
            Glyph() {
                index = 0;
                bmpMono = {};
                bmpGray = {};
                bitmap = nullptr;
            }

            /// @brief Frees any cached glyph bitmap
            ~Glyph() {
                FONT_DEBUG_PRINT("Freeing bitmaps %p, %p", bmpMono.data, bmpGray.data);

                free(bmpGray.data);
                free(bmpMono.data);
            }

            /// @brief Assuming a glyph was previously loaded and rendered by FreeType, this will prepare an internal bitmap struct
            /// @param bmp A pointer to a bitmap struct to prepare
            /// @param parentFont The parent font object
            /// @return True if successful, false otherwise
            bool PrepareBitmap(Bitmap *bmp, Font *parentFont) {
                FONT_DEBUG_CHECK(bmp && !bmp->data);

                // First get all needed glyph metrics
                bmp->size.x = parentFont->face->glyph->bitmap.width;         // get the width of the bitmap
                bmp->size.y = parentFont->face->glyph->bitmap.rows;          // get the height of the bitmap
                bmp->advanceWidth = parentFont->face->glyph->advance.x >> 6; // get the advance width of the glyph
                bmp->bearing.x = parentFont->face->glyph->bitmap_left;       // get the bitmap left side bearing
                bmp->bearing.y = parentFont->face->glyph->bitmap_top;        // get the bitmap top side bearing

                // Check if the glyph has a valid bitmap
                if (!parentFont->face->glyph->bitmap.buffer || bmp->size.x < 1 || bmp->size.y < 1 ||
                    (parentFont->face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_MONO && parentFont->face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)) {
                    // Ok, this means the font does not have a glyph for the codepoint index
                    // Simply make a blank bitmap and update width and height
                    FONT_DEBUG_PRINT("Entering missing glyph path");

                    bmp->size.x = std::max(bmp->advanceWidth, bmp->size.x);
                    if (bmp->size.x < 1) {
                        FONT_DEBUG_PRINT("Failed to get default width for empty glyph");
                        *bmp = {};
                        return false; // something seriously went wrong
                    }
                    bmp->size.y = parentFont->defaultHeight;

                    FONT_DEBUG_PRINT("Creating empty (%i x %i) bitmap for missing glyph", bmp->size.x, bmp->size.y);

                    // Allocate zeroed memory for monochrome bitmap
                    bmp->data = (uint8_t *)calloc(bmp->size.x, bmp->size.y);
                    if (!bmp->data) {
                        FONT_DEBUG_PRINT("Failed to allocate memory for empty glyph bitmap");
                        *bmp = {};
                        return false; // memory allocation failed
                    }
                } else {
                    // The bitmap rendered successfully
                    FONT_DEBUG_PRINT("(%i x %i) bitmap found", bmp->size.x, bmp->size.y);

                    // So, we have a valid glyph bitmap. We'll use that
                    // Allocate zeroed memory for the bitmap
                    bmp->data = (uint8_t *)calloc(bmp->size.x, bmp->size.y);
                    if (!bmp->data) {
                        FONT_DEBUG_PRINT("Failed to allocate memory for glyph bitmap");
                        *bmp = {};
                        return false; // memory allocation failed
                    }

                    auto src = parentFont->face->glyph->bitmap.buffer;
                    auto dst = bmp->data;

                    // Copy the bitmap based on the pixel mode
                    if (parentFont->face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                        for (FT_Pos y = 0; y < bmp->size.y; y++, src += parentFont->face->glyph->bitmap.pitch, dst += bmp->size.x) {
                            for (FT_Pos x = 0; x < bmp->size.x; x++) {
                                dst[x] = (((src[x >> 3]) >> (7 - (x & 7))) & 1) * 255; // this looks at each bit and then sets the pixel
                            }
                        }
                    } else if (parentFont->face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
                        for (FT_Pos y = 0; y < bmp->size.y; y++, src += parentFont->face->glyph->bitmap.pitch, dst += bmp->size.x) {
                            memcpy(dst, src, bmp->size.x); // simply copy the line
                        }
                    } else {
                        FONT_DEBUG_PRINT("Unknown bitmap pixel mode %i", (int)parentFont->face->glyph->bitmap.pixel_mode); // this should never happen
                        free(bmp->data);
                        *bmp = {};
                        return false;
                    }
                }

                return true;
            }

            /// @brief Caches a glyph bitmap with a given codepoint and this happens only once
            /// @param codepoint A valid UTF-32 codepoint
            /// @param parentFont The parent font object
            /// @return True if successful or if bitmap is already cached
            bool CacheBitmap(uint32_t codepoint, Font *parentFont) {
                if (!bitmap) {
                    // Get the glyph index first and store it
                    // Note that this can return a valid glyph index but the index need not have any glyph bitmap
                    index = FT_Get_Char_Index(parentFont->face, codepoint);
                    if (!index) {
                        FONT_DEBUG_PRINT("Got glyph index zero for codepoint %lu", codepoint);
                    }

                    // Load the mono glyph to query details and render
                    if (FT_Load_Glyph(parentFont->face, index, FT_LOAD_TARGET_MONO)) {
                        FONT_DEBUG_PRINT("Failed to load mono glyph for codepoint %lu (%u)", codepoint, index);
                    }

                    // We'll attempt to render the monochrome font first
                    if (FT_Render_Glyph(parentFont->face->glyph, FT_RENDER_MODE_MONO)) {
                        FONT_DEBUG_PRINT("Failed to render mono glyph for codepoint %lu (%u)", codepoint, index);
                    }

                    if (!PrepareBitmap(&bmpMono, parentFont)) {
                        FONT_DEBUG_PRINT("Failed to prepare mono glyph for codepoint %lu (%u)", codepoint, index);
                        return false;
                    }

                    // Load the gray glyph to query details and render
                    if (FT_Load_Char(parentFont->face, codepoint, FT_LOAD_RENDER)) {
                        FONT_DEBUG_PRINT("Failed to load gray glyph for codepoint %lu (%u)", codepoint, index);
                    }

                    // Render the gray bitmap
                    if (FT_Render_Glyph(parentFont->face->glyph, FT_RENDER_MODE_NORMAL)) {
                        FONT_DEBUG_PRINT("Failed to render gray glyph for codepoint %lu (%u)", codepoint, index);
                    }

                    if (!PrepareBitmap(&bmpGray, parentFont)) {
                        FONT_DEBUG_PRINT("Failed to prepare gray glyph for codepoint %lu (%u)", codepoint, index);
                        free(bmpMono.data); // free mono bitmap
                        bmpMono = {};
                        return false;
                    }

                    FONT_DEBUG_PRINT("Bitmap cached (%p, %p) for codepoint %u, index %i", bmpMono.data, bmpGray.data, codepoint, index);
                    FONT_DEBUG_PRINT("Mono: W = %i, H = %i, AW = %i, BX = %i, BY = %i", bmpMono.size.x, bmpMono.size.y, bmpMono.advanceWidth, bmpMono.bearing.x,
                                     bmpMono.bearing.y);
                    FONT_DEBUG_PRINT("Gray: W = %i, H = %i, AW = %i, BX = %i, BY = %i", bmpGray.size.x, bmpGray.size.y, bmpGray.advanceWidth, bmpGray.bearing.x,
                                     bmpGray.bearing.y);

                    bitmap = &bmpGray; // set bitmap to gray bitmap by default
                }

                return bitmap != nullptr;
            }

            /// @brief Renders the glyph bitmap to the target bitmap using "alpha blending"
            /// @param dst The target bitmap to render to
            /// @param dstW The width of the target bitmap
            /// @param dstH The height of the target bitmap
            /// @param dstL The x position on the target bitmap where the rendering should start
            /// @param dstT The y position on the target bitmap where the rendering should start
            /// @return True if successful
            void RenderBitmap(uint8_t *dst, FT_Pos dstW, FT_Pos dstH, FT_Pos dstL, FT_Pos dstT) {
                FONT_DEBUG_CHECK(bitmap && dst);

                auto dstR = dstL + bitmap->size.x; // right of dst + 1 where we will end
                auto dstB = dstT + bitmap->size.y; // bottom of dst + 1 where we will end
                auto alphaSrc = bitmap->data;
                for (FT_Pos dy = dstT; dy < dstB; dy++) {
                    for (FT_Pos dx = dstL; dx < dstR; dx++) {
                        if (dx >= 0 && dx < dstW && dy >= 0 && dy < dstH) { // if we are not clipped
                            auto dstP = (dst + dstW * dy + dx);             // dst pointer
                            if (*alphaSrc > *dstP)                          // blend both alpha and save to dst pointer
                                *dstP = *alphaSrc;
                        }
                        ++alphaSrc;
                    }
                }
            }
        };

        std::unordered_map<uint32_t, Glyph *> glyphs; // holds pointers to cached glyph data for codepoints

        // Delete copy and move constructors and assignments
        Font(const Font &) = delete;
        Font &operator=(const Font &) = delete;
        Font(Font &&) = delete;
        Font &operator=(Font &&) = delete;

        /// @brief Initializes all members
        Font() {
            isUsed = false;
            fontData = nullptr;
            face = nullptr;
            monospaceWidth = defaultHeight = baseline = options = 0;
        }

        /// @brief Frees any cached glyph
        ~Font() {
            // Free the FreeType face object
            if (FT_Done_Face(face)) {
                FONT_DEBUG_PRINT("Failed to free FreeType face object (%p)", face);
            } else {
                FONT_DEBUG_PRINT("FreeType face object freed");
            }

            // Free the buffered font data
            free(fontData);
            FONT_DEBUG_PRINT("Raw font data buffer freed");

            FONT_DEBUG_PRINT("Freeing cached glyphs");
            // Free any allocated glyph manager
            // This should also call the glyphs destructor freeing the bitmap data
            for (auto &it : glyphs)
                delete it.second;
        }

        /// @brief Creates a glyph belonging to a codepoint, caches its bitmap + info and adds it to the hash map
        /// @param codepoint A valid UTF-32 codepoint
        /// @param isMono True for mono bitmap and false for gray
        /// @return The glyph pointer if successful or if the glyph is already in the map, nullptr otherwise
        Glyph *GetGlyph(uint32_t codepoint, bool isMono) {
            if (glyphs.count(codepoint) == 0) {
                // The glyph is not cached yet
                auto newGlyph = new Glyph;

                if (!newGlyph) {
                    FONT_DEBUG_PRINT("Failed to allocate memory");
                    return nullptr; // failed to allocate memory
                }

                // Cache the glyph info and bitmap
                if (!newGlyph->CacheBitmap(codepoint, this)) {
                    delete newGlyph;
                    FONT_DEBUG_PRINT("Failed to cache glyph data");
                    return nullptr; // failed to cache bitmap
                }

                glyphs[codepoint] = newGlyph;                                        // save the Glyph pointer to the map using the codepoint as key
                newGlyph->bitmap = isMono ? &newGlyph->bmpMono : &newGlyph->bmpGray; // select the correct bitmap

                FONT_DEBUG_PRINT("Glyph data for codepoint %u successfully cached", codepoint);

                return newGlyph; // return the glyph pointer
            }

            auto glyph = glyphs[codepoint];                             // we already have the glyph cached, so simply return the pointer
            glyph->bitmap = isMono ? &glyph->bmpMono : &glyph->bmpGray; // select the correct bitmap

            return glyph;
        }

        /// @brief This returns the length of a UTF32 codepoint array in pixels
        /// @param codepoint The codepoint array (string)
        /// @param codepoints The number of codepoints in the array
        /// @return The length of the string in pixels
        FT_Pos GetStringPixelWidth(const uint32_t *codepoint, size_t codepoints) {
            if (monospaceWidth) // return monospace width simply by multiplying the fixed width by the codepoints
                return monospaceWidth * codepoints;

            FT_Pos width = 0;                       // the calculated width in pixel
            auto hasKerning = FT_HAS_KERNING(face); // set to true if font has kerning info
            Glyph *glyph = nullptr;
            Glyph *previousGlyph = nullptr;
            auto isMonochrome = (write_page->bytes_per_pixel == 1) || ((write_page->bytes_per_pixel == 4) && (write_page->alpha_disabled)) ||
                                (options & FONT_LOAD_DONTBLEND); // monochrome or AA?

            for (size_t i = 0; i < codepoints; i++) {
                auto cp = codepoint[i];

                glyph = GetGlyph(cp, isMonochrome);
                if (glyph) {
                    // Add kerning advance width if kerning table is available
                    if (hasKerning && previousGlyph && glyph) {
                        FT_Vector delta;
                        FT_Get_Kerning(face, previousGlyph->index, glyph->index, FT_KERNING_DEFAULT, &delta);
                        width += delta.x >> 6;
                    }

                    width += glyph->bitmap->advanceWidth; // add advance width
                    previousGlyph = glyph;                // save the current glyph pointer for use later
                }
            }

            // Adjust for the last glyph
            if (glyph) {
                auto adjust = glyph->bitmap->advanceWidth;
                if (adjust < glyph->bitmap->size.x)
                    adjust = glyph->bitmap->size.x;
                if (glyph->bitmap->bearing.x > 0 && (glyph->bitmap->size.x + glyph->bitmap->bearing.x) > adjust)
                    adjust = glyph->bitmap->size.x + glyph->bitmap->bearing.x;
                if (glyph->bitmap->bearing.x < 0)
                    adjust += -glyph->bitmap->bearing.x;
                width = width - glyph->bitmap->advanceWidth + adjust;
            }

            return width;
        }
    };

    std::vector<Font *> fonts; // vector that holds all font objects
    libqb_mutex *m;            // we'll use a mutex to give exclusive access to resources used by multiple threads

    FontManager(const FontManager &) = delete;
    FontManager(FontManager &&) = delete;
    FontManager &operator=(const FontManager &) = delete;
    FontManager &operator=(FontManager &&) = delete;

    /// @brief Initializes important stuff and reserves font handle 0
    FontManager() {
        if (FT_Init_FreeType(&library)) {
            gui_alert("Failed to initialize FreeType!");
            exit(5633);
        }

        FONT_DEBUG_PRINT("FreeType library v%i.%i.%i initialized", FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH);

        m = libqb_mutex_new();

        lowestFreeHandle = 0;
        reservedHandle = -1; // we cannot set 0 here since 0 is a valid internal handle

        // Reserve handle 0 so that nothing else can use it
        // We are doing this because QB64 treats handle 0 as invalid
        reservedHandle = CreateHandle();
        FONT_DEBUG_CHECK(reservedHandle == 0); // the first handle must return 0
    }

    /// @brief Frees any used resources
    ~FontManager() {
        // Free all font handles here
        for (size_t handle = 0; handle < fonts.size(); handle++) {
            ReleaseHandle(handle); // release the handle first
            delete fonts[handle];  // now free the object created by CreateHandle()
        }

        // Now that all fonts are closed and font objects are freed, clear the vector
        fonts.clear();

        libqb_mutex_free(m);

        if (FT_Done_FreeType(library)) {
            gui_alert("Failed to finalize FreeType!");
            exit(5633);
        }

        FONT_DEBUG_PRINT("FreeType library finalized");
    }

    /// @brief Creates are recycles a font handle
    /// @return An unused font handle
    int32_t CreateHandle() {
        size_t h, vectorSize = fonts.size(); // save the vector size

        // Scan the vector starting from lowestFreeHandle
        // This will help us quickly allocate a free handle
        for (h = lowestFreeHandle; h < vectorSize; h++) {
            if (!fonts[h]->isUsed) {
                FONT_DEBUG_PRINT("Recent font handle %i recycled", h);
                break;
            }
        }

        if (h >= vectorSize) {
            // Scan through the entire vector and return a slot that is not being used
            // Ideally this should execute in extremely few (if at all) scenarios
            // Also, this loop should not execute if size is 0
            for (h = 0; h < vectorSize; h++) {
                if (!fonts[h]->isUsed) {
                    FONT_DEBUG_PRINT("Font handle %i recycled", h);
                    break;
                }
            }
        }

        if (h >= vectorSize) {
            // If we have reached here then either the vector is empty or there are no empty slots
            // Simply create a new handle at the back of the vector
            auto newHandle = new Font; // allocate and initialize

            if (!newHandle)
                return -1; // we cannot return 0 here since 0 is a valid internal handle

            fonts.push_back(newHandle);
            size_t newVectorSize = fonts.size();

            // If newVectorSize == vectorSize then push_back() failed
            if (newVectorSize <= vectorSize) {
                delete newHandle;
                return -1; // we cannot return 0 here since 0 is a valid internal handle
            }

            h = newVectorSize - 1; // the handle is simply newVectorSize - 1

            FONT_DEBUG_PRINT("Font handle %i created", h);
        }

        FONT_DEBUG_CHECK(fonts[h]->isUsed == false);

        fonts[h]->fontData = nullptr;
        fonts[h]->face = nullptr;
        fonts[h]->monospaceWidth = 0;
        fonts[h]->defaultHeight = 0;
        fonts[h]->baseline = 0;
        fonts[h]->options = 0;
        fonts[h]->isUsed = true;

        lowestFreeHandle = h + 1; // set lowestFreeHandle to allocated handle + 1

        FONT_DEBUG_PRINT("Font handle %i returned", h);

        return (int32_t)h;
    }

    /// @brief This will mark a handle as free so that it's put up for recycling
    /// @param handle A font handle
    void ReleaseHandle(int32_t handle) {
        if (handle >= 0 && handle < fonts.size() && fonts[handle]->isUsed) {
            // Free the FreeType face object
            if (FT_Done_Face(fonts[handle]->face)) {
                FONT_DEBUG_PRINT("Failed to free FreeType face object (%p)", fonts[handle]->face);
            } else {
                FONT_DEBUG_PRINT("FreeType face object freed");
            }
            fonts[handle]->face = nullptr;

            // Free the buffered font data
            free(fonts[handle]->fontData);
            fonts[handle]->fontData = nullptr;
            FONT_DEBUG_PRINT("Raw font data buffer freed");

            FONT_DEBUG_PRINT("Freeing cached glyphs");
            // Free cached glyph data
            // This should also call the glyphs destructor freeing the bitmap data
            for (auto &it : fonts[handle]->glyphs)
                delete it.second;

            // Reset the hash map
            fonts[handle]->glyphs.clear();
            FONT_DEBUG_PRINT("Hash map cleared");

            // Now simply set the 'isUsed' member to false so that the handle can be recycled
            fonts[handle]->isUsed = false;

            // Save the free handle to lowestFreeHandle if it is lower than lowestFreeHandle
            if (handle < lowestFreeHandle)
                lowestFreeHandle = handle;

            FONT_DEBUG_PRINT("Font handle %i marked as free", handle);
        }
    }
};

/// @brief Global font manager object
static FontManager fontManager;
/// @brief Global utf32 object
static UTF32 utf32;

/// @brief Loads a whole font file from disk to memory.
/// This will search for the file in known places if it is not found in the current directory
/// @param file_path_name The font file name. This can be a relative path
/// @param out_bytes The size of the data that was loaded. This cannot be NULL
/// @return A pointer to a buffer with the data. NULL on failure. The caller is responsible for freeing this memory
uint8_t *FontLoadFileToMemory(const char *file_path_name, int32_t *out_bytes) {
    // This is simply a list of known locations to look for a font
    static const char *const FONT_PATHS[][2] = {
#ifdef QB64_WINDOWS
        {"%s/Microsoft/Windows/Fonts/%s", "LOCALAPPDATA"}, {"%s/Fonts/%s", "SystemRoot"}
#elif defined(QB64_MACOSX)
        {"%s/Library/Fonts/%s", "HOME"}, {"%s/Library/Fonts/%s", nullptr}, {"%s/System/Library/Fonts/%s", nullptr}
#elif defined(QB64_LINUX)
        {"%s/.fonts/%s", "HOME"},
        {"%s/.local/share/fonts/%s", "HOME"},
        {"%s/usr/local/share/fonts/%s", nullptr},
        {"%s/usr/share/fonts/%s", nullptr},
        {"%s/usr/share/fonts/opentype/%s", nullptr},
        {"%s/usr/share/fonts/truetype/%s", nullptr}
#endif
    };

    // Attempt to open the file with the current file pathname
    auto fontFile = fopen(file_path_name, "rb");
    if (!fontFile) {
        FONT_DEBUG_PRINT("Failed to open font file: %s", file_path_name);
        FONT_DEBUG_PRINT("Attempting to load font file using known paths");

        static const auto PATH_BUFFER_SIZE = 4096;
        auto pathName = (char *)malloc(PATH_BUFFER_SIZE);
        if (!pathName) {
            FONT_DEBUG_PRINT("Failed to allocate working buffer");
            return nullptr;
        }
        FONT_DEBUG_PRINT("Allocate working buffer");

        // Go over the known locations and see what works
        for (auto i = 0; i < (sizeof(FONT_PATHS) / sizeof(uintptr_t) / 2); i++) {
            memset(pathName, 0, PATH_BUFFER_SIZE);

            if (FONT_PATHS[i][1] && getenv(FONT_PATHS[i][1]))
                std::snprintf(pathName, PATH_BUFFER_SIZE, FONT_PATHS[i][0], getenv(FONT_PATHS[i][1]), file_path_name);
            else
                std::snprintf(pathName, PATH_BUFFER_SIZE, FONT_PATHS[i][0], "", file_path_name);

            FONT_DEBUG_PRINT("Attempting to load %s", pathName);

            fontFile = fopen(pathName, "rb");
            if (fontFile)
                break; // exit the loop if something worked
        }

        free(pathName);
        FONT_DEBUG_PRINT("Working buffer freed");

        if (!fontFile) {
            FONT_DEBUG_PRINT("No know locations worked");
            return nullptr; // return NULL if all attempts failed
        }
    }

    if (fseek(fontFile, 0, SEEK_END) != 0) {
        FONT_DEBUG_PRINT("Failed to seek end of font file: %s", file_path_name);
        fclose(fontFile);
        return nullptr;
    }

    *out_bytes = ftell(fontFile);
    if (*out_bytes < 0) {
        FONT_DEBUG_PRINT("Failed to determine size of font file: %s", file_path_name);
        fclose(fontFile);
        return nullptr;
    }

    if (fseek(fontFile, 0, SEEK_SET) != 0) {
        FONT_DEBUG_PRINT("Failed to seek beginning of font file: %s", file_path_name);
        fclose(fontFile);
        return nullptr;
    }

    auto buffer = (uint8_t *)malloc(*out_bytes);
    if (!buffer) {
        FONT_DEBUG_PRINT("Failed to allocate memory for font file: %s", file_path_name);
        fclose(fontFile);
        return nullptr;
    }

    if (fread(buffer, *out_bytes, 1, fontFile) != 1) {
        FONT_DEBUG_PRINT("Failed to read font file: %s", file_path_name);
        fclose(fontFile);
        free(buffer);
        return nullptr;
    }

    fclose(fontFile);

    FONT_DEBUG_PRINT("Successfully loaded font file: %s", file_path_name);
    return buffer;
}

/// @brief Loads a FreeType font from memory. The font data is locally copied and is kept alive while in use
/// @param content_original The original font data in memory that is copied
/// @param content_bytes The length of the data in bytes
/// @param default_pixel_height The maximum rendering height of the font
/// @param which_font The font index in a font collection (< 0 means default)
/// @param options [IN/OUT] 16=monospace (all old flags are ignored like it always was since forever)
/// @return A valid font handle (> 0) or 0 on failure
int32_t FontLoad(const uint8_t *content_original, int32_t content_bytes, int32_t default_pixel_height, int32_t which_font, int32_t &options) {
    libqb_mutex_guard lock(fontManager.m);

    // Allocate a font handle
    auto h = fontManager.CreateHandle();
    if (h <= INVALID_FONT_HANDLE)
        return INVALID_FONT_HANDLE;

    // Allocate memory to duplicate content
    // Note: You must not deallocate the memory before calling FT_Done_Face
    fontManager.fonts[h]->fontData = (uint8_t *)malloc(content_bytes);
    // Return invalid handle if memory allocation failed
    if (!fontManager.fonts[h]->fontData) {
        fontManager.ReleaseHandle(h);
        FONT_DEBUG_PRINT("Failed to allocate memory");
        return INVALID_FONT_HANDLE;
    }

    memcpy(fontManager.fonts[h]->fontData, content_original, content_bytes); // duplicate content

    // Adjust font index
    if (which_font < 1)
        which_font = 0;

    // Attempt to initialize the font for use
    if (FT_New_Memory_Face(fontManager.library, fontManager.fonts[h]->fontData, content_bytes, which_font, &fontManager.fonts[h]->face)) {
        fontManager.ReleaseHandle(h); // this will also free the memory allocated above
        FONT_DEBUG_PRINT("FT_New_Memory_Face() failed");
        return INVALID_FONT_HANDLE;
    }

    // Set the font pixel height
    if (FT_Set_Pixel_Sizes(fontManager.fonts[h]->face, 0, default_pixel_height)) {
        fontManager.ReleaseHandle(h); // this will also free the memory allocated above
        FONT_DEBUG_PRINT("FT_Set_Pixel_Sizes() failed");
        return INVALID_FONT_HANDLE;
    }

    fontManager.fonts[h]->defaultHeight = default_pixel_height; // save default pixel height

    // Calculate the baseline using font metrics only if it is scalable
    if (FT_IS_SCALABLE(fontManager.fonts[h]->face))
        fontManager.fonts[h]->baseline = FT_MulDiv(FT_MulFix(fontManager.fonts[h]->face->ascender, fontManager.fonts[h]->face->size->metrics.y_scale),
                                                   default_pixel_height, fontManager.fonts[h]->face->size->metrics.height);

    FONT_DEBUG_PRINT("Font baseline = %d", fontManager.fonts[h]->baseline);

    FONT_DEBUG_PRINT("AUTOMONO requested: %s", (options & FONT_LOAD_AUTOMONO) ? "yes" : "no");

    // Check if automatic fixed width font detection was requested
    if ((options & FONT_LOAD_AUTOMONO) && FT_IS_FIXED_WIDTH(fontManager.fonts[h]->face)) {
        FONT_DEBUG_PRINT("Fixed-width font detected. Setting MONOSPACE flag");

        // Force set monospace flag and pass it upstream if the font is fixed width
        options |= FONT_LOAD_MONOSPACE;
    }

    if (options & FONT_LOAD_MONOSPACE) {
        const FT_ULong testCP = 'W'; // since W is usually the widest

        // Load using monochrome rendering
        if (FT_Load_Char(fontManager.fonts[h]->face, testCP, FT_LOAD_RENDER | FT_LOAD_MONOCHROME | FT_LOAD_TARGET_MONO)) {
            FONT_DEBUG_PRINT("FT_Load_Char() (monochrome) failed");
            // Retry using gray-level rendering
            if (FT_Load_Char(fontManager.fonts[h]->face, testCP, FT_LOAD_RENDER)) {
                FONT_DEBUG_PRINT("FT_Load_Char() (gray) failed");
            }
        }

        if (fontManager.fonts[h]->face->glyph) {
            fontManager.fonts[h]->monospaceWidth =
                std::max<FT_Pos>(fontManager.fonts[h]->face->glyph->advance.x >> 6, fontManager.fonts[h]->face->glyph->bitmap.width); // save the max width

            FONT_DEBUG_PRINT("Monospace font (width = %li) requested", fontManager.fonts[h]->monospaceWidth);

            // Set the baseline to bitmap_top if the font is not scalable
            if (!FT_IS_SCALABLE(fontManager.fonts[h]->face))
                fontManager.fonts[h]->baseline = fontManager.fonts[h]->face->glyph->bitmap_top; // for bitmap fonts bitmap_top is the same for all glyph bitmaps
        }

        // Clear the monospace flag is we failed to get the monospace width
        if (!fontManager.fonts[h]->monospaceWidth)
            options &= ~FONT_LOAD_MONOSPACE;
    }

    fontManager.fonts[h]->options = options; // save the options for use later

    FONT_DEBUG_PRINT("Font (height = %i, index = %i) successfully initialized", default_pixel_height, which_font);
    return h;
}

/// @brief Frees the font and any locally cached data
/// @param fh A valid font handle
void FontFree(int32_t fh) {
    libqb_mutex_guard lock(fontManager.m);

    if (IS_VALID_FONT_HANDLE(fh))
        fontManager.ReleaseHandle(fh);
}

/// @brief Returns the font width
/// @param fh A valid font handle
/// @return The width of the font if the font is monospaced or zero otherwise
int32_t FontWidth(int32_t fh) {
    libqb_mutex_guard lock(fontManager.m);

    FONT_DEBUG_CHECK(IS_VALID_FONT_HANDLE(fh));

    return fontManager.fonts[fh]->monospaceWidth;
}

/// @brief Returns the length of an UTF32 codepoint string in pixels
/// @param fh A valid font
/// @param codepoint The UTF32 codepoint array
/// @param codepoints The number of codepoints
/// @return Length in pixels
int32_t FontPrintWidthUTF32(int32_t fh, const uint32_t *codepoint, int32_t codepoints) {
    libqb_mutex_guard lock(fontManager.m);

    if (codepoints > 0) {
        FONT_DEBUG_CHECK(IS_VALID_FONT_HANDLE(fh));

        FONT_DEBUG_PRINT("codepoint = %p, codepoints = %i", codepoint, codepoints);

        // Get the actual width in pixels
        return fontManager.fonts[fh]->GetStringPixelWidth(codepoint, codepoints);
    }

    return 0;
}

/// @brief Returns the length of an ASCII codepoint string in pixels
/// @param fh A valid font
/// @param codepoint The ASCII codepoint array
/// @param codepoints The number of codepoints
/// @return Length in pixels
int32_t FontPrintWidthASCII(int32_t fh, const uint8_t *codepoint, int32_t codepoints) {
    if (codepoints > 0) {
        FONT_DEBUG_CHECK(IS_VALID_FONT_HANDLE(fh));

        // Attempt to convert the string to UTF32 and get the actual width in pixels
        auto count = utf32.ConvertASCII(codepoint, codepoints);
        return FontPrintWidthUTF32(fh, utf32.codepoints.data(), count);
    }

    return 0;
}

/// @brief Master rendering routine (to be called by all other functions). None of the pointer args can be NULL
/// @param fh A valid font handle
/// @param codepoint A pointer to an array of UTF-32 codepoints that needs to be rendered
/// @param codepoints The number of codepoints in the array
/// @param options 1 = monochrome where black is 0 & white is 255 with nothing in between
/// @param out_data A pointer to a pointer to the output pixel data (alpha values)
/// @param out_x A pointer to the output width of the rendered text in pixels
/// @param out_y A pointer to the output height of the rendered text in pixels
/// @return success = 1, failure = 0
bool FontRenderTextUTF32(int32_t fh, const uint32_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y) {
    libqb_mutex_guard lock(fontManager.m);

    FONT_DEBUG_CHECK(IS_VALID_FONT_HANDLE(fh));

    auto fnt = fontManager.fonts[fh];

    // Safety
    *out_data = nullptr;
    *out_x = 0;
    *out_y = fnt->defaultHeight;

    if (codepoints <= 0)
        return codepoints == 0; // true if zero, false if -ve

    auto isMonochrome = bool(options & FONT_RENDER_MONOCHROME); // do we need to do monochrome rendering?
    FT_Vector strPixSize = {
        fnt->GetStringPixelWidth(codepoint, codepoints), // get the total buffer width
        fnt->defaultHeight                               // height is always set by the QB64
    };
    auto outBuf = (uint8_t *)calloc(strPixSize.x, strPixSize.y);
    if (!outBuf)
        return false;

    FONT_DEBUG_PRINT("Allocated (%lu x %lu) buffer", strPixSize.x, strPixSize.y);

    FT_Pos penX = 0;

    if (fnt->monospaceWidth) {
        for (size_t i = 0; i < codepoints; i++) {
            auto cp = codepoint[i];

            auto glyph = fnt->GetGlyph(cp, isMonochrome);
            if (glyph) {
                glyph->RenderBitmap(outBuf, strPixSize.x, strPixSize.y,
                                    penX + glyph->bitmap->bearing.x + fnt->monospaceWidth / 2 - glyph->bitmap->advanceWidth / 2,
                                    fnt->baseline - glyph->bitmap->bearing.y);
                penX += fnt->monospaceWidth;
            }
        }
    } else {
        auto hasKerning = FT_HAS_KERNING(fnt->face); // set to true if font has kerning info
        FontManager::Font::Glyph *glyph = nullptr;
        FontManager::Font::Glyph *previousGlyph = nullptr;

        for (size_t i = 0; i < codepoints; i++) {
            auto cp = codepoint[i];

            glyph = fnt->GetGlyph(cp, isMonochrome);
            if (glyph) {
                // Add kerning advance width if kerning table is available
                if (hasKerning && previousGlyph && glyph) {
                    FT_Vector delta;
                    FT_Get_Kerning(fnt->face, previousGlyph->index, glyph->index, FT_KERNING_DEFAULT, &delta);
                    penX += delta.x >> 6;
                }

                glyph->RenderBitmap(outBuf, strPixSize.x, strPixSize.y, penX + glyph->bitmap->bearing.x, fnt->baseline - glyph->bitmap->bearing.y);
                penX += glyph->bitmap->advanceWidth; // add advance width
                previousGlyph = glyph;               // save the current glyph pointer for use later
            }
        }
    }

    FONT_DEBUG_PRINT("Buffer width = %li, render width = %li", strPixSize.x, penX);

    *out_data = outBuf;
    *out_x = strPixSize.x;
    *out_y = strPixSize.y;

    return true;
}

/// @brief This will call FontRenderTextUTF32() after converting the ASCII codepoint array to UTF-32. None of the pointer args can be NULL
/// @param fh A valid font handle
/// @param codepoint A pointer to an array of ASCII codepoints that needs to be rendered
/// @param codepoints The number of codepoints in the array
/// @param options 1 = monochrome where black is 0 & white is 255 with nothing in between
/// @param out_data A pointer to a pointer to the output pixel data (alpha values)
/// @param out_x A pointer to the output width of the rendered text in pixels
/// @param out_y A pointer to the output height of the rendered text in pixels
/// @return success = 1, failure = 0
bool FontRenderTextASCII(int32_t fh, const uint8_t *codepoint, int32_t codepoints, int32_t options, uint8_t **out_data, int32_t *out_x, int32_t *out_y) {
    if (codepoints > 0) {
        FONT_DEBUG_CHECK(IS_VALID_FONT_HANDLE(fh));

        // Attempt to convert the string to UTF32 and forward to FontRenderTextUTF32()
        auto count = utf32.ConvertASCII(codepoint, codepoints);
        return FontRenderTextUTF32(fh, utf32.codepoints.data(), count, options, out_data, out_x, out_y);
    }

    return false;
}

/// @brief Expose freetype's MD5 procedure for public use
/// @param text The message to build the MD5 hash of
/// @return The generated MD5 hash as hexadecimal string
qbs *func__md5(qbs *text) {
    MD5_CTX ctx;
    unsigned char md5[16];
    qbs *res;
    int i;

    MD5_Init(&ctx);
    if (text->len)
        MD5_Update(&ctx, text->chr, text->len);
    MD5_Final(md5, &ctx);

    res = qbs_new(32, 1);
    for (i = 0; i < 16; i++)
        sprintf((char *)&res->chr[i * 2], "%02X", md5[i]);

    return res;
}

/// @brief Return the true font height in pixel
/// @param qb64_fh A QB64 font handle (this can be a builtin font as well)
/// @param passed Optional arguments flag
/// @return The height in pixels
int32_t func__UFontHeight(int32_t qb64_fh, int32_t passed) {
    libqb_mutex_guard lock(fontManager.m);

    if (is_error_pending())
        return 0;

    if (passed) {
        // Check if a valid font handle was passed
        if (!IS_VALID_QB64_FONT_HANDLE(qb64_fh)) {
            error(QB_ERROR_INVALID_HANDLE);
            return 0;
        }
    } else {
        qb64_fh = write_page->font; // else get the current write page font handle
    }

    if (qb64_fh < 32)
        return fontheight[qb64_fh];

    FONT_DEBUG_CHECK(IS_VALID_FONT_HANDLE(font[qb64_fh]));

    // Else we will return the FreeType font height
    auto fnt = fontManager.fonts[font[qb64_fh]];
    auto face = fnt->face;

    if (FT_IS_SCALABLE(face))
        return (((FT_Pos)face->ascender - (FT_Pos)face->descender) * fnt->defaultHeight) / (FT_Pos)face->units_per_EM;

    return fnt->defaultHeight;
}

/// @brief Returns the text width in pixels
/// @param text The text to calculate the width for
/// @param utf_encoding The UTF encoding of the text (0 = ASCII, 8 = UTF-8, 16 - UTF-16, 32 = UTF-32)
/// @param qb64_fh A QB64 font handle (this can be a builtin font as well)
/// @param passed Optional arguments flag
/// @return The width in pixels
int32_t func__UPrintWidth(const qbs *text, int32_t utf_encoding, int32_t qb64_fh, int32_t passed) {
    libqb_mutex_guard lock(fontManager.m);

    if (is_error_pending() || !text->len)
        return 0;

    // Check UTF argument
    if (passed & 1) {
        if (!IS_VALID_UTF_ENCODING(utf_encoding)) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return 0;
        }
    } else {
        utf_encoding = 0;
    }

    // Check if a valid font handle was passed
    if (passed & 2) {
        if (!IS_VALID_QB64_FONT_HANDLE(qb64_fh)) {
            error(QB_ERROR_INVALID_HANDLE);
            return 0;
        }
    } else {
        qb64_fh = write_page->font; // else get the current write page font handle
    }

    // Convert the string to UTF-32 if needed
    uint32_t const *str32 = nullptr;
    size_t codepoints = 0;

    switch (utf_encoding) {
    case 32: // UTF-32: no conversion needed
        str32 = (uint32_t *)text->chr;
        codepoints = text->len / sizeof(uint32_t);
        break;

    case 16: // UTF-16: conversion required
        codepoints = utf32.ConvertUTF16(text->chr, text->len);
        if (codepoints)
            str32 = utf32.codepoints.data();
        break;

    case 8: // UTF-8: conversion required
        codepoints = utf32.ConvertUTF8(text->chr, text->len);
        if (codepoints)
            str32 = utf32.codepoints.data();
        break;

    default: // ASCII: conversion required
        codepoints = utf32.ConvertASCII(text->chr, text->len);
        if (codepoints)
            str32 = utf32.codepoints.data();
    }

    if (qb64_fh < 32)
        return (int32_t)(codepoints * fontwidth[qb64_fh]);

    FONT_DEBUG_CHECK(IS_VALID_FONT_HANDLE(font[qb64_fh]));

    return (int32_t)fontManager.fonts[font[qb64_fh]]->GetStringPixelWidth(str32, codepoints);
}

/// @brief Returns the vertical line spacing in pixels (font height + extra pixels if any)
/// @param qb64_fh A QB64 font handle (this can be a builtin font as well)
/// @param passed Optional arguments flag
/// @return The vertical spacing in pixels
int32_t func__ULineSpacing(int32_t qb64_fh, int32_t passed) {
    libqb_mutex_guard lock(fontManager.m);

    if (is_error_pending())
        return 0;

    if (passed) {
        // Check if a valid font handle was passed
        if (!IS_VALID_QB64_FONT_HANDLE(qb64_fh)) {
            error(QB_ERROR_INVALID_HANDLE);
            return 0;
        }
    } else {
        qb64_fh = write_page->font; // else get the current write page font handle
    }

    if (qb64_fh < 32)
        return fontheight[qb64_fh];

    FONT_DEBUG_CHECK(IS_VALID_FONT_HANDLE(font[qb64_fh]));

    auto fnt = fontManager.fonts[font[qb64_fh]];
    auto face = fnt->face;

    if (FT_IS_SCALABLE(face))
        return ((FT_Pos)face->height * fnt->defaultHeight) / (FT_Pos)face->units_per_EM;

    return fnt->defaultHeight;
}

/// @brief This renders text on an active destination (graphics mode only) using the currently selected color
/// @param start_x The starting x position
/// @param start_y The starting y position
/// @param text The text that needs to be rendered
/// @param max_width The maximum width of the text (rendering will be clipped beyond width)
/// @param utf_encoding The UTF encoding of the text (0 = ASCII, 8 = UTF-8, 16 - UTF-16, 32 = UTF-32)
/// @param qb64_fh A QB64 font handle (this can be a builtin font as well)
/// @param passed Optional arguments flag
void sub__UPrintString(int32_t start_x, int32_t start_y, const qbs *text, int32_t max_width, int32_t utf_encoding, int32_t qb64_fh, int32_t passed) {
    libqb_mutex_guard lock(fontManager.m);

    if (is_error_pending() || !text->len)
        return;

    // Check if we are in text mode and generate an error if we are
    if (write_page->text) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }

    FONT_DEBUG_PRINT("Graphics mode set. Proceeding...");

    // Check max width
    if (passed & 1) {
        if (max_width < 1)
            return;
    } else {
        max_width = 0;
    }

    // Check UTF argument
    if (passed & 2) {
        if (!IS_VALID_UTF_ENCODING(utf_encoding)) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return;
        }
    } else {
        utf_encoding = 0;
    }

    // Check if a valid font handle was passed
    if (passed & 4) {
        if (!IS_VALID_QB64_FONT_HANDLE(qb64_fh)) {
            error(QB_ERROR_INVALID_HANDLE);
            return;
        }
    } else {
        qb64_fh = write_page->font; // else get the current write page font handle
    }

    // Convert the string to UTF-32 if needed
    uint32_t const *str32 = nullptr;
    size_t codepoints = 0;

    switch (utf_encoding) {
    case 32: // UTF-32: no conversion needed
        FONT_DEBUG_PRINT("UTF-32 string. Skipping conversion");
        str32 = (uint32_t *)text->chr;
        codepoints = text->len / sizeof(uint32_t);
        break;

    case 16: // UTF-16: conversion required
        FONT_DEBUG_PRINT("UTF-16 string. Converting to UTF32");
        codepoints = utf32.ConvertUTF16(text->chr, text->len);
        if (codepoints)
            str32 = utf32.codepoints.data();
        break;

    case 8: // UTF-8: conversion required
        FONT_DEBUG_PRINT("UTF-8 string. Converting to UTF32");
        codepoints = utf32.ConvertUTF8(text->chr, text->len);
        if (codepoints)
            str32 = utf32.codepoints.data();
        break;

    default: // ASCII: conversion required
        FONT_DEBUG_PRINT("ASCII string. Converting to UTF32");
        codepoints = utf32.ConvertASCII(text->chr, text->len);
        if (codepoints)
            str32 = utf32.codepoints.data();
    }

    if (!codepoints)
        return;

    FontManager::Font *fnt = nullptr;
    FT_Face face = nullptr;
    FT_Vector strPixSize, pen;

    if (qb64_fh < 32) {
        strPixSize.x = codepoints * 8;
        strPixSize.y = qb64_fh;
        pen.x = pen.y = 0;
        FONT_DEBUG_PRINT("Using built-in font %i", qb64_fh);
    } else {
        FONT_DEBUG_CHECK(IS_VALID_FONT_HANDLE(font[qb64_fh]));
        fnt = fontManager.fonts[font[qb64_fh]];
        face = fnt->face;
        strPixSize.x = fnt->GetStringPixelWidth(str32, codepoints);
        pen.x = 0;
        if (FT_IS_SCALABLE(face)) {
            strPixSize.y = (((FT_Pos)face->ascender - (FT_Pos)face->descender) * fnt->defaultHeight) / (FT_Pos)face->units_per_EM;
            pen.y = ((FT_Pos)face->ascender * fnt->defaultHeight) / (FT_Pos)face->units_per_EM;
        } else {
            strPixSize.y = fnt->defaultHeight;
            pen.y = fnt->baseline;
        }

        FONT_DEBUG_PRINT("pen.y = %i", pen.y);
        FONT_DEBUG_PRINT("Using custom font. Scalable = %i", FT_IS_SCALABLE(face));
    }

    if (max_width && max_width < strPixSize.x)
        strPixSize.x = max_width;

    auto drawBuf = (uint8_t *)calloc(strPixSize.x, strPixSize.y);
    if (!drawBuf)
        return;

    FONT_DEBUG_PRINT("Allocated (%lu x %lu) buffer", strPixSize.x, strPixSize.y);

    auto isMonochrome = (write_page->bytes_per_pixel == 1) || ((write_page->bytes_per_pixel == 4) && (write_page->alpha_disabled)) ||
                        (fontflags[qb64_fh] & FONT_LOAD_DONTBLEND); // do we need to do monochrome rendering?

    if (qb64_fh < 32) {
        // Render using a built-in font
        FONT_DEBUG_PRINT("Rendering using built-in font");

        FT_Vector draw, pixmap;
        uint8_t const *builtinFont = nullptr;

        for (size_t i = 0; i < codepoints; i++) {
            auto cp = str32[i];
            if (cp > 255)
                cp = 32; // our built-in fonts only has ASCII glyphs

            if (max_width && pen.x + 8 > start_x + max_width)
                break;

            switch (qb64_fh) {
            case 8:
                builtinFont = &charset8x8[cp][0][0];
                break;

            case 14:
                builtinFont = &charset8x16[cp][1][0];
                break;

            case 16:
                builtinFont = &charset8x16[cp][0][0];
            }

            for (draw.y = pen.y, pixmap.y = 0; pixmap.y < qb64_fh; draw.y++, pixmap.y++) {
                for (draw.x = pen.x, pixmap.x = 0; pixmap.x < 8; draw.x++, pixmap.x++) {
                    *(drawBuf + strPixSize.x * draw.y + draw.x) = *builtinFont++;
                }
            }

            pen.x += 8;
        }
    } else {
        // Render using custom font
        FONT_DEBUG_PRINT("Rendering using TrueType font");

        if (fnt->monospaceWidth) {
            // Monospace rendering
            for (size_t i = 0; i < codepoints; i++) {
                auto cp = str32[i];

                auto glyph = fnt->GetGlyph(cp, isMonochrome);
                if (glyph) {
                    if (max_width && pen.x + fnt->monospaceWidth > start_x + max_width)
                        break;

                    glyph->RenderBitmap(drawBuf, strPixSize.x, strPixSize.y,
                                        pen.x + glyph->bitmap->bearing.x + fnt->monospaceWidth / 2 - glyph->bitmap->advanceWidth / 2,
                                        pen.y - glyph->bitmap->bearing.y);
                    pen.x += fnt->monospaceWidth;
                }
            }
        } else {
            // Variable width rendering
            auto hasKerning = FT_HAS_KERNING(fnt->face); // set to true if font has kerning info
            FontManager::Font::Glyph *glyph = nullptr;
            FontManager::Font::Glyph *previousGlyph = nullptr;

            for (size_t i = 0; i < codepoints; i++) {
                auto cp = str32[i];

                glyph = fnt->GetGlyph(cp, isMonochrome);
                if (glyph) {

                    if (max_width && pen.x + glyph->bitmap->size.x > start_x + max_width)
                        break;

                    // Add kerning advance width if kerning table is available
                    if (hasKerning && previousGlyph && glyph) {
                        FT_Vector delta;
                        FT_Get_Kerning(fnt->face, previousGlyph->index, glyph->index, FT_KERNING_DEFAULT, &delta);
                        pen.x += delta.x >> 6;
                    }

                    glyph->RenderBitmap(drawBuf, strPixSize.x, strPixSize.y, pen.x + glyph->bitmap->bearing.x, pen.y - glyph->bitmap->bearing.y);
                    pen.x += glyph->bitmap->advanceWidth; // add advance width
                    previousGlyph = glyph;                // save the current glyph pointer for use later
                }
            }
        }
    }

    // Resolve coordinates based on current viewport settings
    if (write_page->clipping_or_scaling) {
        if (write_page->clipping_or_scaling == 2) {
            start_x = qbr_float_to_long((float)start_x * write_page->scaling_x + write_page->scaling_offset_x) + write_page->view_offset_x;
            start_y = qbr_float_to_long((float)start_y * write_page->scaling_y + write_page->scaling_offset_y) + write_page->view_offset_y;
        } else {
            start_x += write_page->view_offset_x;
            start_y += write_page->view_offset_y;
        }
    }

    auto alphaSrc = drawBuf;

    // 8-bit / alpha-disabled 32-bit / dont-blend (alpha may still be applied)
    if (isMonochrome) {
        switch (write_page->print_mode) {
        case 3:
            for (pen.y = 0; pen.y < strPixSize.y; pen.y++) {
                for (pen.x = 0; pen.x < strPixSize.x; pen.x++) {
                    if (*alphaSrc++)
                        pset_and_clip(start_x + pen.x, start_y + pen.y, write_page->color);
                    else
                        pset_and_clip(start_x + pen.x, start_y + pen.y, write_page->background_color);
                }
            }
            break;

        case 1:
            for (pen.y = 0; pen.y < strPixSize.y; pen.y++) {
                for (pen.x = 0; pen.x < strPixSize.x; pen.x++) {
                    if (*alphaSrc++)
                        pset_and_clip(start_x + pen.x, start_y + pen.y, write_page->color);
                }
            }
            break;

        case 2:
            for (pen.y = 0; pen.y < strPixSize.y; pen.y++) {
                for (pen.x = 0; pen.x < strPixSize.x; pen.x++) {
                    if (!(*alphaSrc++))
                        pset_and_clip(start_x + pen.x, start_y + pen.y, write_page->background_color);
                }
            }
        }
    } else {
        uint32_t a = image_get_bgra_alpha(write_page->color) + 1;
        uint32_t a2 = image_get_bgra_alpha(write_page->background_color) + 1;
        uint32_t z = image_get_bgra_bgr(write_page->color);
        uint32_t z2 = image_get_bgra_bgr(write_page->background_color);

        switch (write_page->print_mode) {
        case 3: {
            float alpha1 = image_get_bgra_alpha(write_page->color);
            float r1 = image_get_bgra_red(write_page->color);
            float g1 = image_get_bgra_green(write_page->color);
            float b1 = image_get_bgra_blue(write_page->color);
            float alpha2 = image_get_bgra_alpha(write_page->background_color);
            float r2 = image_get_bgra_red(write_page->background_color);
            float g2 = image_get_bgra_green(write_page->background_color);
            float b2 = image_get_bgra_blue(write_page->background_color);
            float dr = r2 - r1;
            float dg = g2 - g1;
            float db = b2 - b1;
            float da = alpha2 - alpha1;
            float cw =
                alpha1 ? alpha2 / alpha1 : 100000; // color weight multiplier, avoids seeing black when transitioning from RGBA(?,?,?,255) to RGBA(0,0,0,0)

            for (pen.y = 0; pen.y < strPixSize.y; pen.y++) {
                for (pen.x = 0; pen.x < strPixSize.x; pen.x++) {
                    float d = *alphaSrc++;
                    d = 255 - d;
                    d /= 255.0f;
                    float alpha3 = alpha1 + da * d;
                    d *= cw;
                    if (d > 1.0f)
                        d = 1.0f;
                    float r3 = r1 + dr * d;
                    float g3 = g1 + dg * d;
                    float b3 = b1 + db * d;
                    pset_and_clip(start_x + pen.x, start_y + pen.y,
                                  image_make_bgra(qbr_float_to_long(r3), qbr_float_to_long(g3), qbr_float_to_long(b3), qbr_float_to_long(alpha3)));
                }
            }
        } break;

        case 1:
            for (pen.y = 0; pen.y < strPixSize.y; pen.y++) {
                for (pen.x = 0; pen.x < strPixSize.x; pen.x++) {
                    if (*alphaSrc)
                        pset_and_clip(start_x + pen.x, start_y + pen.y, ((*alphaSrc * a) >> 8 << 24) + z);
                    ++alphaSrc;
                }
            }
            break;

        case 2:
            for (pen.y = 0; pen.y < strPixSize.y; pen.y++) {
                for (pen.x = 0; pen.x < strPixSize.x; pen.x++) {
                    if (*alphaSrc != 255)
                        pset_and_clip(start_x + pen.x, start_y + pen.y, (((255 - *alphaSrc) * a2) >> 8 << 24) + z2);
                    ++alphaSrc;
                }
            }
        }
    }

    free(drawBuf);
}

/// @brief Calculate the starting pixel positions of each codepoint to an array. First one being zero.
/// This also calculates the pixel position of the last + 1 character.
/// @param text Text for which the data needs to be calculated. This can be unicode encoded
/// @param arr A QB64 LONG array. This should be codepoints + 1 long. If the array is shorter additional calculated data is ignored
/// @param utf_encoding The UTF encoding of the text (0 = ASCII, 8 = UTF-8, 16 - UTF-16, 32 = UTF-32)
/// @param qb64_fh A QB64 font handle (this can be a builtin font as well)
/// @param passed Optional arguments flag
/// @return Total codepoints in `text`
int32_t func__UCharPos(const qbs *text, void *arr, int32_t utf_encoding, int32_t qb64_fh, int32_t passed) {
    libqb_mutex_guard lock(fontManager.m);

    if (is_error_pending() || !text->len)
        return 0;

    // Check if have an array to work with
    // If not then simply return the count of codepoints later
    if (!(passed & 1)) {
        FONT_DEBUG_PRINT("Array not passed");
        arr = nullptr;
    }

    // Check UTF argument
    if (passed & 2) {
        if (!IS_VALID_UTF_ENCODING(utf_encoding)) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return 0;
        }
    } else {
        utf_encoding = 0;
    }

    // Check if a valid font handle was passed
    if (passed & 4) {
        if (!IS_VALID_QB64_FONT_HANDLE(qb64_fh)) {
            error(QB_ERROR_INVALID_HANDLE);
            return 0;
        }
    } else {
        qb64_fh = write_page->font; // else get the current write page font handle
    }

    // Convert the string to UTF-32 if needed
    uint32_t const *str32 = nullptr;
    size_t codepoints = 0;

    switch (utf_encoding) {
    case 32: // UTF-32: no conversion needed
        str32 = (uint32_t *)text->chr;
        codepoints = text->len / sizeof(uint32_t);
        break;

    case 16: // UTF-16: conversion required
        codepoints = utf32.ConvertUTF16(text->chr, text->len);
        if (codepoints)
            str32 = utf32.codepoints.data();
        break;

    case 8: // UTF-8: conversion required
        codepoints = utf32.ConvertUTF8(text->chr, text->len);
        if (codepoints)
            str32 = utf32.codepoints.data();
        break;

    default: // ASCII: conversion required
        codepoints = utf32.ConvertASCII(text->chr, text->len);
        if (codepoints)
            str32 = utf32.codepoints.data();
    }

    // Simply return the codepoint count if we do not have any array
    if (!arr || !codepoints)
        return (int32_t)codepoints;

    auto element = (uint32_t *)((byte_element_struct *)arr)->offset;
    auto elements = ((byte_element_struct *)arr)->length / sizeof(uint32_t);
    FontManager::Font *fnt = nullptr;
    FT_Pos monospaceWidth;

    if (qb64_fh < 32) {
        monospaceWidth = 8; // built-in fonts always have a width of 8
    } else {
        FONT_DEBUG_CHECK(IS_VALID_FONT_HANDLE(font[qb64_fh]));
        fnt = fontManager.fonts[font[qb64_fh]];
        monospaceWidth = fnt->monospaceWidth;
    }

    if (monospaceWidth) {
        FONT_DEBUG_PRINT("Calculating positions for monospaced font");

        // Fixed width font character positions
        for (size_t i = 0; i < codepoints; i++) {
            if (i < elements)
                element[i] = i * monospaceWidth;
        }

        // (element[codepoints] - 1) = the end position of the last char in the string
        if (codepoints < elements)
            element[codepoints] = codepoints * monospaceWidth;
    } else {
        FONT_DEBUG_PRINT("Calculating positions for variable width font");

        // Variable width font character positions
        auto face = fnt->face;
        auto hasKerning = FT_HAS_KERNING(fnt->face); // set to true if font has kerning info
        FontManager::Font::Glyph *glyph = nullptr;
        FontManager::Font::Glyph *previousGlyph = nullptr;
        FT_Pos penX = 0;
        auto isMonochrome = (write_page->bytes_per_pixel == 1) || ((write_page->bytes_per_pixel == 4) && (write_page->alpha_disabled)) ||
                            (fontflags[qb64_fh] & FONT_LOAD_DONTBLEND); // monochrome or AA?

        for (size_t i = 0; i < codepoints; i++) {
            auto cp = str32[i];

            glyph = fnt->GetGlyph(cp, isMonochrome);
            if (glyph) {
                if (i < elements)
                    element[i] = penX;

                // Add kerning advance width if kerning table is available
                if (hasKerning && previousGlyph && glyph) {
                    FT_Vector delta;
                    FT_Get_Kerning(fnt->face, previousGlyph->index, glyph->index, FT_KERNING_DEFAULT, &delta);
                    penX += delta.x >> 6;
                }

                penX += glyph->bitmap->advanceWidth; // add advance width
                previousGlyph = glyph;               // save the current glyph pointer for use later
            }
        }

        // (element[codepoints] - 1) = the end position of the last char in the string
        if (codepoints < elements)
            element[codepoints] = penX;
    }

    return (int32_t)codepoints;
}
