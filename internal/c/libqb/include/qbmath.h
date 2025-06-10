#pragma once

#include "error_handle.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

static inline double func_log(double value) {
    if (value <= 0) {
        error(5);
        return 0;
    }
    return std::log(value);
}

// FIX
template <typename T> static inline T func_fix_double(T value) {
    if constexpr (std::is_floating_point_v<T>) {
        return (value < 0) ? std::ceil(value) : std::floor(value);
    } else {
        return value;
    }
}

template <typename T> static inline T func_fix_float(T value) {
    if constexpr (std::is_floating_point_v<T>) {
        return (value < 0) ? std::ceil(value) : std::floor(value);
    } else {
        return value;
    }
}

// EXP
static inline double func_exp_single(double value) {
    if (value <= 88.02969) {
        return std::exp(value);
    }
    error(6);
    return 0;
}

static inline long double func_exp_float(long double value) {
    if (value <= 709.782712893) {
        return std::exp(value);
    }
    error(6);
    return 0;
}

static inline double func_sqr(double value) {
    if (value < 0) {
        error(5);
        return 0;
    }
    return std::sqrt(value);
}

static inline long double pow2(long double x, long double y) {
    if (x < 0) {
        if (y != std::floor(y)) {
            error(5);
            return 0;
        }
    }
    return std::pow(x, y);
}

template <typename T> static inline T func_abs(T v) {
    static_assert(std::is_arithmetic_v<T>, "func_abs requires an arithmetic type");

    if constexpr (std::is_unsigned_v<T>) {
        return v;
    } else if constexpr (std::is_floating_point_v<T>) {
        return std::fabs(v);
    } else if constexpr (sizeof(T) <= sizeof(int32_t)) {
        return std::abs(v);
    } else {
        return std::llabs(v);
    }
}

template <typename T> static inline constexpr int32_t func_sgn(T v) {
    static_assert(std::is_arithmetic_v<T>, "func_sgn requires an arithmetic type");

    if constexpr (std::is_unsigned_v<T>) {
        return v != 0 ? 1 : 0;
    } else {
        return (T(0) < v) - (v < T(0));
    }
}
