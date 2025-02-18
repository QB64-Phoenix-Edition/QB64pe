//-----------------------------------------------------------------------------------------------------
// Windows Cursor & Icon Loader for QB64-PE by a740g
//
// Bibliography:
// https://en.wikipedia.org/wiki/ICO_(file_format)
// https://learn.microsoft.com/en-us/previous-versions/ms997538(v=msdn.10)
// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
// https://devblogs.microsoft.com/oldnewthing/20101018-00/?p=12513
// https://devblogs.microsoft.com/oldnewthing/20101019-00/?p=12503
// https://devblogs.microsoft.com/oldnewthing/20101021-00/?p=12483
// https://devblogs.microsoft.com/oldnewthing/20101022-00/?p=12473
// https://books.google.co.in/books?id=LpkFEO2FG8sC&pg=PA318&redir_esc=y#v=onepage&q&f=false
// https://books.google.co.in/books?id=qR6ngUchllsC&pg=PA349&redir_esc=y#v=onepage&q&f=false
// https://www.informit.com/articles/article.aspx?p=1186882
//-----------------------------------------------------------------------------------------------------

#include "libqb-common.h"

#include "../stb/stb_image.h"
#include "../stb/stb_image_write.h"
#include "image.h"
#include "sg_curico.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class CurIcoImage {
    class Stream {
        const uint8_t *buffer;
        size_t size;
        size_t cursor;

        auto Read(uint8_t *data, size_t dataSize) {
            auto bytesToRead = std::min(dataSize, size - cursor);

            if (bytesToRead > 0) {
                std::copy(buffer + cursor, buffer + cursor + bytesToRead, data);
                cursor += bytesToRead;
            }

            return bytesToRead;
        }

      public:
        Stream(const uint8_t *data, size_t dataSize) : buffer(data), size(dataSize), cursor(0) {}

        auto IsEOF() const {
            return cursor >= size;
        }

        auto GetSize() const {
            return size;
        }

        auto GetPosition() const {
            return cursor;
        }

        void Seek(size_t position) {
            if (position <= size)
                cursor = position;
            else
                throw std::runtime_error("Failed to seek to position " + std::to_string(position) + " of " + std::to_string(size));
        }

        template <typename T> T Read() {
            T value = T();

            if (Read(reinterpret_cast<uint8_t *>(&value), sizeof(T)) != sizeof(T))
                throw std::runtime_error("Failed to read " + std::to_string(sizeof(T)) + " byte(s) from position " + std::to_string(cursor) + " of " +
                                         std::to_string(size));

            return value;
        }

        auto GetData() {
            return buffer + cursor;
        }
    };

    enum class Type : uint16_t {
        Icon = 1,
        Cursor,
    };

    struct Header {
        uint16_t reserved;
        Type type;
        uint16_t count;

        Header() : reserved(0), type(Type::Icon), count(0) {}

        Header(Type type) : reserved(0), type(type), count(0) {}

        Header(Stream &stream) {
            ReadFromStream(stream);
        }

        auto IsValid() {
            return reserved == 0 && (type == Type::Icon || type == Type::Cursor) && count > 0;
        }

        static auto GetStructSize() noexcept {
            return 6;
        } // sizeof(Header)

        void ReadFromStream(Stream &stream) {
            reserved = stream.Read<uint16_t>();
            type = Type(stream.Read<uint16_t>());
            count = stream.Read<uint16_t>();
        }

        void WriteToFile(FILE *file) const noexcept {
            fwrite(&reserved, sizeof(reserved), 1, file);
            fwrite(&type, sizeof(type), 1, file);
            fwrite(&count, sizeof(count), 1, file);
        }
    };

    struct DirectoryEntry {
        uint8_t width;
        uint8_t height;
        uint8_t colorCount;
        uint8_t reserved;
        uint16_t planes_xHotspot;
        uint16_t bitCount_yHotspot;
        uint32_t bytesInRes;
        uint32_t imageOffset;

        DirectoryEntry() : width(0), height(0), colorCount(0), reserved(0), planes_xHotspot(0), bitCount_yHotspot(0), bytesInRes(0), imageOffset(0) {}

        static auto GetStructSize() noexcept {
            return 16;
        } // sizeof(DirectoryEntry)

        void ReadFromStream(Stream &stream) {
            width = stream.Read<uint8_t>();
            height = stream.Read<uint8_t>();
            colorCount = stream.Read<uint8_t>();
            reserved = stream.Read<uint8_t>();
            planes_xHotspot = stream.Read<uint16_t>();
            bitCount_yHotspot = stream.Read<uint16_t>();
            bytesInRes = stream.Read<uint32_t>();
            imageOffset = stream.Read<uint32_t>();
        }

        void WriteToFile(FILE *file) const noexcept {
            fwrite(&width, sizeof(width), 1, file);
            fwrite(&height, sizeof(height), 1, file);
            fwrite(&colorCount, sizeof(colorCount), 1, file);
            fwrite(&reserved, sizeof(reserved), 1, file);
            fwrite(&planes_xHotspot, sizeof(planes_xHotspot), 1, file);
            fwrite(&bitCount_yHotspot, sizeof(bitCount_yHotspot), 1, file);
            fwrite(&bytesInRes, sizeof(bytesInRes), 1, file);
            fwrite(&imageOffset, sizeof(imageOffset), 1, file);
        }
    };

    struct BmpInfoHeader {
        uint32_t structSize;
        int32_t width;
        int32_t height;
        uint16_t planes;
        uint16_t bitCount;
        uint32_t compression;
        uint32_t sizeImage;
        int32_t xPelsPerMeter;
        int32_t yPelsPerMeter;
        uint32_t clrUsed;
        uint32_t clrImportant;

        BmpInfoHeader()
            : structSize(sizeof(BmpInfoHeader)), width(0), height(0), planes(0), bitCount(0), compression(0), sizeImage(0), xPelsPerMeter(0), yPelsPerMeter(0),
              clrUsed(0), clrImportant(0) {}

        BmpInfoHeader(uint32_t width, uint32_t height, uint32_t bpp)
            : structSize(sizeof(BmpInfoHeader)), width(width), height(height), planes(0), bitCount(0), compression(0), sizeImage(width * height * bpp / 8),
              xPelsPerMeter(0), yPelsPerMeter(0), clrUsed(0), clrImportant(0) {}

        static auto GetStructSize() noexcept {
            return 40u;
        } // sizeof(BmpInfoHeader)

        void ReadFromStream(Stream &stream) {
            structSize = stream.Read<uint32_t>();
            width = stream.Read<int32_t>();
            height = stream.Read<int32_t>();
            planes = stream.Read<uint16_t>();
            bitCount = stream.Read<uint16_t>();
            compression = stream.Read<uint32_t>();
            sizeImage = stream.Read<uint32_t>();
            xPelsPerMeter = stream.Read<int32_t>();
            yPelsPerMeter = stream.Read<int32_t>();
            clrUsed = stream.Read<uint32_t>();
            clrImportant = stream.Read<uint32_t>();
        }
    };

    // QB64 BGRA friendly color class
    struct Color {
        union {
            struct {
                uint8_t b;
                uint8_t g;
                uint8_t r;
                uint8_t a;
            };

            uint32_t value;
        };

        Color() {
            value = 0;
        }

        Color(uint32_t value) {
            this->value = value;
        }

        Color(uint8_t b, uint8_t g, uint8_t r, uint8_t a = 0xFFu) {
            this->b = b;
            this->g = g;
            this->r = r;
            this->a = a;
        }
    };

    class Palette {
        std::vector<Color> palette;

      public:
        Palette() = default;

        Palette(size_t size) {
            Resize(size);
        }

        Palette(Stream &input, size_t size) {
            ReadFromStream(input, size);
        }

        auto GetSize() {
            return palette.size();
        }

        auto GetColor(size_t index) {
            return palette[index];
        }

        void SetColor(size_t index, Color value) {
            palette[index] = value;
        }

        void Resize(size_t size) {
            if (size > 256)
                throw std::runtime_error("Unsupported palette size: " + std::to_string(size));

            palette.resize(size);
        }

        void ReadFromStream(Stream &input, size_t size) {
            Resize(size);

            for (size_t i = 0; i < palette.size(); i++) {
                Color color;

                // WARNING: The loading order is important
                color.r = input.Read<uint8_t>();
                color.g = input.Read<uint8_t>();
                color.b = input.Read<uint8_t>();
                color.a = input.Read<uint8_t>();
                color.a = 0xff;

                palette[i] = color;
            }
        }
    };

    static constexpr inline auto CalculateStride(int width, int bpp) {
        return ((width * bpp + 31) >> 5) << 2;
    }

    static void MaskToAlpha(const uint8_t *mask, uint32_t *dst, int width, int height) {
        auto stride = CalculateStride(width, 1); // stride in bytes for 1bpp DIB mask

        image_log_info("Width = %d, height = %d, stride = %d", width, height, stride);

        for (auto y = 0; y < height; y++, mask += stride) {
            for (auto x = 0; x < width; x++) {
                // A black pixel in the mask means that the corresponding pixel in the image is copied
                *dst = image_set_bgra_alpha(*dst, (~(mask[x >> 3] >> (7 - (x & 7))) & 0x1) * 0xff);
                ++dst;
            }
        }
    }

    static void FlipVertically(uint32_t *image, int width, int height) {
        auto halfHeight = height >> 1;

        image_log_info("Width = %d, height = %d, halfHeight = %d", width, height, halfHeight);

        for (auto y = 0; y < halfHeight; y++) {
            auto topRow = image + y * width;
            auto bottomRow = image + (height - y - 1) * width;

            // Swap the top row with the bottom row
            std::swap_ranges(topRow, topRow + width, bottomRow);
        }
    }

    /// @brief This function simply writes a dummy header and directory entry to an icon file post which the PNG payload can be written
    /// @param fileName The name of the icon file
    /// @param payloadData A pointer to the PNG payload
    /// @param payloadSize The size of the PNG payload in bytes
    static auto WriteToFile(const char *fileName, const uint8_t *payloadData, uint32_t payloadSize) {
        if (fileName && fileName[0] && payloadData && payloadSize) {
            auto file = fopen(fileName, "wb");

            if (file) {
                // Write the main header
                Header header(Type::Icon);
                header.count = 1;
                IMAGE_DEBUG_CHECK(header.IsValid());
                header.WriteToFile(file);

                // Write the directory entry
                DirectoryEntry directoryEntry;
                directoryEntry.bytesInRes = payloadSize;
                directoryEntry.imageOffset = Header::GetStructSize() + DirectoryEntry::GetStructSize();
                image_log_info("Writing directory entry: bytesInRes = %u, imageOffset = %u", directoryEntry.bytesInRes, directoryEntry.imageOffset);
                directoryEntry.WriteToFile(file);

                image_log_info("Writing payload: size = %u", payloadSize);

                // Write the payload
                if (fwrite(payloadData, sizeof(uint8_t), payloadSize, file) != payloadSize) {
                    fclose(file);
                    image_log_error("Failed to write payload to %s", fileName);
                    return false;
                }

                fclose(file);
                image_log_info("Successfully saved to %s", fileName);
                return true;
            }
        }

        image_log_error("Invalid parameters: fileName=%s, payloadData=%p, payloadSize=%u", fileName, payloadData, payloadSize);
        return false;
    }

  public:
    struct WriteToFileContext {
        const char *fileName;
        bool success;
    };

    static void WriteToFileCallback(void *context, void *data, int size) {
        auto ctx = reinterpret_cast<WriteToFileContext *>(context);
        ctx->success = WriteToFile(ctx->fileName, reinterpret_cast<const uint8_t *>(data), size);
    }

    void LoadFromMemory(const void *in_data, size_t in_dataSize, uint32_t **out_data, int *out_x, int *out_y) {
        if (!in_data || !in_dataSize || !out_x || !out_y) {
            image_log_error("Invalid parameters: in_data=%p, in_dataSize=%llu, out_x=%p, out_y=%p", in_data, in_dataSize, out_x, out_y);

            return;
        }

        Stream input(reinterpret_cast<const uint8_t *>(in_data), in_dataSize);

        Header header(input);
        if (!header.IsValid()) {
            image_log_error("Not an ICO/CUR file");

            return;
        }

        image_log_info("Type = %u, count = %u", unsigned(header.type), header.count);

        std::vector<DirectoryEntry> directory(header.count);

        // Read and probe the entire directory for the best image
        for (size_t i = 0; i < header.count; i++) {
            directory[i].ReadFromStream(input); // load the directory entry

            image_log_info("Width = %u, height = %u, colorCount = %u, bytesInRes = %u, imageOffset = %u", directory[i].width, directory[i].height,
                           directory[i].colorCount, directory[i].bytesInRes, directory[i].imageOffset);
        }

        size_t imageIndex = 0; // use the first image in the directory by default
        bool foundBestImage = false;

        // Find the best image (32bpp or 24bpp)
        for (size_t i = 0; i < header.count; i++) {
            // Simply pick one with the largest byte size
            // Note that this can goof up if the file has mixed 32bpp RGB and PNG images. Not sure why anyone would create such a monstrosity
            if (directory[i].width == 0 && directory[i].height == 0 && directory[i].colorCount == 0 &&
                directory[i].bytesInRes >= directory[imageIndex].bytesInRes) {
                image_log_info("Attempt 1: entry %llu is better than %llu", i, imageIndex);
                foundBestImage = true; // set the flag to true if we find the best image
                imageIndex = i;        // save the index and keep looking
            }
        }

        if (!foundBestImage) {
            // Try again, but this time just check the color count
            for (size_t i = 0; i < header.count; i++) {
                if (directory[i].colorCount == 0 && directory[i].bytesInRes >= directory[imageIndex].bytesInRes) {
                    image_log_info("Attempt 2: entry %llu is better than %llu", i, imageIndex);
                    foundBestImage = true; // set the flag to true if we find the best image
                    imageIndex = i;        // save the index and keep looking
                }
            }

            if (!foundBestImage) {
                // If we still did not find anything then we are probably dealing with a legacy file format. Simply pick one with the largest byte size
                image_log_info("Selecting image with largest bytesInRes");
                for (size_t i = 0; i < header.count; i++) {
                    if (directory[i].bytesInRes >= directory[imageIndex].bytesInRes) {
                        image_log_info("Attempt 3: entry %llu is better than %llu", i, imageIndex);
                        foundBestImage = true; // set the flag to true if we find the best image
                        imageIndex = i;        // save the index and keep looking
                    }
                }
            }
        }

        image_log_info(
            "Selected index = %llu, width = %i, height = %i, colorCount = %u, planes_xHotspot = %u, bitCount_yHotspot = %u, bytesInRes = %u, imageOffset = %u",
            imageIndex, directory[imageIndex].width, directory[imageIndex].height, directory[imageIndex].colorCount, directory[imageIndex].planes_xHotspot,
            directory[imageIndex].bitCount_yHotspot, directory[imageIndex].bytesInRes, directory[imageIndex].imageOffset);

        // Seek the location of the image data
        input.Seek(directory[imageIndex].imageOffset);

        // At this point the image can be a PNG or a Windows DIB
        // We'll use stb_image if we detect that it's a PNG else we'll fallback to our Windows DIB code
        auto imgSig = input.Read<uint32_t>();
        if (imgSig == 0x474e5089u) {
            // We have a PNG
            input.Seek(directory[imageIndex].imageOffset); // seek to the start of the image data

            image_log_info("Loading PNG image");

            int compOut;
            *out_data = reinterpret_cast<uint32_t *>(
                stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(input.GetData()), input.GetSize() - input.GetPosition(), out_x, out_y, &compOut, 4));

            IMAGE_DEBUG_CHECK(compOut == 4);

            if (!*out_data)
                image_log_info("Failed to load PNG: %s", stbi_failure_reason());
        } else if (imgSig == BmpInfoHeader::GetStructSize()) {
            // We have a Windows DIB
            input.Seek(directory[imageIndex].imageOffset); // seek to the start of the image data

            image_log_info("Loading Windows DIB image");

            // Read the DIB header
            BmpInfoHeader bmpInfoHeader;
            bmpInfoHeader.ReadFromStream(input);

            image_log_info("Width = %u, height = %u, bitCount = %u, planes = %u, compression = %u, sizeImage = %u", bmpInfoHeader.width, bmpInfoHeader.height,
                           bmpInfoHeader.bitCount, bmpInfoHeader.planes, bmpInfoHeader.compression, bmpInfoHeader.sizeImage);

            auto width = ::abs(bmpInfoHeader.width);
            auto height = ::abs(bmpInfoHeader.height) >> 1;               // height is always multiplied by 2 due to the mask
            auto colors = 1llu << bmpInfoHeader.bitCount;                 // get the total number of colors in the image
            auto stride = CalculateStride(width, bmpInfoHeader.bitCount); // calculate the stride

            image_log_info("Stride = %d, colors = %llu", stride, colors);

            // Sanity check
            if (width <= 0 || height <= 0 || !colors) {
                image_log_error("Invalid image properties");

                return;
            }

            // Allocate memory for the image
            *out_data = reinterpret_cast<uint32_t *>(malloc(width * height * sizeof(uint32_t)));
            if (!(*out_data)) {
                image_log_error("Failed to allocate %lld bytes", width * height * sizeof(uint32_t));

                return;
            }

            // Read the pixel data
            switch (colors) {
            case (1llu << 1): {
                // This is a 1bpp (monochrome) image

                Palette palette(input, colors);
                image_log_info("Read %llu colors from palette", colors);

                // Read the image
                auto src = input.GetData();
                auto dst = *out_data;

                for (auto y = 0; y < height; y++) {
                    for (auto x = 0; x < width; x++) {
                        *dst = palette.GetColor(((src[x >> 3] >> (7 - (x & 7))) & 1)).value;
                        ++dst;
                    }
                    src += stride;
                }

                // Read and apply the mask
                MaskToAlpha(src, *out_data, width, height);
            } break;

            case (1llu << 4): {
                // This is a 4bpp (16-color) image

                Palette palette(input, colors);
                image_log_info("Read %llu colors from palette", colors);

                // Read the image
                auto src = input.GetData();
                auto dst = *out_data;

                for (auto y = 0; y < height; y++) {
                    for (auto x = 0; x < width; x++) {
                        *dst = palette.GetColor((src[x >> 1] >> ((!(x & 1)) << 2)) & 0xF).value;
                        ++dst;
                    }
                    src += stride;
                }

                // Read and apply the mask
                MaskToAlpha(src, *out_data, width, height);
            } break;

            case (1llu << 8): {
                // This is an 8bpp (256-color) image

                Palette palette(input, colors);
                image_log_info("Read %llu colors from palette", colors);

                // Read the image
                auto src = input.GetData();
                auto dst = *out_data;

                for (auto y = 0; y < height; y++) {
                    for (auto x = 0; x < width; x++) {
                        *dst = palette.GetColor(src[x]).value;
                        ++dst;
                    }
                    src += stride;
                }

                // Read and apply the mask
                MaskToAlpha(src, *out_data, width, height);
            } break;

            case (1llu << 15): {
                // This is a 15bpp (555 format) image

                auto src = input.GetData();
                auto dst = *out_data;

                for (auto y = 0; y < height; y++) {
                    for (auto x = 0; x < width; x++) {
                        auto index = x << 1;
                        auto pixel = src[index] | (src[index + 1] << 8);
                        auto r = uint8_t(image_scale_5bits_to_8bits((pixel >> 10) & 0x1f));
                        auto g = uint8_t(image_scale_5bits_to_8bits((pixel >> 5) & 0x1f));
                        auto b = uint8_t(image_scale_5bits_to_8bits(pixel & 0x1f));
                        *dst = image_make_bgra(r, g, b);
                        ++dst;
                    }
                    src += stride;
                }

                // Read and apply the mask
                MaskToAlpha(src, *out_data, width, height);
            } break;

            case (1llu << 16): {
                // This is a 16bpp (565 format) image

                auto src = input.GetData();
                auto dst = *out_data;

                for (auto y = 0; y < height; y++) {
                    for (auto x = 0; x < width; x++) {
                        auto index = x << 1;
                        auto pixel = src[index] | (src[index + 1] << 8);
                        auto r = uint8_t(image_scale_5bits_to_8bits((pixel >> 11) & 0x1f));
                        auto g = uint8_t(image_scale_6bits_to_8bits((pixel >> 5) & 0x3f));
                        auto b = uint8_t(image_scale_5bits_to_8bits(pixel & 0x1f));
                        *dst = image_make_bgra(r, g, b);
                        ++dst;
                    }
                    src += stride;
                }

                // Read and apply the mask
                MaskToAlpha(src, *out_data, width, height);
            }

            case (1llu << 24): {
                // This is a 24bpp (888 format) image

                auto src = input.GetData();
                auto dst = *out_data;

                for (auto y = 0; y < height; y++) {
                    for (auto x = 0; x < width; x++) {
                        auto index = x * 3;
                        auto r = src[index];
                        auto g = src[index + 1];
                        auto b = src[index + 2];
                        *dst = image_make_bgra(r, g, b);
                        ++dst;
                    }
                    src += stride;
                }

                // Read and apply the mask
                MaskToAlpha(src, *out_data, width, height);
            } break;

            case (1llu << 32): {
                // This is a 32bpp (8888 format) image
                // No stride and no mask is needed

                auto src = input.GetData();
                auto dst = *out_data;
                size_t pixels = width * height;

                for (size_t i = 0; i < pixels; i++) {
                    auto r = *src;
                    ++src;
                    auto g = *src;
                    ++src;
                    auto b = *src;
                    ++src;
                    auto a = *src;
                    ++src;
                    *dst = image_make_bgra(r, g, b, a);
                    ++dst;
                }
            } break;

            default:
                // Unknown pixel format
                image_log_error("Unknown pixel format: %u", bmpInfoHeader.bitCount);

                free(*out_data);
                *out_data = nullptr;

                return;
            }

            // Flip the image
            FlipVertically(*out_data, width, height);

            *out_x = width;
            *out_y = height;
        } else {
            // Unknown image format
            image_log_error("Unknown image format: 0x%08x", imgSig);
        }
    }
};

uint32_t *curico_load_memory(const void *data, size_t dataSize, int *x, int *y, int *components) {
    uint32_t *out_data = nullptr;

    try {
        std::unique_ptr<CurIcoImage> curico = std::make_unique<CurIcoImage>(); // use unique_ptr for memory management

        curico->LoadFromMemory(data, dataSize, &out_data, x, y);
        *components = 4; // always 32bpp BGRA
    } catch (const std::exception &e) {
        image_log_error("Failed to load ICO/CUR: %s", e.what());

        if (out_data) {
            // Just in case this was allocated
            free(out_data);
            out_data = nullptr;
        }
    }

    return out_data;
}

uint32_t *curico_load_file(const char *filename, int *x, int *y, int *components) {
    if (!filename || !filename[0] || !x || !y || !components) {
        image_log_error("Invalid parameters");
        return nullptr;
    }

    auto pFile = fopen(filename, "rb");
    if (!pFile) {
        image_log_error("Failed to open %s", filename);
        return nullptr;
    }

    if (fseek(pFile, 0, SEEK_END)) {
        image_log_error("Failed to seek %s", filename);
        fclose(pFile);
        return nullptr;
    }

    auto len = ftell(pFile);
    if (len < 0) {
        image_log_error("Failed to get length of %s", filename);
        fclose(pFile);
        return nullptr;
    }

    std::vector<uint8_t> buffer(len);

    rewind(pFile);

    if (long(fread(&buffer[0], sizeof(uint8_t), len, pFile)) != len || ferror(pFile)) {
        image_log_error("Failed to read %s", filename);
        fclose(pFile);
        return nullptr;
    }

    fclose(pFile);

    return curico_load_memory(&buffer[0], len, x, y, components);
}

bool curico_save_file(const char *filename, int x, int y, int components, const void *data) {
    if (!filename || !filename[0] || !data || x < 1 || y < 1 || components != sizeof(uint32_t)) {
        image_log_error("Invalid parameters");
        return false;
    }

    // Just write a PNG using the stb_image_writer callback
    CurIcoImage::WriteToFileContext context = {filename, false};
    stbi_write_png_compression_level = 100;
    auto stbiResult = stbi_write_png_to_func(&CurIcoImage::WriteToFileCallback, &context, x, y, components, data, 0) != 0;

    return stbiResult && context.success;
}
