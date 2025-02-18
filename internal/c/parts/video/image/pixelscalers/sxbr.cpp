/*****************************************************************************************************\
* This is a re-implementation of the XBRZ pixel scaler for QB64-PE in C.
*
* Bibliography:
* https://sourceforge.net/projects/xbrz/
* https://intrepidis.blogspot.com/2014/02/xbrz-in-java.html
* https://github.com/MiYanni/xBRZ.NET
* https://github.com/will-wyx/xbrz
* https://github.com/stanio/xbrz-java
* https://github.com/daelsepara/PixelFilterJS
* https://github.com/EasingSoft/UltraScaler
\*****************************************************************************************************/

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct ScalerCfg {
    double luminanceWeight;
    double equalColorTolerance;
    double centerDirectionBias;
    double dominantDirectionThreshold;
    double steepDirectionThreshold;
} ScalerCfg;

static const ScalerCfg default_scaler_cfg = {
    .luminanceWeight = 1.0, .equalColorTolerance = 30.0, .centerDirectionBias = 4.0, .dominantDirectionThreshold = 3.6, .steepDirectionThreshold = 2.2};

static inline uint8_t getAlpha(uint32_t pix) {
    return (uint8_t)(pix >> 24);
}

static inline uint8_t getRed(uint32_t pix) {
    return (uint8_t)((pix >> 16) & 0xFFu);
}

static inline uint8_t getGreen(uint32_t pix) {
    return (uint8_t)((pix >> 8) & 0xFFu);
}

static inline uint8_t getBlue(uint32_t pix) {
    return (uint8_t)(pix & 0xFFu);
}

static inline uint32_t makePixel(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t)(a << 24 | r << 16 | g << 8 | b);
}

static inline void fillBlock(uint32_t *trg, int pitch, uint32_t col, int blockWidth, int blockHeight) {
    int pitchWords = pitch >> 2;

    for (int y = 0; y < blockHeight; ++y, trg += pitchWords) {
        for (int x = 0; x < blockWidth; ++x) {
            trg[x] = col;
        }
    }
}

static inline void colorGradientARGB(unsigned int m, unsigned int n, uint32_t *pixBack, uint32_t pixFront) {
    unsigned int weightFront = getAlpha(pixFront) * m;
    unsigned int weightBack = getAlpha(*pixBack) * (n - m);
    unsigned int weightSum = weightFront + weightBack;

    if (weightSum == 0) {
        return;
    }

    uint8_t resultAlpha = (uint8_t)(weightSum / n);
    uint8_t resultRed = (uint8_t)((getRed(pixFront) * weightFront + getRed(*pixBack) * weightBack) / weightSum);
    uint8_t resultGreen = (uint8_t)((getGreen(pixFront) * weightFront + getGreen(*pixBack) * weightBack) / weightSum);
    uint8_t resultBlue = (uint8_t)((getBlue(pixFront) * weightFront + getBlue(*pixBack) * weightBack) / weightSum);

    *pixBack = makePixel(resultAlpha, resultRed, resultGreen, resultBlue);
}

typedef enum RotationDegree { ROT_0 = 0, ROT_90, ROT_180, ROT_270 } RotationDegree;

typedef struct OutputMatrix {
    uint32_t *out;
    int outWidth;
    RotationDegree rotDeg;
    int N;
} OutputMatrix;

static inline uint32_t *outputMatrixRef(OutputMatrix *matrix, int I, int J) {
    int N = matrix->N;
    int I_old, J_old;

    switch (matrix->rotDeg) {
    case ROT_90:
        I_old = N - 1 - J;
        J_old = I;
        break;

    case ROT_180:
        I_old = N - 1 - I;
        J_old = N - 1 - J;
        break;

    case ROT_270:
        I_old = J;
        J_old = N - 1 - I;
        break;

    case ROT_0:
    default:
        I_old = I;
        J_old = J;
    }

    return matrix->out + J_old + I_old * matrix->outWidth;
}

static inline double square(double value) {
    return value * value;
}

static inline double distYCbCr(uint32_t pix1, uint32_t pix2, double luminanceWeight) {
    int r_diff = (int)getRed(pix1) - (int)getRed(pix2);
    int g_diff = (int)getGreen(pix1) - (int)getGreen(pix2);
    int b_diff = (int)getBlue(pix1) - (int)getBlue(pix2);

    const double k_b = 0.0593;
    const double k_r = 0.2627;
    const double k_g = 1 - k_b - k_r;

    const double scale_b = 0.5 / (1.0 - k_b);
    const double scale_r = 0.5 / (1.0 - k_r);

    double y = k_r * r_diff + k_g * g_diff + k_b * b_diff;
    double c_b = scale_b * (b_diff - y);
    double c_r = scale_r * (r_diff - y);

    return sqrt(square(luminanceWeight * y) + square(c_b) + square(c_r));
}

static inline double colorDistanceARGB(uint32_t pix1, uint32_t pix2, double luminanceWeight) {
    double a1 = getAlpha(pix1) / 255.0;
    double a2 = getAlpha(pix2) / 255.0;
    double d = distYCbCr(pix1, pix2, luminanceWeight);

    if (a1 < a2) {
        return a1 * d + 255 * (a2 - a1);
    } else {
        return a2 * d + 255 * (a1 - a2);
    }
}

typedef enum BlendType { BLEND_NONE = 0, BLEND_NORMAL, BLEND_DOMINANT } BlendType;

typedef struct BlendResult {
    BlendType blend_f, blend_g, blend_j, blend_k;
} BlendResult;

typedef struct Kernel_3x3 {
    uint32_t a, b, c, d, e, f, g, h, i;
} Kernel_3x3;

typedef struct Kernel_4x4 {
    uint32_t a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p;
} Kernel_4x4;

static inline BlendResult preProcessCorners(const Kernel_4x4 *ker, const ScalerCfg *cfg) {
    BlendResult result = {BLEND_NONE, BLEND_NONE, BLEND_NONE, BLEND_NONE};

    if ((ker->f == ker->g && ker->j == ker->k) || (ker->f == ker->j && ker->g == ker->k)) {
        return result;
    }

    double jg = colorDistanceARGB(ker->i, ker->f, cfg->luminanceWeight) + colorDistanceARGB(ker->f, ker->c, cfg->luminanceWeight) +
                colorDistanceARGB(ker->n, ker->k, cfg->luminanceWeight) + colorDistanceARGB(ker->k, ker->h, cfg->luminanceWeight) +
                cfg->centerDirectionBias * colorDistanceARGB(ker->j, ker->g, cfg->luminanceWeight);

    double fk = colorDistanceARGB(ker->e, ker->j, cfg->luminanceWeight) + colorDistanceARGB(ker->j, ker->o, cfg->luminanceWeight) +
                colorDistanceARGB(ker->b, ker->g, cfg->luminanceWeight) + colorDistanceARGB(ker->g, ker->l, cfg->luminanceWeight) +
                cfg->centerDirectionBias * colorDistanceARGB(ker->f, ker->k, cfg->luminanceWeight);

    if (jg < fk) {
        bool dominantGradient = cfg->dominantDirectionThreshold * jg < fk;

        if (ker->f != ker->g && ker->f != ker->j) {
            result.blend_f = dominantGradient ? BLEND_DOMINANT : BLEND_NORMAL;
        }

        if (ker->k != ker->j && ker->k != ker->g) {
            result.blend_k = dominantGradient ? BLEND_DOMINANT : BLEND_NORMAL;
        }
    } else if (fk < jg) {
        bool dominantGradient = cfg->dominantDirectionThreshold * fk < jg;

        if (ker->j != ker->f && ker->j != ker->k) {
            result.blend_j = dominantGradient ? BLEND_DOMINANT : BLEND_NORMAL;
        }

        if (ker->g != ker->f && ker->g != ker->k) {
            result.blend_g = dominantGradient ? BLEND_DOMINANT : BLEND_NORMAL;
        }
    }

    return result;
}

static inline uint32_t get_b(RotationDegree rotDeg, const Kernel_3x3 *ker) {
    switch (rotDeg) {
    case ROT_90:
        return ker->d;
    case ROT_180:
        return ker->h;
    case ROT_270:
        return ker->f;
    case ROT_0:
    default:
        return ker->b;
    }
}

static inline uint32_t get_c(RotationDegree rotDeg, const Kernel_3x3 *ker) {
    switch (rotDeg) {
    case ROT_90:
        return ker->a;
    case ROT_180:
        return ker->g;
    case ROT_270:
        return ker->i;
    case ROT_0:
    default:
        return ker->c;
    }
}

static inline uint32_t get_d(RotationDegree rotDeg, const Kernel_3x3 *ker) {
    switch (rotDeg) {
    case ROT_90:
        return ker->h;
    case ROT_180:
        return ker->f;
    case ROT_270:
        return ker->b;
    case ROT_0:
    default:
        return ker->d;
    }
}

static inline uint32_t get_e(RotationDegree rotDeg, const Kernel_3x3 *ker) {
    (void)rotDeg;
    return ker->e;
}

static inline uint32_t get_f(RotationDegree rotDeg, const Kernel_3x3 *ker) {
    switch (rotDeg) {
    case ROT_90:
        return ker->b;
    case ROT_180:
        return ker->d;
    case ROT_270:
        return ker->h;
    case ROT_0:
    default:
        return ker->f;
    }
}

static inline uint32_t get_g(RotationDegree rotDeg, const Kernel_3x3 *ker) {
    switch (rotDeg) {
    case ROT_90:
        return ker->i;
    case ROT_180:
        return ker->c;
    case ROT_270:
        return ker->a;
    case ROT_0:
    default:
        return ker->g;
    }
}

static inline uint32_t get_h(RotationDegree rotDeg, const Kernel_3x3 *ker) {
    switch (rotDeg) {
    case ROT_90:
        return ker->f;
    case ROT_180:
        return ker->b;
    case ROT_270:
        return ker->d;
    case ROT_0:
    default:
        return ker->h;
    }
}

static inline uint32_t get_i(RotationDegree rotDeg, const Kernel_3x3 *ker) {
    switch (rotDeg) {
    case ROT_90:
        return ker->c;
    case ROT_180:
        return ker->a;
    case ROT_270:
        return ker->g;
    case ROT_0:
    default:
        return ker->i;
    }
}

static inline BlendType getTopR(uint8_t b) {
    return (BlendType)(0x3 & (b >> 2));
}

static inline BlendType getBottomR(uint8_t b) {
    return (BlendType)(0x3 & (b >> 4));
}

static inline BlendType getBottomL(uint8_t b) {
    return (BlendType)(0x3 & (b >> 6));
}

static inline void clearAddTopL(uint8_t *b, BlendType bt) {
    *b = (uint8_t)bt;
}

static inline void addTopR(uint8_t *b, BlendType bt) {
    *b |= (bt << 2);
}

static inline void addBottomR(uint8_t *b, BlendType bt) {
    *b |= (bt << 4);
}

static inline void addBottomL(uint8_t *b, BlendType bt) {
    *b |= (bt << 6);
}

static inline bool blendingNeeded(uint8_t b) {
    return b != 0;
}

static inline uint8_t rotateBlendInfo(RotationDegree rotDeg, uint8_t b) {
    switch (rotDeg) {
    case ROT_90:
        return ((b << 2) | (b >> 6)) & 0xff;
    case ROT_180:
        return ((b << 4) | (b >> 4)) & 0xff;
    case ROT_270:
        return ((b << 6) | (b >> 2)) & 0xff;
    case ROT_0:
    default:
        return b;
    }
}

typedef struct ScalerTypeVTable {
    int scale;
    void (*blendLineShallow)(uint32_t, OutputMatrix *);
    void (*blendLineSteep)(uint32_t, OutputMatrix *);
    void (*blendLineSteepAndShallow)(uint32_t, OutputMatrix *);
    void (*blendLineDiagonal)(uint32_t, OutputMatrix *);
    void (*blendCorner)(uint32_t, OutputMatrix *);
} ScalerTypeVTable;

static inline void scaler2x_blendLineShallow(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 4, outputMatrixRef(out, 2 - 1, 0), col);
    colorGradientARGB(3, 4, outputMatrixRef(out, 2 - 1, 1), col);
}

static inline void scaler2x_blendLineSteep(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 4, outputMatrixRef(out, 0, 2 - 1), col);
    colorGradientARGB(3, 4, outputMatrixRef(out, 1, 2 - 1), col);
}

static inline void scaler2x_blendLineSteepAndShallow(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 4, outputMatrixRef(out, 1, 0), col);
    colorGradientARGB(1, 4, outputMatrixRef(out, 0, 1), col);
    colorGradientARGB(5, 6, outputMatrixRef(out, 1, 1), col);
}

static inline void scaler2x_blendLineDiagonal(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 2, outputMatrixRef(out, 1, 1), col);
}

static inline void scaler2x_blendCorner(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(21, 100, outputMatrixRef(out, 1, 1), col);
}

static ScalerTypeVTable scaler2x_vtable = {.scale = 2,
                                           .blendLineShallow = scaler2x_blendLineShallow,
                                           .blendLineSteep = scaler2x_blendLineSteep,
                                           .blendLineSteepAndShallow = scaler2x_blendLineSteepAndShallow,
                                           .blendLineDiagonal = scaler2x_blendLineDiagonal,
                                           .blendCorner = scaler2x_blendCorner};

static inline void scaler3x_blendLineShallow(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 4, outputMatrixRef(out, 3 - 1, 0), col);
    colorGradientARGB(1, 4, outputMatrixRef(out, 3 - 2, 2), col);

    colorGradientARGB(3, 4, outputMatrixRef(out, 3 - 1, 1), col);
    *outputMatrixRef(out, 3 - 1, 2) = col;
}

static inline void scaler3x_blendLineSteep(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 4, outputMatrixRef(out, 0, 3 - 1), col);
    colorGradientARGB(1, 4, outputMatrixRef(out, 2, 3 - 2), col);

    colorGradientARGB(3, 4, outputMatrixRef(out, 1, 3 - 1), col);
    *outputMatrixRef(out, 2, 3 - 1) = col;
}

static inline void scaler3x_blendLineSteepAndShallow(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 4, outputMatrixRef(out, 2, 0), col);
    colorGradientARGB(1, 4, outputMatrixRef(out, 0, 2), col);
    colorGradientARGB(3, 4, outputMatrixRef(out, 2, 1), col);
    colorGradientARGB(3, 4, outputMatrixRef(out, 1, 2), col);
    *outputMatrixRef(out, 2, 2) = col;
}

static inline void scaler3x_blendLineDiagonal(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 8, outputMatrixRef(out, 1, 2), col);
    colorGradientARGB(1, 8, outputMatrixRef(out, 2, 1), col);
    colorGradientARGB(7, 8, outputMatrixRef(out, 2, 2), col);
}

static inline void scaler3x_blendCorner(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(45, 100, outputMatrixRef(out, 2, 2), col);
}

static ScalerTypeVTable scaler3x_vtable = {.scale = 3,
                                           .blendLineShallow = scaler3x_blendLineShallow,
                                           .blendLineSteep = scaler3x_blendLineSteep,
                                           .blendLineSteepAndShallow = scaler3x_blendLineSteepAndShallow,
                                           .blendLineDiagonal = scaler3x_blendLineDiagonal,
                                           .blendCorner = scaler3x_blendCorner};

static inline void scaler4x_blendLineShallow(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 4, outputMatrixRef(out, 4 - 1, 0), col);
    colorGradientARGB(1, 4, outputMatrixRef(out, 4 - 2, 2), col);

    colorGradientARGB(3, 4, outputMatrixRef(out, 4 - 1, 1), col);
    colorGradientARGB(3, 4, outputMatrixRef(out, 4 - 2, 3), col);

    *outputMatrixRef(out, 4 - 1, 2) = col;
    *outputMatrixRef(out, 4 - 1, 3) = col;
}

static inline void scaler4x_blendLineSteep(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 4, outputMatrixRef(out, 0, 4 - 1), col);
    colorGradientARGB(1, 4, outputMatrixRef(out, 2, 4 - 2), col);

    colorGradientARGB(3, 4, outputMatrixRef(out, 1, 4 - 1), col);
    colorGradientARGB(3, 4, outputMatrixRef(out, 3, 4 - 2), col);

    *outputMatrixRef(out, 2, 4 - 1) = col;
    *outputMatrixRef(out, 3, 4 - 1) = col;
}

static inline void scaler4x_blendLineSteepAndShallow(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(3, 4, outputMatrixRef(out, 3, 1), col);
    colorGradientARGB(3, 4, outputMatrixRef(out, 1, 3), col);
    colorGradientARGB(1, 4, outputMatrixRef(out, 3, 0), col);
    colorGradientARGB(1, 4, outputMatrixRef(out, 0, 3), col);

    colorGradientARGB(1, 3, outputMatrixRef(out, 2, 2), col);

    *outputMatrixRef(out, 3, 3) = col;
    *outputMatrixRef(out, 3, 2) = col;
    *outputMatrixRef(out, 2, 3) = col;
}

static inline void scaler4x_blendLineDiagonal(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(1, 2, outputMatrixRef(out, 4 - 1, 4 / 2), col);
    colorGradientARGB(1, 2, outputMatrixRef(out, 4 - 2, 4 / 2 + 1), col);
    *outputMatrixRef(out, 4 - 1, 4 - 1) = col;
}

static inline void scaler4x_blendCorner(uint32_t col, OutputMatrix *out) {
    colorGradientARGB(68, 100, outputMatrixRef(out, 3, 3), col);
    colorGradientARGB(9, 100, outputMatrixRef(out, 3, 2), col);
    colorGradientARGB(9, 100, outputMatrixRef(out, 2, 3), col);
}

static ScalerTypeVTable scaler4x_vtable = {.scale = 4,
                                           .blendLineShallow = scaler4x_blendLineShallow,
                                           .blendLineSteep = scaler4x_blendLineSteep,
                                           .blendLineSteepAndShallow = scaler4x_blendLineSteepAndShallow,
                                           .blendLineDiagonal = scaler4x_blendLineDiagonal,
                                           .blendCorner = scaler4x_blendCorner};

typedef struct OobReaderTransparent {
    const uint32_t *s_m1;
    const uint32_t *s_0;
    const uint32_t *s_p1;
    const uint32_t *s_p2;
    int s_width;
} OobReaderTransparent;

static inline OobReaderTransparent oobReaderTransparentCreate(const uint32_t *src, int srcWidth, int srcHeight, int y) {
    return (OobReaderTransparent){.s_m1 = (0 <= y - 1 && y - 1 < srcHeight) ? src + srcWidth * (y - 1) : NULL,
                                  .s_0 = (0 <= y && y < srcHeight) ? src + srcWidth * y : NULL,
                                  .s_p1 = (0 <= y + 1 && y + 1 < srcHeight) ? src + srcWidth * (y + 1) : NULL,
                                  .s_p2 = (0 <= y + 2 && y + 2 < srcHeight) ? src + srcWidth * (y + 2) : NULL,
                                  .s_width = srcWidth};
}

static inline void oobReaderTransparentReadDhlp(const OobReaderTransparent *reader, Kernel_4x4 *ker, int x) {
    if (0 <= x + 2 && x + 2 < reader->s_width) {
        ker->d = reader->s_m1 ? reader->s_m1[x + 2] : 0;
        ker->h = reader->s_0 ? reader->s_0[x + 2] : 0;
        ker->l = reader->s_p1 ? reader->s_p1[x + 2] : 0;
        ker->p = reader->s_p2 ? reader->s_p2[x + 2] : 0;
    } else {
        ker->d = 0;
        ker->h = 0;
        ker->l = 0;
        ker->p = 0;
    }
}

static inline bool pixelEqual(uint32_t pix1, uint32_t pix2, const ScalerCfg *cfg) {
    return colorDistanceARGB(pix1, pix2, cfg->luminanceWeight) < cfg->equalColorTolerance;
}

static inline bool doLineBlend(uint8_t blend, uint32_t e, uint32_t g, uint32_t c, uint32_t i, uint32_t h, uint32_t f, const ScalerCfg *cfg) {
    if (getBottomR(blend) >= BLEND_DOMINANT) {
        return true;
    }

    if (getTopR(blend) != BLEND_NONE && !pixelEqual(e, g, cfg)) {
        return false;
    }

    if (getBottomL(blend) != BLEND_NONE && !pixelEqual(e, c, cfg)) {
        return false;
    }

    if (!pixelEqual(e, i, cfg) && pixelEqual(g, h, cfg) && pixelEqual(h, i, cfg) && pixelEqual(i, f, cfg) && pixelEqual(f, c, cfg)) {
        return false;
    }

    return true;
}

static inline void blendPixel(ScalerTypeVTable *scaler, RotationDegree rotDeg, const Kernel_3x3 *ker, uint32_t *target, int trgWidth, uint8_t blendInfo,
                              const ScalerCfg *cfg) {
    uint32_t b = get_b(rotDeg, ker);
    uint32_t c = get_c(rotDeg, ker);
    uint32_t d = get_d(rotDeg, ker);
    uint32_t e = get_e(rotDeg, ker);
    uint32_t f = get_f(rotDeg, ker);
    uint32_t g = get_g(rotDeg, ker);
    uint32_t h = get_h(rotDeg, ker);
    uint32_t i = get_i(rotDeg, ker);

    uint8_t blend = rotateBlendInfo(rotDeg, blendInfo);

    if (getBottomR(blend) >= BLEND_NORMAL) {
        bool doBlend = doLineBlend(blend, e, g, c, i, h, f, cfg);

        uint32_t px = colorDistanceARGB(e, f, cfg->luminanceWeight) <= colorDistanceARGB(e, h, cfg->luminanceWeight) ? f : h;

        OutputMatrix out_matrix = {target, trgWidth, rotDeg, scaler->scale};

        if (doBlend) {
            double fg = colorDistanceARGB(f, g, cfg->luminanceWeight);
            double hc = colorDistanceARGB(h, c, cfg->luminanceWeight);

            bool haveShallowLine = cfg->steepDirectionThreshold * fg <= hc && e != g && d != g;
            bool haveSteepLine = cfg->steepDirectionThreshold * hc <= fg && e != c && b != c;

            if (haveShallowLine) {
                if (haveSteepLine) {
                    scaler->blendLineSteepAndShallow(px, &out_matrix);
                } else {
                    scaler->blendLineShallow(px, &out_matrix);
                }
            } else {
                if (haveSteepLine) {
                    scaler->blendLineSteep(px, &out_matrix);
                } else {
                    scaler->blendLineDiagonal(px, &out_matrix);
                }
            }
        } else {
            scaler->blendCorner(px, &out_matrix);
        }
    }
}

static void scaleImage(ScalerTypeVTable *scaler, uint32_t *src, uint32_t *trg, int srcWidth, int srcHeight, const ScalerCfg *cfg, int yFirst, int yLast) {
    yFirst = (yFirst > 0) ? yFirst : 0;
    yLast = (yLast < srcHeight) ? yLast : srcHeight;

    if (yFirst >= yLast || srcWidth <= 0) {
        return;
    }

    int trgWidth = srcWidth * scaler->scale;
    uint8_t *preProcBuf = (uint8_t *)(trg + yLast * scaler->scale * trgWidth) - srcWidth;

    {
        OobReaderTransparent oobReader_y_m1 = oobReaderTransparentCreate(src, srcWidth, srcHeight, yFirst - 1);

        Kernel_4x4 ker4 = {0};
        oobReaderTransparentReadDhlp(&oobReader_y_m1, &ker4, -4);
        ker4.a = ker4.d;
        ker4.e = ker4.h;
        ker4.i = ker4.l;
        ker4.m = ker4.p;

        oobReaderTransparentReadDhlp(&oobReader_y_m1, &ker4, -3);
        ker4.b = ker4.d;
        ker4.f = ker4.h;
        ker4.j = ker4.l;
        ker4.n = ker4.p;

        oobReaderTransparentReadDhlp(&oobReader_y_m1, &ker4, -2);
        ker4.c = ker4.d;
        ker4.g = ker4.h;
        ker4.k = ker4.l;
        ker4.o = ker4.p;

        oobReaderTransparentReadDhlp(&oobReader_y_m1, &ker4, -1);

        {
            BlendResult res = preProcessCorners(&ker4, cfg);
            clearAddTopL(&preProcBuf[0], res.blend_k);
        }

        for (int x = 0; x < srcWidth; ++x) {
            ker4.a = ker4.b;
            ker4.e = ker4.f;
            ker4.i = ker4.j;
            ker4.m = ker4.n;
            ker4.b = ker4.c;
            ker4.f = ker4.g;
            ker4.j = ker4.k;
            ker4.n = ker4.o;
            ker4.c = ker4.d;
            ker4.g = ker4.h;
            ker4.k = ker4.l;
            ker4.o = ker4.p;

            oobReaderTransparentReadDhlp(&oobReader_y_m1, &ker4, x);

            BlendResult res = preProcessCorners(&ker4, cfg);
            addTopR(&preProcBuf[x], res.blend_j);

            if (x + 1 < srcWidth) {
                clearAddTopL(&preProcBuf[x + 1], res.blend_k);
            }
        }
    }

    for (int y = yFirst; y < yLast; ++y) {
        uint32_t *out = trg + scaler->scale * y * trgWidth;

        OobReaderTransparent oobReader_y = oobReaderTransparentCreate(src, srcWidth, srcHeight, y);

        Kernel_4x4 ker4 = {0};
        oobReaderTransparentReadDhlp(&oobReader_y, &ker4, -4);
        ker4.a = ker4.d;
        ker4.e = ker4.h;
        ker4.i = ker4.l;
        ker4.m = ker4.p;

        oobReaderTransparentReadDhlp(&oobReader_y, &ker4, -3);
        ker4.b = ker4.d;
        ker4.f = ker4.h;
        ker4.j = ker4.l;
        ker4.n = ker4.p;

        oobReaderTransparentReadDhlp(&oobReader_y, &ker4, -2);
        ker4.c = ker4.d;
        ker4.g = ker4.h;
        ker4.k = ker4.l;
        ker4.o = ker4.p;

        oobReaderTransparentReadDhlp(&oobReader_y, &ker4, -1);

        uint8_t blend_xy1 = 0;
        {
            BlendResult res = preProcessCorners(&ker4, cfg);
            clearAddTopL(&blend_xy1, res.blend_k);
            addBottomL(&preProcBuf[0], res.blend_g);
        }

        for (int x = 0; x < srcWidth; ++x, out += scaler->scale) {
            ker4.a = ker4.b;
            ker4.e = ker4.f;
            ker4.i = ker4.j;
            ker4.m = ker4.n;
            ker4.b = ker4.c;
            ker4.f = ker4.g;
            ker4.j = ker4.k;
            ker4.n = ker4.o;
            ker4.c = ker4.d;
            ker4.g = ker4.h;
            ker4.k = ker4.l;
            ker4.o = ker4.p;

            oobReaderTransparentReadDhlp(&oobReader_y, &ker4, x);

            uint8_t blend_xy = preProcBuf[x];
            {
                BlendResult res = preProcessCorners(&ker4, cfg);
                addBottomR(&blend_xy, res.blend_f);
                addTopR(&blend_xy1, res.blend_j);
                preProcBuf[x] = blend_xy1;

                if (x + 1 < srcWidth) {
                    clearAddTopL(&blend_xy1, res.blend_k);
                    addBottomL(&preProcBuf[x + 1], res.blend_g);
                }
            }

            fillBlock(out, trgWidth * sizeof(uint32_t), ker4.f, scaler->scale, scaler->scale);

            Kernel_3x3 ker3 = {ker4.a, ker4.b, ker4.c, ker4.e, ker4.f, ker4.g, ker4.i, ker4.j, ker4.k};

            if (blendingNeeded(blend_xy)) {
                blendPixel(scaler, ROT_0, &ker3, out, trgWidth, blend_xy, cfg);
                blendPixel(scaler, ROT_90, &ker3, out, trgWidth, blend_xy, cfg);
                blendPixel(scaler, ROT_180, &ker3, out, trgWidth, blend_xy, cfg);
                blendPixel(scaler, ROT_270, &ker3, out, trgWidth, blend_xy, cfg);
            }
        }
    }
}

void scaleSuperXBR2(uint32_t *data, int w, int h, uint32_t *out) {
    scaleImage(&scaler2x_vtable, data, out, w, h, &default_scaler_cfg, 0, INT_MAX);
}

void scaleSuperXBR3(uint32_t *data, int w, int h, uint32_t *out) {
    scaleImage(&scaler3x_vtable, data, out, w, h, &default_scaler_cfg, 0, INT_MAX);
}

void scaleSuperXBR4(uint32_t *data, int w, int h, uint32_t *out) {
    scaleImage(&scaler4x_vtable, data, out, w, h, &default_scaler_cfg, 0, INT_MAX);
}
