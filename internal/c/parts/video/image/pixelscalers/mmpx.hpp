/*
    Copyright 2020 Morgan McGuire & Mara Gagiu.
    Available under the MIT license.
    https://casual-effects.com/research/McGuire2021PixelArt/index.html
*/

#ifndef __MMPX_H__
#define __MMPX_H__

#include <cstdint>

void mmpx_scale2x(const uint32_t *srcBuffer, uint32_t *dst, uint32_t srcWidth, uint32_t srcHeight);

#endif

#ifdef MMPX_IMPLEMENTATION

#include <cstdbool>

static inline uint32_t luma(uint32_t color) {
    const uint32_t alpha = (color & 0xFF000000) >> 24;
    return (((color & 0x00FF0000) >> 16) + ((color & 0x0000FF00) >> 8) + (color & 0x000000FF) + 1) * (256 - alpha);
}

static inline bool all_eq2(uint32_t B, uint32_t A0, uint32_t A1) { return ((B ^ A0) | (B ^ A1)) == 0; }

static inline bool all_eq3(uint32_t B, uint32_t A0, uint32_t A1, uint32_t A2) { return ((B ^ A0) | (B ^ A1) | (B ^ A2)) == 0; }

static inline bool all_eq4(uint32_t B, uint32_t A0, uint32_t A1, uint32_t A2, uint32_t A3) { return ((B ^ A0) | (B ^ A1) | (B ^ A2) | (B ^ A3)) == 0; }

static inline bool any_eq3(uint32_t B, uint32_t A0, uint32_t A1, uint32_t A2) { return B == A0 || B == A1 || B == A2; }

static inline bool none_eq2(uint32_t B, uint32_t A0, uint32_t A1) { return (B != A0) && (B != A1); }

static inline bool none_eq4(uint32_t B, uint32_t A0, uint32_t A1, uint32_t A2, uint32_t A3) { return B != A0 && B != A1 && B != A2 && B != A3; }

static inline int mmpx_clamp(int v, int min, int max) { return v < min ? min : min > max ? max : v; }

struct Meta {
    const uint32_t *srcBuffer;
    uint32_t srcWidth;
    uint32_t srcHeight;
    uint32_t srcMaxX;
    uint32_t srcMaxY;
};

static inline uint32_t src(const struct Meta *meta, int x, int y) {
    // Clamp to border
    if ((uint32_t)x > (uint32_t)meta->srcMaxX || (uint32_t)y > (uint32_t)meta->srcMaxY) {
        x = mmpx_clamp(x, 0, meta->srcMaxX);
        y = mmpx_clamp(y, 0, meta->srcMaxY);
    }

    return meta->srcBuffer[y * meta->srcWidth + x];
}

static struct Meta build_meta(const uint32_t *srcBuffer, uint32_t srcWidth, uint32_t srcHeight) {
    return (struct Meta){
        .srcBuffer = srcBuffer,
        .srcWidth = srcWidth,
        .srcHeight = srcHeight,
        .srcMaxX = srcWidth - 1,
        .srcMaxY = srcHeight - 1,
    };
}

void mmpx_scale2x(const uint32_t *srcBuffer, uint32_t *dst, uint32_t srcWidth, uint32_t srcHeight) {
    const struct Meta meta = build_meta(srcBuffer, srcWidth, srcHeight);

    for (uint32_t srcY = 0; srcY < srcHeight; ++srcY) {
        uint32_t srcX = 0;

        // Inputs carried along rows
        uint32_t A = src(&meta, srcX - 1, srcY - 1);
        uint32_t B = src(&meta, srcX, srcY - 1);
        uint32_t C = src(&meta, srcX + 1, srcY - 1);

        uint32_t D = src(&meta, srcX - 1, srcY);
        uint32_t E = src(&meta, srcX, srcY);
        uint32_t F = src(&meta, srcX + 1, srcY);

        uint32_t G = src(&meta, srcX - 1, srcY + 1);
        uint32_t H = src(&meta, srcX, srcY + 1);
        uint32_t I = src(&meta, srcX + 1, srcY + 1);

        uint32_t Q = src(&meta, srcX - 2, srcY);
        uint32_t R = src(&meta, srcX + 2, srcY);

        for (srcX = 0; srcX < srcWidth; ++srcX) {
            // Outputs
            uint32_t J = E, K = E, L = E, M = E;

            if (((A ^ E) | (B ^ E) | (C ^ E) | (D ^ E) | (F ^ E) | (G ^ E) | (H ^ E) | (I ^ E)) != 0) {
                const uint32_t P = src(&meta, srcX, srcY - 2), S = src(&meta, srcX, srcY + 2);
                const uint32_t Bl = luma(B), Dl = luma(D), El = luma(E), Fl = luma(F), Hl = luma(H);

                // 1:1 slope rules
                {
                    if ((D == B && D != H && D != F) && (El >= Dl || E == A) && any_eq3(E, A, C, G) && ((El < Dl) || A != D || E != P || E != Q))
                        J = D;
                    if ((B == F && B != D && B != H) && (El >= Bl || E == C) && any_eq3(E, A, C, I) && ((El < Bl) || C != B || E != P || E != R))
                        K = B;
                    if ((H == D && H != F && H != B) && (El >= Hl || E == G) && any_eq3(E, A, G, I) && ((El < Hl) || G != H || E != S || E != Q))
                        L = H;
                    if ((F == H && F != B && F != D) && (El >= Fl || E == I) && any_eq3(E, C, G, I) && ((El < Fl) || I != H || E != R || E != S))
                        M = F;
                }

                // Intersection rules
                {
                    if ((E != F && all_eq4(E, C, I, D, Q) && all_eq2(F, B, H)) && (F != src(&meta, srcX + 3, srcY)))
                        K = M = F;
                    if ((E != D && all_eq4(E, A, G, F, R) && all_eq2(D, B, H)) && (D != src(&meta, srcX - 3, srcY)))
                        J = L = D;
                    if ((E != H && all_eq4(E, G, I, B, P) && all_eq2(H, D, F)) && (H != src(&meta, srcX, srcY + 3)))
                        L = M = H;
                    if ((E != B && all_eq4(E, A, C, H, S) && all_eq2(B, D, F)) && (B != src(&meta, srcX, srcY - 3)))
                        J = K = B;
                    if (Bl < El && all_eq4(E, G, H, I, S) && none_eq4(E, A, D, C, F))
                        J = K = B;
                    if (Hl < El && all_eq4(E, A, B, C, P) && none_eq4(E, D, G, I, F))
                        L = M = H;
                    if (Fl < El && all_eq4(E, A, D, G, Q) && none_eq4(E, B, C, I, H))
                        K = M = F;
                    if (Dl < El && all_eq4(E, C, F, I, R) && none_eq4(E, B, A, G, H))
                        J = L = D;
                }

                // 2:1 slope rules
                {
                    if (H != B) {
                        if (H != A && H != E && H != C) {
                            if (all_eq3(H, G, F, R) && none_eq2(H, D, src(&meta, srcX + 2, srcY - 1)))
                                L = M;
                            if (all_eq3(H, I, D, Q) && none_eq2(H, F, src(&meta, srcX - 2, srcY - 1)))
                                M = L;
                        }

                        if (B != I && B != G && B != E) {
                            if (all_eq3(B, A, F, R) && none_eq2(B, D, src(&meta, srcX + 2, srcY + 1)))
                                J = K;
                            if (all_eq3(B, C, D, Q) && none_eq2(B, F, src(&meta, srcX - 2, srcY + 1)))
                                K = J;
                        }
                    } // H !== B

                    if (F != D) {
                        if (D != I && D != E && D != C) {
                            if (all_eq3(D, A, H, S) && none_eq2(D, B, src(&meta, srcX + 1, srcY + 2)))
                                J = L;
                            if (all_eq3(D, G, B, P) && none_eq2(D, H, src(&meta, srcX + 1, srcY - 2)))
                                L = J;
                        }

                        if (F != E && F != A && F != G) {
                            if (all_eq3(F, C, H, S) && none_eq2(F, B, src(&meta, srcX - 1, srcY + 2)))
                                K = M;
                            if (all_eq3(F, I, B, P) && none_eq2(F, H, src(&meta, srcX - 1, srcY - 2)))
                                M = K;
                        }
                    } // F !== D
                }     // 2:1 slope
            }

            const uint32_t dstIndex = ((srcX + srcX) + (srcY << 2) * srcWidth) >> 0;
            uint32_t *dstPacked = dst + dstIndex;

            *dstPacked = J;
            dstPacked++;
            *dstPacked = K;
            dstPacked += srcWidth + meta.srcMaxX;
            *dstPacked = L;
            dstPacked++;
            *dstPacked = M;

            A = B;
            B = C;
            C = src(&meta, srcX + 2, srcY - 1);
            Q = D;
            D = E;
            E = F;
            F = R;
            R = src(&meta, srcX + 3, srcY);
            G = H;
            H = I;
            I = src(&meta, srcX + 2, srcY + 1);
        } // X
    }     // Y
}

#endif
