//----------------------------------------------------------------------------------------------------------------------
// QB64-PE Font Library
// Powered by FreeType 2.4.12 (https://github.com/vinniefalco/FreeTypeAmalgam)
//----------------------------------------------------------------------------------------------------------------------

#define FONT_DEBUG 0
#include "font.h"
#include "freetypeamalgam.h"
#include "gui.h"
#include "libqb-common.h"
#include <cmath>
#include <unordered_map>
#include <vector>

// QB64 expects invalid font handles to be zero
#define IS_FONT_HANDLE_VALID(_handle_) ((_handle_) > INVALID_FONT_HANDLE && (_handle_) < fontManager.fonts.size() && fontManager.fonts[_handle_]->isUsed)

// See FontManager::Font::utf8Decode() below for more details
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

// clang-format off
/// @brief See FontManager::Font::utf8Decode() below for more details
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

/// @brief This class manages all font handles, bitmaps, hashmaps of glyph bitmaps etc.
struct FontManager {
    FT_Library library;       // FreeType library object
    int32_t lowestFreeHandle; // the lowest free handle that can be allocated
    int32_t reservedHandle;   // this is set to handle 0 so that it is not returned to QB64

    /// @brief Manages a single font
    struct Font {
        bool isUsed;              // is this handle in use?
        FT_Byte *fontData;        // raw font data (we always store a copy as long as the font is in use)
        FT_Face face;             // FreeType face object
        FT_Pos monospaceWidth;    // the monospace width (if font was loaded as monospace, else zero)
        FT_Pos defaultHeight;     // default (max) pixel height the user wants
        FT_Pos baseline;          // font baeline in pixels
        FT_ULong *utf32Codepoint; // UTF32 dynamic codepoint buffer used for conversion from ASCII / UTF-8
        FT_ULong utf32Codepoints; // the length of utf32Codepoint

        /// @brief Manages a single glyph in a font
        struct Glyph {
            FT_UInt index;       // glyph index
            FT_Byte *bitmapMono; // raw monochrome bitmap in 8-bit format
            FT_Byte *bitmapGray; // raw anti-aliased bitamp in 8-bit format
            FT_Byte *bitmap;     // the current selected bitmap (mono / gray)
            FT_Vector size;      // bitmap width & height in pixels
            FT_Pos advanceWidth; // glyph advance width in pixels
            FT_Vector bearing;   // glyph left and top side bearing in pixels

            // Delete copy and move constructors and assignments
            Glyph(const Glyph &) = delete;
            Glyph &operator=(const Glyph &) = delete;
            Glyph(Glyph &&) = delete;
            Glyph &operator=(Glyph &&) = delete;

            /// @brief Just initialize everything
            Glyph() {
                bitmapMono = bitmapGray = bitmap = nullptr;
                size.x = size.y = index = advanceWidth = bearing.x = bearing.y = 0;
            }

            /// @brief Frees any cached glyph bitmap
            ~Glyph() {
                FONT_DEBUG_PRINT("Freeing bitmaps %p, %p", bitmapMono, bitmapGray);

                free(bitmapGray);
                free(bitmapMono);
            }

            /// @brief Caches a glyph bitmap with a given codepoint and this happens only once
            /// @param codepoint A valid UTF-32 codepoint
            /// @param parentFont The parent font object
            /// @return True if successful or if bitmap is already cached
            bool CacheBitmap(FT_ULong codepoint, Font *parentFont) {
                if (!bitmap) {
                    // Get the glyph index first and store it
                    // Note that this can return a valid glyph index but the index need not have any glyph bitmap
                    index = FT_Get_Char_Index(parentFont->face, codepoint);
                    if (!index) {
                        FONT_DEBUG_PRINT("Got glyph index zero for codepoint %lu", codepoint);
                    }

                    // Load the glyph to query details and render
                    if (FT_Load_Glyph(parentFont->face, index, FT_LOAD_DEFAULT)) {
                        FONT_DEBUG_PRINT("Failed to load glyph for codepoint %lu (%u)", codepoint, index);
                    }

                    // We'll attemot to render the monochrome font first
                    if (FT_Render_Glyph(parentFont->face->glyph, FT_RENDER_MODE_MONO)) {
                        FONT_DEBUG_PRINT("Failed to render glyph for codepoint %lu (%u)", codepoint, index);
                    }

                    size.x = parentFont->face->glyph->bitmap.width;         // get the width of the bitmap
                    size.y = parentFont->face->glyph->bitmap.rows;          // get the height of the bitmap
                    advanceWidth = parentFont->face->glyph->advance.x / 64; // get the advance width of the glyph
                    bearing.x = parentFont->face->glyph->bitmap_left;       // get the bitmap left side bearing
                    bearing.y = parentFont->face->glyph->bitmap_top;        // get the bitmap top side bearing

                    if (!parentFont->face->glyph->bitmap.buffer || size.x < 1 || size.y < 1) {
                        // Ok, this means the font does not have a glyph for the codepoint index
                        // Simply make a blank bitmap and update width and height
                        size.x = std::max(advanceWidth, size.x);
                        if (size.x < 1) {
                            FONT_DEBUG_PRINT("Failed to get default size for empty glyph");

                            return false; // something seriously went wrong
                        }
                        size.y = parentFont->defaultHeight;

                        FONT_DEBUG_PRINT("Creating empty %i x %i glyph for missing codepoint %u (%i)", size.x, size.y, codepoint, index);

                        // Allocate zeroed memory for monochrome and gray bitmaps
                        bitmapGray = (uint8_t *)calloc(size.x, size.y);
                        if (bitmapGray) {
                            bitmapMono = (uint8_t *)calloc(size.x, size.y);
                            if (!bitmapMono) {
                                free(bitmapGray);
                                bitmapGray = nullptr;
                            }
                        }
                    } else {
                        // Allocate zeroed memory for monochrome and gray bitmaps
                        bitmapGray = (uint8_t *)calloc(size.x, size.y);
                        if (bitmapGray) {
                            bitmapMono = (uint8_t *)calloc(size.x, size.y);
                            if (!bitmapMono) {
                                free(bitmapGray);
                                bitmapGray = nullptr;
                            }
                        }

                        // Proceed only if both allocations were successful
                        if (bitmapGray && bitmapMono) {
                            // We already have the mono bitmap rendered so simply copy that first
                            // We simply use 255 for 1 and 0 for 0 with nothing in between
                            auto src = parentFont->face->glyph->bitmap.buffer;
                            auto dst = bitmapMono;

                            for (auto y = 0; y < size.y; y++, src += parentFont->face->glyph->bitmap.pitch, dst += size.x) {
                                for (auto x = 0; x < size.x; x++) {
                                    dst[x] = (((src[x / 8]) >> (7 - (x & 7))) & 1) * 255; // this looks at each bit and then sets the pixel
                                }
                            }

                            // Render the bitmap in gray mode
                            if (FT_Load_Char(parentFont->face, codepoint, FT_LOAD_RENDER) || FT_Render_Glyph(parentFont->face->glyph, FT_RENDER_MODE_NORMAL)) {
                                FONT_DEBUG_PRINT("Failed to render gray glyph for codepoint %lu (%u)", codepoint, index);

                                // Simply copy the mono one to the gray as a fallback
                                memcpy(bitmapGray, bitmapMono, size.x * size.y);

                                FONT_DEBUG_PRINT("Using monochrome bitmap for gray");
                            } else {
                                // Now copy the 8-bit bitmap
                                auto src = parentFont->face->glyph->bitmap.buffer;
                                auto dst = bitmapGray;

                                for (auto y = 0; y < size.y; y++, src += parentFont->face->glyph->bitmap.pitch, dst += size.x) {
                                    memcpy(dst, src, size.x); // simply copy the line
                                }
                            }
                        }
                    }

                    FONT_DEBUG_PRINT("Bitmap cached (%p, %p) for codepoint %u", bitmapGray, bitmapMono, codepoint);
                    FONT_DEBUG_PRINT("I = %i, W = %i, H = %i, AW = %i, BX = %i, BY = %i", index, size.x, size.y, advanceWidth, bearing.x, bearing.y);

                    bitmap = bitmapGray; // set bitmap to gray bitmap by default
                }

                return bitmap != nullptr;
            }

            /// @brief Renders the glyph bitmap to the target bitmap using alpha blending
            /// @param dst The target bitmap to render to
            /// @param dstW The width of the target bitmap
            /// @param dstH The height of the target bitmap
            /// @param dstL The x position on the target bitmap where the rendering should start
            /// @param dstT The y position on the target bitmap where the rendering should start
            /// @return True if successful
            bool RenderBitmapBlend(uint8_t *dst, int dstW, int dstH, int dstL, int dstT) {
                if (!bitmap || !dst)
                    return false;

                auto dstR = dstL + size.x; // right of dst + 1 where we will end
                auto dstB = dstT + size.y; // bottom of dst + 1 where we will end
                for (auto dy = dstT, sy = 0; dy < dstB; dy++, sy++) {
                    for (auto dx = dstL, sx = 0; dx < dstR; dx++, sx++) {
                        if (dx >= 0 && dx < dstW && dy >= 0 && dy < dstH) { // if we are not clipped
                            auto dstP = (dst + dstW * dy + dx);             // dst pointer
                            int alphaSrc = *(bitmap + size.x * sy + sx);    // src alpha from src pointer
                            if (alphaSrc > *dstP)                           // blend both alpha and save to dst pointer
                                *dstP = alphaSrc;
                        }
                    }
                }

                return true;
            }

            /// @brief Blits the glyph bitmap to the target bitmap
            /// @param dst The target bitmap to render to
            /// @param dstW The width of the target bitmap
            /// @param dstH The height of the target bitmap
            /// @param dstL The x position on the target bitmap where the rendering should start
            /// @param dstT The y position on the target bitmap where the rendering should start
            /// @return True if successful
            bool RenderBitmapBlit(uint8_t *dst, int dstW, int dstH, int dstL, int dstT) {
                if (!bitmap || !dst)
                    return false;

                auto dstR = dstL + size.x; // right of dst + 1 where we will end
                auto dstB = dstT + size.y; // bottom of dst + 1 where we will end
                for (auto dy = dstT, sy = 0; dy < dstB; dy++, sy++) {
                    for (auto dx = dstL, sx = 0; dx < dstR; dx++, sx++) {
                        if (dx >= 0 && dx < dstW && dy >= 0 && dy < dstH) {         // if we are not clipped
                            *(dst + dstW * dy + dx) = *(bitmap + size.x * sy + sx); // copy the pixel
                        }
                    }
                }

                return true;
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
            monospaceWidth = defaultHeight = baseline = utf32Codepoints = 0;
            utf32Codepoint = nullptr;
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

            // Free any UTF32 conversion buffer
            free(utf32Codepoint);
            FONT_DEBUG_PRINT("UTF32 conversion buffer freed");

            FONT_DEBUG_PRINT("Freeing cached glyphs");
            // Free any allocated glyph manager
            // This should also call the glyphs destructor freeing the bitmap data
            for (auto &it : glyphs)
                delete it.second;
        }

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
        /// @param codep The decoded codepoint after state changes to UTF8_ACCEPT
        /// @param byte The current byte being processed in the UTF-8 string
        /// @return UTF8_ACCEPT if enough bytes have been read for a character,
        /// UTF8_REJECT if the byte is not allowed to occur at its position,
        /// and some other positive value if more bytes have to be read
        uint32_t UTF8Decode(uint32_t *state, uint32_t *codep, uint32_t byte) {
            uint32_t type = utf8d[byte];
            *codep = (*state != UTF8_ACCEPT) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);
            *state = utf8d[256 + *state * 16 + type];
            return *state;
        }

        /// @brief Resizes the UTF32 conversion buffer
        /// @param codepoints New codepoints required
        /// @return True if the buffer was allocated correctly
        bool ResizeCodepointBuffer(FT_ULong codepoints) {
            if (codepoints <= utf32Codepoints || !codepoints) {
                utf32Codepoints = codepoints;
                return utf32Codepoint != nullptr;
            }

            auto *tempBuffer = (FT_ULong *)realloc(utf32Codepoint, codepoints * sizeof(FT_ULong));
            if (!tempBuffer)
                return false;
            utf32Codepoint = tempBuffer;

            FONT_DEBUG_PRINT("UTF32 conversion buffer resized from %i to %i", utf32Codepoints, codepoints);

            utf32Codepoints = codepoints;

            return true;
        }

        /// @brief Converts an ASCII string to UTF-32 to an internal buffer
        /// @param codepoint The ASCII string
        /// @param codepoints The number of characters in the string
        /// @return True if successful
        bool ConvertASCIIToUTF32(const FT_Bytes codepoint, FT_ULong codepoints) {
            // Resize the codepoint buffer
            if (ResizeCodepointBuffer(codepoints)) {
                // Convert the ASCII string
                for (FT_ULong i = 0; i < codepoints; i++)
                    utf32Codepoint[i] = codepage437_to_unicode16[codepoint[i]];

                return true;
            }

            return false;
        }

        /// @brief Creates a glyph belonging to a codepoint, caches its bitmap + info and adds it to the hash map
        /// @param codepoint A valid UTF-32 codepoint
        /// @return True if successful or if the glyph is already in the map
        bool CacheGlyph(FT_ULong codepoint) {
            if (glyphs.count(codepoint) == 0) {
                auto newGlyph = new Glyph;

                if (!newGlyph) {
                    FONT_DEBUG_PRINT("Failed to allocate mmemory");

                    return false; // failed to allocate memory
                }

                // Cache the glyph info and bitmap
                if (!newGlyph->CacheBitmap(codepoint, this)) {
                    delete newGlyph;

                    FONT_DEBUG_PRINT("Failed to cache glyph data");

                    return false; // failed to cache bitmap
                }

                // Ok we are good. Save the Glyph address to the map using the codepoint as key
                glyphs[codepoint] = newGlyph;

                FONT_DEBUG_PRINT("Glyph data for codepoint %u successfully cached", codepoint);
            }

            return true; // we already have the glyph cached or the above went well
        }

        /// @brief This returns the length of a UTF32 codepoint array in pixels
        /// @param codepoint The codepoint array (string)
        /// @param codepoints The number of codepoints in the array
        /// @return The length of the string in pixels
        size_t GetStringPixelWidth(const FT_ULong *codepoint, FT_ULong codepoints) {
            if (monospaceWidth) // return monospace width simply by multiplying the fixed width by the codepoints
                return (size_t)monospaceWidth * (size_t)codepoints;

            size_t width = 0;                       // the calculated width in pixel
            auto hasKerning = FT_HAS_KERNING(face); // set to true if font has kerning info

            for (FT_ULong i = 0; i < codepoints; i++) {
                auto cp = codepoint[i];

                if (CacheGlyph(cp)) {
                    auto glyph = glyphs[cp];

                    width += glyph->advanceWidth; // add advance width

                    // Add kerning advance width if kerning table is available
                    auto j = i + 1;
                    if (hasKerning && j < codepoints) {
                        auto cp2 = codepoint[j];

                        if (CacheGlyph(cp2)) {
                            FT_Vector delta;

                            if (FT_Get_Kerning(face, glyph->index, glyphs[cp2]->index, FT_KERNING_DEFAULT, &delta)) {
                                FONT_DEBUG_PRINT("Failed to get kerning information for %lu -> %lu", cp, cp2);
                            }

                            width += delta.x / 64;
                        }
                    }
                }
            }

            return width;
        }

        /// @brief This returns the length of a recently converted UTF32 codepoint array in pixels
        /// @return The length of the string in pixels
        size_t GetStringPixelWidth() { return GetStringPixelWidth(utf32Codepoint, utf32Codepoints); }
    };

    std::vector<Font *> fonts; // vector that holds all font objects

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

        lowestFreeHandle = 0;
        reservedHandle = -1; // we cannot set 0 here since 0 is a valid internal handle

        // Reserve handle 0 so that nothing else can use it
        // We are doing this becase QB64 treats handle 0 as invalid
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
            // Ideally this should execute in extremely few (if at all) senarios
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
        fonts[h]->utf32Codepoint = nullptr;
        fonts[h]->utf32Codepoints = 0;
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

            // Free any UTF32 conversion buffer
            free(fonts[handle]->utf32Codepoint);
            fonts[handle]->utf32Codepoint = nullptr;
            FONT_DEBUG_PRINT("UTF32 conversion buffer freed");

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
                sprintf_s(pathName, PATH_BUFFER_SIZE, FONT_PATHS[i][0], getenv(FONT_PATHS[i][1]), file_path_name);
            else
                sprintf_s(pathName, PATH_BUFFER_SIZE, FONT_PATHS[i][0], "", file_path_name);

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
/// @param options 16=monospace (all old flags are ignored like it always was since forever)
/// @return A valid font handle (> 0) or 0 on failure
int32_t FontLoad(const uint8_t *content_original, int32_t content_bytes, int32_t default_pixel_height, int32_t which_font, int32_t options) {
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
    fontManager.fonts[h]->baseline =
        lroundf((((float)fontManager.fonts[h]->face->size->metrics.ascender / 64.0f) / ((float)fontManager.fonts[h]->face->size->metrics.height / 64.0f)) *
                (float)default_pixel_height);

    if (options & FONT_LOAD_MONOSPACE) {
        // Get the width of upper case W
        if (FT_Load_Char(fontManager.fonts[h]->face, 'W', FT_LOAD_DEFAULT)) {
            FONT_DEBUG_PRINT("FT_Load_Char() 'W' failed");
        }
        fontManager.fonts[h]->monospaceWidth = fontManager.fonts[h]->face->glyph->advance.x / 64; // save the width

        // Get the width of upper case M
        if (FT_Load_Char(fontManager.fonts[h]->face, 'M', FT_LOAD_DEFAULT)) {
            FONT_DEBUG_PRINT("FT_Load_Char() 'M' failed");
        }
        fontManager.fonts[h]->monospaceWidth =
            std::max(fontManager.fonts[h]->monospaceWidth, fontManager.fonts[h]->face->glyph->advance.x / 64); // save the max of both

        FONT_DEBUG_PRINT("Monospace font (width = %li) requested", fontManager.fonts[h]->monospaceWidth);
    }

    FONT_DEBUG_PRINT("Font (height = %i, index = %i) successfully initialized", default_pixel_height, which_font);

    return h;
}

/// @brief Frees the font and any locally cached data
/// @param fh A valid font handle
void FontFree(int32_t fh) {
    if (IS_FONT_HANDLE_VALID(fh))
        fontManager.ReleaseHandle(fh);
}

/// @brief Returns the font width
/// @param fh A valid font handle
/// @return The width of the font if the font is monospaced or zero otherwise
int32_t FontWidth(int32_t fh) {
    FONT_DEBUG_CHECK(IS_FONT_HANDLE_VALID(fh));

    if (fontManager.fonts[fh]->monospaceWidth)
        return fontManager.fonts[fh]->monospaceWidth;

    FONT_DEBUG_PRINT("Font width for variable width font %i requested", fh);

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
    FONT_DEBUG_CHECK(IS_FONT_HANDLE_VALID(fh));

    auto font = fontManager.fonts[fh];

    // Safety
    *out_data = nullptr;
    *out_x = 0;
    *out_y = font->defaultHeight;

    if (codepoints <= 0)
        return codepoints == 0; // true if zero, false if -ve

    auto isMonochrome = options & FONT_RENDER_MONOCHROME;                                  // do we need to do monochrome rendering?
    auto outBufW = font->GetStringPixelWidth((FT_ULong *)codepoint, (FT_ULong)codepoints); // get the total buffer width
    auto outBufH = (size_t)font->defaultHeight;                                            // height is always set by the QB64
    auto outBuf = (uint8_t *)calloc(outBufW, outBufH);
    if (!outBuf)
        return false;

    FONT_DEBUG_PRINT("Allocated %llu x %llu buffer", outBufW, outBufH);

    auto outX = 0;

    if (font->monospaceWidth) {
        for (auto i = 0; i < codepoints; i++) {
            auto cp = codepoint[i];

            if (font->CacheGlyph(cp)) {
                auto glyph = font->glyphs[cp];
                glyph->bitmap = isMonochrome ? glyph->bitmapMono : glyph->bitmapGray; // select monochrome or gray bitmap

                if (glyph->RenderBitmapBlit(outBuf, outBufW, outBufH, outX + glyph->bearing.x + font->monospaceWidth / 2 - glyph->advanceWidth / 2,
                                            font->baseline - glyph->bearing.y)) {
                    outX += font->monospaceWidth;
                }
            }
        }
    } else {
        auto hasKerning = FT_HAS_KERNING(font->face); // set to true if font has kerning info

        for (auto i = 0; i < codepoints; i++) {
            auto cp = codepoint[i];

            if (font->CacheGlyph(cp)) {
                auto glyph = font->glyphs[cp];
                glyph->bitmap = isMonochrome ? glyph->bitmapMono : glyph->bitmapGray; // select monochrome or gray bitmap

                if (glyph->RenderBitmapBlend(outBuf, outBufW, outBufH, outX + glyph->bearing.x, font->baseline - glyph->bearing.y)) {
                    outX += glyph->advanceWidth; // add advance width

                    // Add kerning advance width if kerning table is available
                    auto j = i + 1;
                    if (hasKerning && j < codepoints) {
                        auto cp2 = codepoint[j];

                        if (font->CacheGlyph(cp2)) {
                            FT_Vector delta;

                            if (FT_Get_Kerning(font->face, glyph->index, font->glyphs[cp2]->index, FT_KERNING_DEFAULT, &delta)) {
                                FONT_DEBUG_PRINT("Failed to get kerning information for %lu -> %lu", cp, cp2);
                            }

                            outX += delta.x / 64;
                        }
                    }
                }
            }
        }
    }

    FONT_DEBUG_CHECK(outX == outBufW);

    *out_data = outBuf;
    *out_x = outBufW;
    *out_y = outBufH;

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
        FONT_DEBUG_CHECK(IS_FONT_HANDLE_VALID(fh));

        // Atempt to convert the string to UTF32
        if (fontManager.fonts[fh]->ConvertASCIIToUTF32(codepoint, codepoints)) {
            // Forward to FontRenderTextUTF32()
            return FontRenderTextUTF32(fh, (uint32_t *)fontManager.fonts[fh]->utf32Codepoint, codepoints, options, out_data, out_x, out_y);
        }
    }

    return false;
}

/// @brief Returns the length of an UTF32 codepoint string in pixels
/// @param fh A valid font
/// @param codepoint The UTF32 codepoint array
/// @param codepoints The number of codepoints
/// @return Length in pixels
int32_t FontPrintWidthUTF32(int32_t fh, const uint32_t *codepoint, int32_t codepoints) {
    if (codepoints > 0) {
        FONT_DEBUG_CHECK(IS_FONT_HANDLE_VALID(fh));

        // Get the actual width in pixels
        return fontManager.fonts[fh]->GetStringPixelWidth((FT_ULong *)codepoint, codepoints);
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
        FONT_DEBUG_CHECK(IS_FONT_HANDLE_VALID(fh));

        // Atempt to convert the string to UTF32
        if (fontManager.fonts[fh]->ConvertASCIIToUTF32(codepoint, codepoints)) {
            // Get the actual width in pixels
            return fontManager.fonts[fh]->GetStringPixelWidth();
        }
    }

    return 0;
}
