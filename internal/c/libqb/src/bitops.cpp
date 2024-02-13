
#include "libqb-common.h"

#include <math.h>

#include "bitops.h"

// bit-array access functions (note: used to be included through 'bit.cpp')
uint64_t getubits(uint32_t bsize, uint8_t *base, intptr_t i) {
    int64_t bmask;
    bmask = ~(-(((int64_t)1) << bsize));
    i *= bsize;
    return ((*(uint64_t *)(base + (i >> 3))) >> (i & 7)) & bmask;
}

int64_t getbits(uint32_t bsize, uint8_t *base, intptr_t i) {
    int64_t bmask, bval64;
    bmask = ~(-(((int64_t)1) << bsize));
    i *= bsize;
    bval64 = ((*(uint64_t *)(base + (i >> 3))) >> (i & 7)) & bmask;
    if (bval64 & (((int64_t)1) << (bsize - 1)))
        return bval64 | (~bmask);
    return bval64;
}

void setbits(uint32_t bsize, uint8_t *base, intptr_t i, int64_t val) {
    int64_t bmask;
    uint64_t *bptr64;
    bmask = (((uint64_t)1) << bsize) - 1;
    i *= bsize;
    bptr64 = (uint64_t *)(base + (i >> 3));
    *bptr64 = (*bptr64 & (((bmask << (i & 7)) ^ -1))) | ((val & bmask) << (i & 7));
}
