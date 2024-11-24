#pragma once

#include "libqb-common.h"
#include <limits.h>
#include <stdint.h>

uint64_t getubits(uint32_t bsize, uint8_t *base, intptr_t i);
int64_t getbits(uint32_t bsize, uint8_t *base, intptr_t i);
void setbits(uint32_t bsize, uint8_t *base, intptr_t i, int64_t val);

template <typename T> static inline constexpr T func__rol(T value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(T) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

template <typename T> static inline constexpr T func__ror(T value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(T) - 1;
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
