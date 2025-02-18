//-----------------------------------------------------------------------------------------------------
// PCX Loader for QB64-PE by a740g
//
// Bibliography:
// https://github.com/EzArIk/PcxFileType
// https://github.com/mackron/dr_pcx
// http://fileformats.archiveteam.org/wiki/PCX
// https://en.wikipedia.org/wiki/PCX
// https://moddingwiki.shikadi.net/wiki/PCX_Format
//-----------------------------------------------------------------------------------------------------

#include "libqb-common.h"

#include "image.h"
#include "sg_pcx.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class PCXImage {
    // Stream reader for files loaded into memory
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
    };

    ////////////////////////////////////////////////////////////
    // PCX File Structure
    //
    //    Header        128 bytes
    //
    //    Pixel Data    scan0 plane0
    //                  scan0 plane1
    //                  ..
    //                  scan0 planeN
    //                  scan1 plane0
    //                  scan1 plane1
    //                  ..
    //                  scan1 planeN
    //                  ...
    //                  scanM planeN
    //
    //    Palette       0x0C
    //    (8-bit only)  r0,g0,b0
    //                  r1,g1,b1
    //                  ...
    //                  r256,g256,b256
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // struct PCXHeader
    // {
    //     BYTE Manufacturer;  // Constant Flag   10 = ZSoft .PCX
    //     BYTE Version;       // Version Information
    //                         // 0 = Version 2.5
    //                         // 2 = Version 2.8 w/palette information
    //                         // 3 = Version 2.8 w/o palette information
    //                         // 4 = (PC Paintbrush for Windows)
    //                         // 5 = Version 3.0
    //     BYTE Encoding;      // 1 = .PCX run length encoding
    //     BYTE BitsPerPixel;  // Number of bits/pixel per plane (1, 2, 4 or 8)
    //     WORD XMin;          // Picture Dimensions
    //     WORD YMin;          // (Xmin, Ymin) - (Xmax - Ymax) inclusive
    //     WORD XMax;
    //     WORD YMax;
    //     WORD HDpi;          // Horizontal Resolution of creating device
    //     WORD VDpi;          // Vertical Resolution of creating device
    //     BYTE ColorMap[48];  // Color palette for 16-color palette
    //     BYTE Reserved;
    //     BYTE NPlanes;       // Number of color planes
    //     WORD BytesPerLine;  // Number of bytes per scan line per color plane (always even for .PCX files)
    //     WORD PaletteInfo;   // How to interpret palette - 1 = color/BW, 2 = grayscale
    //     BYTE Filler[58];
    // };
    ////////////////////////////////////////////////////////////

    enum Id : uint8_t { ZSoftPCX = 10 };

    enum Version : uint8_t { Version2_5 = 0, Version2_8_Palette = 2, Version2_8_DefaultPalette = 3, Version3_0 = 5 };

    enum Encoding : uint8_t { None = 0, RunLengthEncoded = 1 };

    enum PaletteType : uint8_t { Indexed = 1, Grayscale = 2 };

    static const auto RLEMask = 0xC0u;
    static const auto PaletteMarker = 0x0Cu;

    class Header {
      public:
        Id id = Id::ZSoftPCX;
        Version version = Version::Version3_0;
        Encoding encoding = Encoding::RunLengthEncoded;
        uint8_t bitsPerPixel;
        uint16_t xMin;
        uint16_t yMin;
        uint16_t xMax;
        uint16_t yMax;
        uint16_t hDpi;
        uint16_t vDpi;
        std::vector<uint8_t> colorMap;
        uint8_t reserved = 0;
        uint8_t nPlanes;
        uint16_t bytesPerLine;
        PaletteType paletteInfo;
        std::vector<uint8_t> filler;

      private:
        auto ReadByte(Stream &input) {
            return input.Read<uint8_t>();
        }

        auto ReadUInt16(Stream &input) {
            return input.Read<uint16_t>();
        }

      public:
        Header(Stream &input) : colorMap(48), filler(58) {
            id = (Id)ReadByte(input);
            version = (Version)ReadByte(input);
            encoding = (Encoding)ReadByte(input);
            bitsPerPixel = ReadByte(input);
            xMin = ReadUInt16(input);
            yMin = ReadUInt16(input);
            xMax = ReadUInt16(input);
            yMax = ReadUInt16(input);
            hDpi = ReadUInt16(input);
            vDpi = ReadUInt16(input);
            for (size_t i = 0; i < colorMap.size(); i++)
                colorMap[i] = ReadByte(input);
            reserved = ReadByte(input);
            nPlanes = ReadByte(input);
            bytesPerLine = ReadUInt16(input);
            paletteInfo = (PaletteType)ReadUInt16(input);
            for (size_t i = 0; i < filler.size(); i++)
                filler[i] = ReadByte(input);
        }

        Header() = delete;
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

    // Manages the PCX image palette (dummy for 24bpp images)
    class Palette {
      public:
        static const uint8_t EGAColors = 16;

        enum class EGAPalette : uint8_t { MONO = 0, CGA = 1, EGA = 2 };

        static constexpr uint32_t MONO_PALETTE[] = {0x000000, 0xFFFFFF, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
                                                    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000};
        static constexpr uint32_t CGA_PALETTE[] = {0x000000, 0x00AAAA, 0xAA00AA, 0xAAAAAA, 0x000000, 0x000000, 0x000000, 0x000000,
                                                   0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000};
        static constexpr uint32_t EGA_PALETTE[] = {0x000000, 0x0000A8, 0x00A800, 0x00A8A8, 0xA80000, 0xA800A8, 0xA85400, 0xA8A8A8,
                                                   0x545454, 0x5454FE, 0x54FE54, 0x54FEFE, 0xFE5454, 0xFE54FE, 0xFEFE54, 0xFEFEFE};

      private:
        std::vector<Color> m_palette;

      public:
        Palette() {
            LoadFromEGAPalette(EGAPalette::EGA);
        };

        Palette(size_t size) {
            Resize(size);
        }

        Palette(EGAPalette type) {
            LoadFromEGAPalette(type);
        }

        Palette(const std::vector<uint8_t> &colorMap) {
            LoadFromColorMap(colorMap);
        }

        Palette(Stream &input, size_t size) {
            LoadFromStream(input, size);
        }

        auto GetSize() {
            return m_palette.size();
        }

        auto GetColor(size_t index) {
            return m_palette[index];
        }

        void SetColor(size_t index, Color value) {
            m_palette[index] = value;
        }

        void Resize(size_t size) {
            if (size != 2 && size != 16 && size != 256)
                throw std::runtime_error("Unsupported palette size: " + std::to_string(size));

            m_palette.resize(size);
        }

        void LoadFromEGAPalette(EGAPalette type) {
            const uint32_t *egaPalette;

            switch (type) {
            case EGAPalette::MONO:
                egaPalette = MONO_PALETTE;
                break;

            case EGAPalette::CGA:
                egaPalette = CGA_PALETTE;
                break;

            case EGAPalette::EGA:
                egaPalette = EGA_PALETTE;
                break;

            default:
                throw std::runtime_error("Unsupported EGAPalette type: " + std::to_string(uint8_t(type)));
            }

            m_palette.resize(16);

            for (auto i = 0; i < 16; i++)
                m_palette[i] = Color(uint8_t((egaPalette[i] >> 16) & 0xff), uint8_t((egaPalette[i] >> 8) & 0xff),
                                     uint8_t((egaPalette[i]) & 0xff)); // NOTE: The color order the array is RGB
        }

        void LoadFromColorMap(const std::vector<uint8_t> &colorMap) {
            if (colorMap.size() != 48)
                throw std::runtime_error("Trying to read an unsupported palette size (" + std::to_string(colorMap.size()) + ") from a header ColorMap");

            auto index = 0;
            for (auto i = 0; i < 16; i++) {
                Color color;

                // WARNING: Load order is important
                color.b = colorMap[index++];
                color.g = colorMap[index++];
                color.r = colorMap[index++];
                color.a = 255;

                m_palette[i] = color;
            }
        }

        void LoadFromStream(Stream &input, size_t size) {
            if (size != 16 && size != 256)
                throw std::runtime_error("Unsupported palette size: " + std::to_string(size));

            m_palette.resize(size);

            for (size_t i = 0; i < m_palette.size(); ++i) {
                Color color;

                // WARNING: Read order is important
                color.b = input.Read<uint8_t>();
                color.g = input.Read<uint8_t>();
                color.r = input.Read<uint8_t>();
                color.a = 255;

                m_palette[i] = color;
            }
        }
    };

    // RLE decoder class
    class ByteReader {
        Stream &m_stream;
        bool isRLE;
        uint32_t m_count;
        uint8_t m_rleValue;

      public:
        ByteReader(Stream &stream, bool isRLE) : m_stream(stream), isRLE(isRLE), m_count(0), m_rleValue(0) {}

        uint8_t ReadByte() {
            if (isRLE) {
                if (m_count > 0) {
                    m_count--;
                    return m_rleValue;
                }

                auto code = m_stream.Read<uint8_t>();

                if ((code & RLEMask) == RLEMask) {
                    m_count = uint32_t(code & (RLEMask ^ 0xff));
                    m_rleValue = m_stream.Read<uint8_t>();

                    m_count--;
                    return m_rleValue;
                }

                return code;
            }

            return m_stream.Read<uint8_t>();
        }

        void Reset() {
            m_count = 0;
            m_rleValue = 0;
        }

        ByteReader() = delete;
    };

    // Classes to handle reading indices of various bit depths from encoded streams
    class IndexReader {
        ByteReader &m_reader;
        uint32_t m_bitsPerPixel;
        uint32_t m_bitMask;
        uint32_t m_bitsRemaining = 0;
        uint32_t m_byteRead;

      public:
        IndexReader(ByteReader &reader, uint32_t bitsPerPixel) : m_reader(reader), m_bitsPerPixel(bitsPerPixel) {
            if (!(bitsPerPixel == 1 || bitsPerPixel == 2 || bitsPerPixel == 4 || bitsPerPixel == 8))
                throw std::runtime_error("bitsPerPixel must be 1, 2, 4 or 8. Got: " + std::to_string(bitsPerPixel));

            m_bitMask = uint32_t((1 << (int)m_bitsPerPixel) - 1);
        }

        uint32_t ReadIndex() {
            // NOTE: This does not work for non-power-of-two bits per pixel (e.g. 6) since it does not concatenate shift adjacent bytes together

            if (m_bitsRemaining == 0) {
                m_byteRead = m_reader.ReadByte();
                m_bitsRemaining = 8;
            }

            // NOTE: Reads from the most significant bits
            uint32_t index = (m_byteRead >> int(8 - m_bitsPerPixel)) & m_bitMask;
            m_byteRead <<= int(m_bitsPerPixel);
            m_bitsRemaining -= m_bitsPerPixel;

            return index;
        }
    };

  public:
    void LoadFromMemory(const void *in_data, size_t in_dataSize, uint32_t **out_data, int *out_x, int *out_y) {
        if (!in_data || !in_dataSize || !out_x || !out_y) {
            image_log_error("Invalid parameters: in_data=%p, in_dataSize=%llu, out_x=%p, out_y=%p", in_data, in_dataSize, out_x, out_y);

            return;
        }

        // Prepare the memory input stream
        Stream input(reinterpret_cast<const uint8_t *>(in_data), in_dataSize);

        // Load and validate header
        Header header(input);

        if (header.id != Id::ZSoftPCX) {
            image_log_error("Not a PCX file");

            return;
        }

        if (header.version != Version::Version3_0 && header.version != Version::Version2_8_Palette && header.version != Version::Version2_8_DefaultPalette &&
            header.version != Version::Version2_5) {
            image_log_error("Unsupported PCX version: %d", int(header.version));

            return;
        }

        if (header.bitsPerPixel != 1 && header.bitsPerPixel != 2 && header.bitsPerPixel != 4 && header.bitsPerPixel != 8) {
            image_log_error("Unsupported PCX bits per pixel: %d", int(header.bitsPerPixel));

            return;
        }

        auto width = header.xMax - header.xMin + 1;
        auto height = header.yMax - header.yMin + 1;
        if (width < 0 || height < 0 || width > 0xffff || height > 0xffff) {
            image_log_error("Invalid image dimensions: (%d, %d) - (%d, %d)", header.xMin, header.yMin, header.xMax, header.yMax);

            return;
        }

        // Pixels per line, including PCX's even-number-of-pixels buffer
        auto pixelsPerLine = header.bytesPerLine * 8 /*bitsPerByte*/ / header.bitsPerPixel;

        // Bits per pixel, including all bit planes
        auto bpp = header.bitsPerPixel * header.nPlanes;

        if (bpp != 1 && bpp != 2 && bpp != 4 && bpp != 8 && bpp != 24 && bpp != 32) {
            image_log_error("Unsupported PCX bit depth: %d", bpp);

            return;
        }

        image_log_trace("Loading: %i x %i pixels @ %i bpp, %i planes, %i bits / plane", width, height, bpp, int(header.nPlanes), int(header.bitsPerPixel));

        // Load the palette
        Palette palette;

        if (bpp == 1) {
            // HACK: Monochrome images don't always include a reasonable palette in v3.0.
            // Default them to black and white in all cases

            palette.LoadFromEGAPalette(Palette::EGAPalette::MONO);
        } else if (bpp < 8) {
            // 16-color palette in the ColorMap portion of the header

            switch (header.version) {
            case Version::Version2_5:
            case Version::Version2_8_DefaultPalette: {
                switch (bpp) {
                // 4-color CGA palette
                case 2:
                    palette.LoadFromEGAPalette(Palette::EGAPalette::CGA);
                    break;

                // 16-color EGA palette
                default:
                case 4:
                    palette.LoadFromEGAPalette(Palette::EGAPalette::EGA);
                    break;
                }

                break;
            }

            default:
            case Version::Version2_8_Palette:
            case Version::Version3_0: {
                palette.LoadFromColorMap(header.colorMap);
                break;
            }
            }
        } else if (bpp == 8) {
            // 256-color palette is saved at the end of the file, with one byte marker

            auto dataPosition = input.GetPosition();

            input.Seek(input.GetSize() - (1 + (256 * 3)));

            if (input.Read<uint8_t>() != PaletteMarker) {
                image_log_error("PCX palette marker not present in file");

                return;
            }

            palette.LoadFromStream(input, 256);

            input.Seek(dataPosition);
        } else {
            // Dummy palette for 32-bit and 24-bit images

            palette.Resize(256);
        }

        // Load the pixel data
        *out_data = reinterpret_cast<uint32_t *>(malloc(width * height * sizeof(uint32_t)));
        if (!(*out_data)) {
            image_log_error("Failed to allocate %lld bytes", width * height * sizeof(uint32_t));

            return;
        }

        memset(*out_data, 0xff, width * height * sizeof(uint32_t));

        // Accumulate indices across bit planes
        std::vector<uint32_t> indexBuffer(width);

        ByteReader byteReader(input, header.encoding == Encoding::RunLengthEncoded);
        std::unique_ptr<IndexReader> indexReader;

        for (int y = 0; y < height; y++) {
            auto dstRow = &(*out_data)[y * width];
            indexBuffer.assign(width, 0);

            size_t offset = 0;

            // Decode the RLE byte stream
            byteReader.Reset();

            // Read indices of a given length out of the byte stream
            indexReader = std::make_unique<IndexReader>(byteReader, header.bitsPerPixel);

            // Planes are stored consecutively for each scan line
            for (int plane = 0; plane < header.nPlanes; plane++) {
                for (int x = 0; x < pixelsPerLine; x++) {
                    auto index = indexReader->ReadIndex();

                    // Account for padding bytes
                    if (x < width) {
                        indexBuffer[x] |= (index << (plane * header.bitsPerPixel));
                    }
                }
            }

            for (int x = 0; x < width; x++) {
                uint32_t index = indexBuffer[x];

                switch (bpp) {
                case 32:
                    dstRow[offset] = index;
                    break;

                case 24:
                    dstRow[offset] = Color(image_get_bgra_blue(index), image_get_bgra_green(index), image_get_bgra_red(index)).value;
                    break;

                default:
                    dstRow[offset] = palette.GetColor(index).value;
                }

                ++offset;
            }
        }

        *out_x = width;
        *out_y = height;
    }
};

uint32_t *pcx_load_memory(const void *data, size_t dataSize, int *x, int *y, int *components) {
    uint32_t *out_data = nullptr;

    try {
        std::unique_ptr<PCXImage> pcx = std::make_unique<PCXImage>(); // use unique_ptr for memory management

        pcx->LoadFromMemory(data, dataSize, &out_data, x, y);
        *components = 4; // always 32bpp BGRA
    } catch (const std::exception &e) {
        image_log_error("Failed to load PCX: %s", e.what());

        if (out_data) {
            // Just in case this was allocated
            free(out_data);
            out_data = nullptr;
        }
    }

    return out_data;
}

uint32_t *pcx_load_file(const char *filename, int *x, int *y, int *components) {
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

    return pcx_load_memory(&buffer[0], len, x, y, components);
}
