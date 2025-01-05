#ifndef INCLUDE_LIBQB_ROUNDING_H
#define INCLUDE_LIBQB_ROUNDING_H

#include <cmath>
#include <stdint.h>

#include "event.h"

#ifdef QB64_NOT_X86

#    define qbr(f) std::llround(f)

#    define qbr_longdouble_to_uint64(f) uint64_t(std::llround(f))

#    define qbr_float_to_long(f) std::lround(f)

#    define qbr_double_to_long(f) std::lround(f)

static inline void fpu_reinit() { /* do nothing */ }

#else

// QBASIC compatible rounding via FPU:
// FLDS=load single
// FLDL=load double
// FLDT=load long double
static inline int64_t qbr(long double f) {
    int64_t i;
    int temp = 0;
    if (f > 9223372036854775807.0L) {
        temp = 1;
        f = f - 9223372036854775808ULL;
    } // if it's too large for a signed int64, make it an unsigned int64 and return that value if possible.
    __asm__("fldt %1;"
            "fistpll %0;"
            : "=m"(i)
            : "m"(f));
    if (temp)
        return i | 0x8000000000000000; // if it's an unsigned int64, manually set the bit flag
    return i;
}

static inline uint64_t qbr_longdouble_to_uint64(long double f) {
    uint64_t i;
    __asm__("fldt %1;"
            "fistpll %0;"
            : "=m"(i)
            : "m"(f));
    return i;
}

static inline int32_t qbr_float_to_long(float f) {
    int32_t i;
    __asm__("flds %1;"
            "fistpl %0;"
            : "=m"(i)
            : "m"(f));
    return i;
}

static inline int32_t qbr_double_to_long(double f) {
    int32_t i;
    __asm__("fldl %1;"
            "fistpl %0;"
            : "=m"(i)
            : "m"(f));
    return i;
}

static inline void fpu_reinit() {
    unsigned int mode = 0x37F;
    asm("fldcw %0" : : "m"(*&mode));
}

#endif // x86 support

// CSNG
static inline double func_csng_float(long double value) {
    if ((value <= 3.402823466E38L) && (value >= -3.402823466E38L)) {
        return value;
    }
    error(6);
    return 0;
}

static inline double func_csng_double(double value) {
    if ((value <= 3.402823466E38) && (value >= -3.402823466E38)) {
        return value;
    }
    error(6);
    return 0;
}

// CDBL
static inline double func_cdbl_float(long double value) {
    if ((value <= 1.7976931348623157E308L) && (value >= -1.7976931348623157E308L)) {
        return value;
    }
    error(6);
    return 0;
}

// CINT
// func_cint_single uses func_cint_double
static inline int32_t func_cint_double(double value) {
    if ((value < 32767.5) && (value >= -32768.5)) {
        return qbr_double_to_long(value);
    }
    error(6);
    return 0;
}

static inline int64_t func_cint_float(long double value) {
    if ((value < 32767.5L) && (value >= -32768.5L)) {
        return qbr(value);
    }
    error(6);
    return 0;
}

static inline int16_t func_cint_long(int32_t value) {
    if ((value >= -32768) && (value <= 32767))
        return value;
    error(6);
    return 0;
}

static inline int16_t func_cint_ulong(uint32_t value) {
    if (value <= 32767)
        return value;
    error(6);
    return 0;
}

static inline int16_t func_cint_int64(int64_t value) {
    if ((value >= -32768) && (value <= 32767))
        return value;
    error(6);
    return 0;
}

static inline int16_t func_cint_uint64(uint64_t value) {
    if (value <= 32767)
        return value;
    error(6);
    return 0;
}

// CLNG
// func_clng_single uses func_clng_double
//-2147483648 to 2147483647
static inline int32_t func_clng_double(double value) {
    if ((value < 2147483647.5) && (value >= -2147483648.5)) {
        return qbr_double_to_long(value);
    }
    error(6);
    return 0;
}

static inline int64_t func_clng_float(long double value) {
    if ((value < 2147483647.5L) && (value >= -2147483648.5L)) {
        return qbr(value);
    }
    error(6);
    return 0;
}

static inline int32_t func_clng_ulong(uint32_t value) {
    if (value <= 2147483647)
        return value;
    error(6);
    return 0;
}

static inline int32_t func_clng_int64(int64_t value) {
    if ((value >= -2147483648) && (value <= 2147483647))
        return value;
    error(6);
    return 0;
}

static inline int32_t func_clng_uint64(uint64_t value) {
    if (value <= 2147483647)
        return value;
    error(6);
    return 0;
}

//_ROUND (note: round performs no error checking)
static inline int64_t func_round_double(long double value) {
    return qbr(value);
}

static inline int64_t func_round_float(long double value) {
    return qbr(value);
}

#endif
