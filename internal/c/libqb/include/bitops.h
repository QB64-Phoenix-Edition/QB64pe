#pragma once

#include "libqb-common.h"
#include <limits.h>
#include <stdint.h>

uint64_t getubits(uint32_t bsize, uint8_t *base, intptr_t i);
int64_t getbits(uint32_t bsize, uint8_t *base, intptr_t i);
void setbits(uint32_t bsize, uint8_t *base, intptr_t i, int64_t val);

// The rotation functions below are the way they are for a couple of reasons:
//  1. They are safer (well folks seem to think so; see https://en.wikipedia.org/wiki/Circular_shift#Implementing_circular_shifts)
//  2. We are using C library constants and there is just 1 numeric literal - '1'
//  3. GGC recognizes the 'pattern' and will optimize it out to 'roX' and 3 more instructions when using O2
static inline constexpr uint8_t func__rol8(uint8_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

static inline constexpr uint8_t func__ror8(uint8_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value >> count) | (value << (-count & mask));
}

static inline constexpr uint16_t func__rol16(uint16_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

static inline constexpr uint16_t func__ror16(uint16_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value >> count) | (value << (-count & mask));
}

static inline constexpr uint32_t func__rol32(uint32_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

static inline constexpr uint32_t func__ror32(uint32_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value >> count) | (value << (-count & mask));
}

static inline constexpr uint64_t func__rol64(uint64_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

static inline constexpr uint64_t func__ror64(uint64_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value >> count) | (value << (-count & mask));
}

// bit-shifting
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
