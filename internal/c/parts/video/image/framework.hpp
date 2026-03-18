#pragma once

#include "filepath.h"
#include "image.h"
#include "nanosvg/nanosvg.h"
#include "nanosvg/nanosvgrast.h"
#include "pixelscalers/pixelscalers.h"
#include "qoi/qoi.h"
#include "sg_curico/sg_curico.h"
#include "sg_pcx/sg_pcx.h"
#include "stb/stb_image.h"
#include "tiny_webp/tiny_webp.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

enum class ImageScaler { None = 0, Sxbr2, Sxbr3, Sxbr4, Mmpx2, Hq2xA, Hq2xB, Hq3xA, Hq3xB };

static constexpr int ImageScaleFactor[] = {1, 2, 3, 4, 2, 2, 2, 3, 3};

inline int GetImageScaleFactor(ImageScaler s) {
    return ImageScaleFactor[static_cast<size_t>(s)];
}

class ImageDecoder;
class ImageScalerBase;

class ImageSource {
  public:
    enum class Type { File, Memory };

    static ImageSource FromFile(const char *fileName) {
        ImageSource s;
        s.type = Type::File;
        s.file = fileName;
        return s;
    }

    static ImageSource FromMemory(const uint8_t *data, size_t size) {
        ImageSource s;
        s.type = Type::Memory;
        s.data = data;
        s.size = size;
        return s;
    }

    Type GetType() const {
        return type;
    }

    const char *GetFile() const {
        return file;
    }

    const uint8_t *GetData() const {
        return data;
    }

    size_t GetSize() const {
        return size;
    }

  private:
    Type type = Type::File;
    const char *file = nullptr;
    const uint8_t *data = nullptr;
    size_t size = 0;
};

class Image {
  public:
    using PixelDeleter = void (*)(void *);

    Image() = default;

    Image(uint32_t *pixels, int32_t width, int32_t height, PixelDeleter deleter, bool isVector)
        : pixels(pixels, deleter), width(width), height(height), isVector(isVector) {}

    Image(Image &&) noexcept = default;
    Image &operator=(Image &&) noexcept = default;

    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;

    bool IsValid() const {
        return pixels != nullptr;
    }

    uint32_t *GetData() const {
        return pixels.get();
    }

    int GetWidth() const {
        return width;
    }

    int GetHeight() const {
        return height;
    }

    bool IsVectorGraphics() const {
        return isVector;
    }

    static Image FromDecoder(uint32_t *p, int w, int h, PixelDeleter freeFn, bool isVector) {
        return Image(p, w, h, freeFn, isVector);
    }

    static Image FromScaler(uint32_t *p, int w, int h) {
        return Image(p, w, h, DefaultDelete, false);
    }

    static Image Load(const ImageSource &src, const ImageScalerBase *scaler);

  private:
    static void DefaultDelete(void *p) {
        delete[] reinterpret_cast<uint32_t *>(p);
    }

    std::unique_ptr<uint32_t[], PixelDeleter> pixels{nullptr, DefaultDelete};
    int32_t width = 0;
    int32_t height = 0;
    bool isVector = false;
};

class ImageDecoder {
  public:
    static constexpr int RequiredComponents = sizeof(uint32_t);

    virtual ~ImageDecoder() = default;

    virtual const char *GetName() const = 0;

    virtual bool IsVectorGraphics() const {
        return false;
    }

    virtual uint32_t *LoadFromFile(const char *fileName, int *width, int *height, int scale) const = 0;

    virtual uint32_t *LoadFromMemory(const void *data, size_t size, int *width, int *height, int scale) const = 0;

    virtual Image::PixelDeleter GetFreeFunction() const = 0;
};

class StbImageDecoder : public ImageDecoder {
  public:
    const char *GetName() const override {
        return "stb_image";
    }

    static void FreePixels(void *p) {
        stbi_image_free(p);
    }

    Image::PixelDeleter GetFreeFunction() const override {
        return FreePixels;
    }

    uint32_t *LoadFromFile(const char *fileName, int *w, int *h, int scale) const override {
        (void)scale;
        int comp;
        return reinterpret_cast<uint32_t *>(stbi_load(fileName, w, h, &comp, RequiredComponents));
    }

    uint32_t *LoadFromMemory(const void *data, size_t size, int *w, int *h, int scale) const override {
        (void)scale;
        int comp;
        return reinterpret_cast<uint32_t *>(
            stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(data), static_cast<int>(size), w, h, &comp, RequiredComponents));
    }
};

class WebPImageDecoder : public ImageDecoder {
  public:
    const char *GetName() const override {
        return "webp";
    }

    static void FreePixels(void *p) {
        free(p);
    }

    Image::PixelDeleter GetFreeFunction() const override {
        return FreePixels;
    }

    uint32_t *LoadFromFile(const char *fileName, int *w, int *h, int scale) const override {
        (void)scale;
        return reinterpret_cast<uint32_t *>(twp_read(fileName, w, h, twp_FORMAT_RGBA, 0));
    }

    uint32_t *LoadFromMemory(const void *data, size_t size, int *w, int *h, int scale) const override {
        (void)scale;
        return reinterpret_cast<uint32_t *>(twp_read_from_memory(const_cast<void *>(data), size, w, h, twp_FORMAT_RGBA, 0));
    }
};

class PCXImageDecoder : public ImageDecoder {
  public:
    const char *GetName() const override {
        return "sg_pcx";
    }

    static void FreePixels(void *p) {
        free(p);
    }

    Image::PixelDeleter GetFreeFunction() const override {
        return FreePixels;
    }

    uint32_t *LoadFromFile(const char *fileName, int *w, int *h, int scale) const override {
        (void)scale;
        return pcx_load_file(fileName, w, h);
    }

    uint32_t *LoadFromMemory(const void *data, size_t size, int *w, int *h, int scale) const override {
        (void)scale;
        return pcx_load_memory(data, size, w, h);
    }
};

class QOIImageDecoder : public ImageDecoder {
  public:
    const char *GetName() const override {
        return "qoi";
    }

    static void FreePixels(void *p) {
        free(p);
    }

    Image::PixelDeleter GetFreeFunction() const override {
        return FreePixels;
    }

    uint32_t *LoadFromFile(const char *fileName, int *w, int *h, int scale) const override {
        (void)scale;
        qoi_desc desc;
        auto p = reinterpret_cast<uint32_t *>(qoi_read(fileName, &desc, RequiredComponents));
        if (p) {
            *w = desc.width;
            *h = desc.height;
        }
        return p;
    }

    uint32_t *LoadFromMemory(const void *data, size_t size, int *w, int *h, int scale) const override {
        (void)scale;
        qoi_desc desc;
        auto p = reinterpret_cast<uint32_t *>(qoi_decode(data, static_cast<int>(size), &desc, RequiredComponents));
        if (p) {
            *w = desc.width;
            *h = desc.height;
        }
        return p;
    }
};

class CurIcoImageDecoder : public ImageDecoder {
  public:
    const char *GetName() const override {
        return "sg_curico";
    }

    static void FreePixels(void *p) {
        free(p);
    }

    Image::PixelDeleter GetFreeFunction() const override {
        return FreePixels;
    }

    uint32_t *LoadFromFile(const char *fileName, int *w, int *h, int scale) const override {
        (void)scale;
        return curico_load_file(fileName, w, h);
    }

    uint32_t *LoadFromMemory(const void *data, size_t size, int *w, int *h, int scale) const override {
        (void)scale;
        return curico_load_memory(data, size, w, h);
    }
};

class SVGImageDecoder : public ImageDecoder {
  public:
    const char *GetName() const override {
        return "nanosvg";
    }

    bool IsVectorGraphics() const override {
        return true;
    }

    static void FreePixels(void *p) {
        free(p);
    }

    Image::PixelDeleter GetFreeFunction() const override {
        return FreePixels;
    }

    uint32_t *LoadFromFile(const char *fileName, int *w, int *h, int scale) const override {
        uint32_t *pixels = nullptr;

        auto fp = fopen(fileName, "rb");
        if (!fp)
            return nullptr;

        fseek(fp, 0, SEEK_END);
        auto size = ftell(fp);
        rewind(fp);

        if (size <= 0) {
            fclose(fp);
            return nullptr;
        }

        auto svg = reinterpret_cast<char *>(malloc(size + 1));
        if (!svg) {
            fclose(fp);
            return nullptr;
        }

        fread(svg, 1, size, fp);
        svg[size] = '\0';
        fclose(fp);

        if (strstr(svg, "<svg")) {
            auto img = nsvgParse(svg, "px", 96.0f);
            if (img) {
                pixels = Rasterize(img, w, h, scale);
                nsvgDelete(img);
            }
        }

        free(svg);
        return pixels;
    }

    uint32_t *LoadFromMemory(const void *data, size_t size, int *w, int *h, int scale) const override {
        auto svg = reinterpret_cast<char *>(malloc(size + 1));
        if (!svg)
            return nullptr;

        memcpy(svg, data, size);
        svg[size] = '\0';

        uint32_t *pixels = nullptr;

        if (strstr(svg, "<svg")) {
            auto img = nsvgParse(svg, "px", 96.0f);
            if (img) {
                pixels = Rasterize(img, w, h, scale);
                nsvgDelete(img);
            }
        }

        free(svg);
        return pixels;
    }

  private:
    static uint32_t *Rasterize(NSVGimage *img, int *wOut, int *hOut, int scale) {
        auto rast = nsvgCreateRasterizer();
        if (!rast)
            return nullptr;

        int w = static_cast<int>(img->width * scale);
        int h = static_cast<int>(img->height * scale);

        auto pixels = reinterpret_cast<uint32_t *>(malloc(w * h * sizeof(uint32_t)));
        if (pixels) {
            nsvgRasterize(rast, img, 0, 0, static_cast<float>(scale), reinterpret_cast<unsigned char *>(pixels), w, h, w * RequiredComponents);
            *wOut = w;
            *hOut = h;
        }

        nsvgDeleteRasterizer(rast);
        return pixels;
    }
};

static StbImageDecoder gStbDecoder;
static WebPImageDecoder gWebPDecoder;
static PCXImageDecoder gPcxDecoder;
static QOIImageDecoder gQoiDecoder;
static CurIcoImageDecoder gCurIcoDecoder;
static SVGImageDecoder gSvgDecoder;

static const ImageDecoder *gDecoderRegistry[] = {&gStbDecoder, &gWebPDecoder, &gPcxDecoder, &gQoiDecoder, &gCurIcoDecoder, &gSvgDecoder};

static constexpr size_t DecoderCount = _countof(gDecoderRegistry);

class ImageScalerBase {
  public:
    virtual ~ImageScalerBase() = default;

    virtual const char *GetName() const = 0;
    virtual int GetFactor() const = 0;

    virtual Image Apply(const Image &src) const = 0;
};

class ScalerNone : public ImageScalerBase {
  public:
    const char *GetName() const override {
        return "NONE";
    }

    int GetFactor() const override {
        return 1;
    }

    Image Apply(const Image &src) const override {
        (void)src;
        return Image();
    }
};

class ScalerSxbr2 : public ImageScalerBase {
  public:
    const char *GetName() const override {
        return "SXBR2";
    }

    int GetFactor() const override {
        return 2;
    }

    Image Apply(const Image &src) const override {
        int w = src.GetWidth() * 2;
        int h = src.GetHeight() * 2;
        auto *out = new uint32_t[w * h];
        scaleSuperXBR2(src.GetData(), src.GetWidth(), src.GetHeight(), out);
        return Image::FromScaler(out, w, h);
    }
};

class ScalerSxbr3 : public ImageScalerBase {
  public:
    const char *GetName() const override {
        return "SXBR3";
    }

    int GetFactor() const override {
        return 3;
    }

    Image Apply(const Image &src) const override {
        int w = src.GetWidth() * 3;
        int h = src.GetHeight() * 3;
        auto *out = new uint32_t[w * h];
        scaleSuperXBR3(src.GetData(), src.GetWidth(), src.GetHeight(), out);
        return Image::FromScaler(out, w, h);
    }
};

class ScalerSxbr4 : public ImageScalerBase {
  public:
    const char *GetName() const override {
        return "SXBR4";
    }

    int GetFactor() const override {
        return 4;
    }

    Image Apply(const Image &src) const override {
        int w = src.GetWidth() * 4;
        int h = src.GetHeight() * 4;
        auto *out = new uint32_t[w * h];
        scaleSuperXBR4(src.GetData(), src.GetWidth(), src.GetHeight(), out);
        return Image::FromScaler(out, w, h);
    }
};

class ScalerMmpx2 : public ImageScalerBase {
  public:
    const char *GetName() const override {
        return "MMPX2";
    }

    int GetFactor() const override {
        return 2;
    }

    Image Apply(const Image &src) const override {
        int w = src.GetWidth() * 2;
        int h = src.GetHeight() * 2;
        auto *out = new uint32_t[w * h];
        mmpx_scale2x(src.GetData(), out, src.GetWidth(), src.GetHeight());
        return Image::FromScaler(out, w, h);
    }
};

class ScalerHq2xA : public ImageScalerBase {
  public:
    const char *GetName() const override {
        return "HQ2XA";
    }

    int GetFactor() const override {
        return 2;
    }

    Image Apply(const Image &src) const override {
        int w = src.GetWidth() * 2;
        int h = src.GetHeight() * 2;
        auto *out = new uint32_t[w * h];
        hq2xA(src.GetData(), src.GetWidth(), src.GetHeight(), out);
        return Image::FromScaler(out, w, h);
    }
};

class ScalerHq2xB : public ImageScalerBase {
  public:
    const char *GetName() const override {
        return "HQ2XB";
    }

    int GetFactor() const override {
        return 2;
    }

    Image Apply(const Image &src) const override {
        int w = src.GetWidth() * 2;
        int h = src.GetHeight() * 2;
        auto *out = new uint32_t[w * h];
        hq2xB(src.GetData(), src.GetWidth(), src.GetHeight(), out);
        return Image::FromScaler(out, w, h);
    }
};

class ScalerHq3xA : public ImageScalerBase {
  public:
    const char *GetName() const override {
        return "HQ3XA";
    }

    int GetFactor() const override {
        return 3;
    }

    Image Apply(const Image &src) const override {
        int w = src.GetWidth() * 3;
        int h = src.GetHeight() * 3;
        auto *out = new uint32_t[w * h];
        hq3xA(src.GetData(), src.GetWidth(), src.GetHeight(), out);
        return Image::FromScaler(out, w, h);
    }
};

class ScalerHq3xB : public ImageScalerBase {
  public:
    const char *GetName() const override {
        return "HQ3XB";
    }

    int GetFactor() const override {
        return 3;
    }

    Image Apply(const Image &src) const override {
        int w = src.GetWidth() * 3;
        int h = src.GetHeight() * 3;
        auto *out = new uint32_t[w * h];
        hq3xB(src.GetData(), src.GetWidth(), src.GetHeight(), out);
        return Image::FromScaler(out, w, h);
    }
};

struct ScalerEntry {
    const char *name;
    const ImageScalerBase *scaler;
};

static ScalerNone gScalerNone;
static ScalerSxbr2 gScalerSxbr2;
static ScalerSxbr3 gScalerSxbr3;
static ScalerSxbr4 gScalerSxbr4;
static ScalerMmpx2 gScalerMmpx2;
static ScalerHq2xA gScalerHq2xA;
static ScalerHq2xB gScalerHq2xB;
static ScalerHq3xA gScalerHq3xA;
static ScalerHq3xB gScalerHq3xB;

static const ScalerEntry gScalerRegistry[] = {{"NONE", &gScalerNone},   {"SXBR2", &gScalerSxbr2}, {"SXBR3", &gScalerSxbr3},
                                              {"SXBR4", &gScalerSxbr4}, {"MMPX2", &gScalerMmpx2}, {"HQ2XA", &gScalerHq2xA},
                                              {"HQ2XB", &gScalerHq2xB}, {"HQ3XA", &gScalerHq3xA}, {"HQ3XB", &gScalerHq3xB}};

static constexpr size_t ScalerCount = _countof(gScalerRegistry);

inline const ImageScalerBase *GetScalerByName(const char *name) {
    for (size_t i = 0; i < ScalerCount; ++i) {
        if (std::strcmp(gScalerRegistry[i].name, name) == 0)
            return gScalerRegistry[i].scaler;
    }
    return &gScalerNone;
}

inline Image DecodeInternal(const ImageSource &src, int scaleFactor) {
    int width = 0;
    int height = 0;

    if (src.GetType() == ImageSource::Type::File)
        image_log_info("Loading image from file %s", src.GetFile());
    else
        image_log_info("Loading image from memory");

    for (size_t i = 0; i < DecoderCount; ++i) {
        const ImageDecoder *dec = gDecoderRegistry[i];
        uint32_t *pixels = nullptr;

        if (src.GetType() == ImageSource::Type::File) {
            pixels = dec->LoadFromFile(src.GetFile(), &width, &height, scaleFactor);
        } else {
            pixels = dec->LoadFromMemory(src.GetData(), src.GetSize(), &width, &height, scaleFactor);
        }

        if (pixels) {
            image_log_trace("Image dimensions (%s) = (%i, %i)", dec->GetName(), width, height);
            bool isVector = dec->IsVectorGraphics();
            return Image::FromDecoder(pixels, width, height, dec->GetFreeFunction(), isVector);
        }
    }

    return Image();
}

inline Image Image::Load(const ImageSource &src, const ImageScalerBase *scaler) {
    const int factor = scaler ? scaler->GetFactor() : 1;

    Image base = DecodeInternal(src, factor);
    if (!base.IsValid())
        return base;

    if (base.IsVectorGraphics())
        return base;

    if (!scaler || scaler->GetFactor() == 1)
        return base;

    Image scaled = scaler->Apply(base);
    return scaled.IsValid() ? std::move(scaled) : std::move(base);
}
