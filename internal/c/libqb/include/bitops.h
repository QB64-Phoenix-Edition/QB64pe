#pragma once

#include <cstdint>
#include <limits>

// bit-array access functions (note: used to be included through 'bit.cpp')
static inline constexpr uint64_t getubits(uint32_t bsize, uint8_t *base, intptr_t i) {
    int64_t bmask;
    bmask = ~(-(((int64_t)1) << bsize));
    i *= bsize;
    return ((*(uint64_t *)(base + (i >> 3))) >> (i & 7)) & bmask;
}

static inline constexpr int64_t getbits(uint32_t bsize, uint8_t *base, intptr_t i) {
    int64_t bmask, bval64;
    bmask = ~(-(((int64_t)1) << bsize));
    i *= bsize;
    bval64 = ((*(uint64_t *)(base + (i >> 3))) >> (i & 7)) & bmask;
    if (bval64 & (((int64_t)1) << (bsize - 1)))
        return bval64 | (~bmask);
    return bval64;
}

static inline void setbits(uint32_t bsize, uint8_t *base, intptr_t i, int64_t val) {
    int64_t bmask;
    uint64_t *bptr64;
    bmask = (((uint64_t)1) << bsize) - 1;
    i *= bsize;
    bptr64 = (uint64_t *)(base + (i >> 3));
    *bptr64 = (*bptr64 & (((bmask << (i & 7)) ^ -1))) | ((val & bmask) << (i & 7));
}

template <typename T> static inline constexpr T func__rol(T value, unsigned int count) {
    const unsigned int mask = std::numeric_limits<uint8_t>::digits * sizeof(T) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

template <typename T> static inline constexpr T func__ror(T value, unsigned int count) {
    const unsigned int mask = std::numeric_limits<uint8_t>::digits * sizeof(T) - 1;
    count &= mask;
    return (value >> count) | (value << (-count & mask));
}

static inline constexpr uint64_t func__shl(uint64_t a1, int b1) {
    return a1 << b1;
}

static inline constexpr uint64_t func__shr(uint64_t a1, int b1) {
    return a1 >> b1;
}

static inline constexpr int64_t func__readbit(uint64_t a1, int b1) {
    return (a1 & 1ull << b1) ? QB_TRUE : QB_FALSE;
}

static inline constexpr uint64_t func__setbit(uint64_t a1, int b1) {
    return a1 | 1ull << b1;
}

static inline constexpr uint64_t func__resetbit(uint64_t a1, int b1) {
    return a1 & ~(1ull << b1);
}

static inline constexpr uint64_t func__togglebit(uint64_t a1, int b1) {
    return a1 ^ 1ull << b1;
}
