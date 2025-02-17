#pragma once

#include <cstdint>

void hq2xA(uint32_t *img, int w, int h, uint32_t *out);
void hq2xB(uint32_t *img, int w, int h, uint32_t *out);
void hq3xA(uint32_t *img, int w, int h, uint32_t *out);
void hq3xB(uint32_t *img, int w, int h, uint32_t *out);
void mmpx_scale2x(const uint32_t *srcBuffer, uint32_t *dst, uint32_t srcWidth, uint32_t srcHeight);
void scaleSuperXBR2(uint32_t *data, int w, int h, uint32_t *out);
void scaleSuperXBR3(uint32_t *data, int w, int h, uint32_t *out);
void scaleSuperXBR4(uint32_t *data, int w, int h, uint32_t *out);
