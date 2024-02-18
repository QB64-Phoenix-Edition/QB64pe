#pragma once

#include <cmath>
#include <cstdlib>
#include <stdint.h>

double func_log(double value);
double func_fix_double(double value);
long double func_fix_float(long double value);
double func_exp_single(double value);
long double func_exp_float(long double value);

double func_sqr(double value);
long double pow2(long double x, long double y);

// force abs to return floating point numbers correctly
static inline double func_abs(double d) { return std::fabs(d); }
static inline long double func_abs(long double d) { return std::fabs(d); }
static inline float func_abs(float d) { return std::fabs(d); }

static inline uint8_t func_abs(uint8_t d) { return d; }
static inline uint16_t func_abs(uint16_t d) { return d; }
static inline uint32_t func_abs(uint32_t d) { return d; }
static inline uint64_t func_abs(uint64_t d) { return d; }
static inline int8_t func_abs(int8_t d) { return std::abs(d); }
static inline int16_t func_abs(int16_t d) { return std::abs(d); }
static inline int32_t func_abs(int32_t d) { return std::abs(d); }
static inline int64_t func_abs(int64_t d) { return std::llabs(d); }

static inline int32_t func_sgn(uint8_t v) {
    if (v)
        return 1;
    else
        return 0;
}

static inline int32_t func_sgn(int8_t v) {
    if (v) {
        if (v > 0)
            return 1;
        else
            return -1;
    }
    return 0;
}

static inline int32_t func_sgn(uint16_t v) {
    if (v)
        return 1;
    else
        return 0;
}

static inline int32_t func_sgn(int16_t v) {
    if (v) {
        if (v > 0)
            return 1;
        else
            return -1;
    }
    return 0;
}

static inline int32_t func_sgn(uint32_t v) {
    if (v)
        return 1;
    else
        return 0;
}

static inline int32_t func_sgn(int32_t v) {
    if (v) {
        if (v > 0)
            return 1;
        else
            return -1;
    }
    return 0;
}

static inline int32_t func_sgn(uint64_t v) {
    if (v)
        return 1;
    else
        return 0;
}

static inline int32_t func_sgn(int64_t v) {
    if (v) {
        if (v > 0)
            return 1;
        else
            return -1;
    }
    return 0;
}

static inline int32_t func_sgn(float v) {
    if (v) {
        if (v > 0)
            return 1;
        else
            return -1;
    }
    return 0;
}

static inline int32_t func_sgn(double v) {
    if (v) {
        if (v > 0)
            return 1;
        else
            return -1;
    }
    return 0;
}

static inline int32_t func_sgn(long double v) {
    if (v) {
        if (v > 0)
            return 1;
        else
            return -1;
    }
    return 0;
}
