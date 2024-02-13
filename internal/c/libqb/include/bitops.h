#pragma once

#include <limits.h>
#include <stdint.h>

uint64_t getubits(uint32_t bsize, uint8_t *base, intptr_t i);
int64_t getbits(uint32_t bsize, uint8_t *base, intptr_t i);
void setbits(uint32_t bsize, uint8_t *base, intptr_t i, int64_t val);

// a740g: ROR & ROL additions start
// The rotation functions below are the way they are for a couple of reasons:
//  1. They are safer (well folks seem to think so; see https://en.wikipedia.org/wiki/Circular_shift#Implementing_circular_shifts)
//  2. We are using C library constants and there is just 1 numeric literal - '1'
//  3. GGC recognizes the 'pattern' and will optimize it out to 'roX' and 3 more instructions when using O2
static inline uint8_t func__rol8(uint8_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

static inline uint8_t func__ror8(uint8_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value >> count) | (value << (-count & mask));
}

static inline uint16_t func__rol16(uint16_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

static inline uint16_t func__ror16(uint16_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value >> count) | (value << (-count & mask));
}

static inline uint32_t func__rol32(uint32_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

static inline uint32_t func__ror32(uint32_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value >> count) | (value << (-count & mask));
}

static inline uint64_t func__rol64(uint64_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value << count) | (value >> (-count & mask));
}

static inline uint64_t func__ror64(uint64_t value, unsigned int count) {
    const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
    count &= mask;
    return (value >> count) | (value << (-count & mask));
}
// a740g: ROR & ROL additions end

// bit-shifting
static inline uint64_t func__shl(uint64_t a1, int b1) { return a1 << b1; }
static inline uint64_t func__shr(uint64_t a1, int b1) { return a1 >> b1; }

static inline int64_t func__readbit(uint64_t a1, int b1) {
    if (a1 & 1ull << b1)
        return -1;
    else
        return 0;
}

static inline uint64_t func__setbit(uint64_t a1, int b1) { return a1 | 1ull << b1; }
static inline uint64_t func__resetbit(uint64_t a1, int b1) { return a1 & ~(1ull << b1); }
static inline uint64_t func__togglebit(uint64_t a1, int b1) { return a1 ^ 1ull << b1; }

