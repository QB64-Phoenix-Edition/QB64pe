#ifndef INCLUDE_LIBQB_ROUNDING_H
#define INCLUDE_LIBQB_ROUNDING_H

#include <stdint.h>

#include "event.h"

int64_t qbr(long double);
uint64_t qbr_longdouble_to_uint64(long double);
int32_t qbr_float_to_long(float);
int32_t qbr_double_to_long(double);

void fpu_reinit();

// CSNG
static inline double func_csng_float(long double value) {
    if ((value <= 3.402823466E38) && (value >= -3.402823466E38)) {
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
    if ((value <= 1.7976931348623157E308) &&
        (value >= -1.7976931348623157E308)) {
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
    if ((value < 32767.5) && (value >= -32768.5)) {
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
    if ((value < 2147483647.5) && (value >= -2147483648.5)) {
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
static inline int64_t func_round_double(long double value) { return qbr(value); }
static inline int64_t func_round_float(long double value) { return qbr(value); }

#endif
