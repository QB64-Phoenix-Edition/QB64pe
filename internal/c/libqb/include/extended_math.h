#pragma once

#include "error_handle.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>

/* Extra maths functions - we do what we must because we can */
inline constexpr double func_deg2rad(double value) {
    return (value * 0.01745329251994329576923690768489);
}

inline constexpr double func_rad2deg(double value) {
    return (value * 57.29577951308232);
}

inline constexpr double func_deg2grad(double value) {
    return (value * 1.111111111111111);
}

inline constexpr double func_grad2deg(double value) {
    return (value * 0.9);
}

inline constexpr double func_rad2grad(double value) {
    return (value * 63.66197723675816);
}

inline constexpr double func_grad2rad(double value) {
    return (value * .01570796326794896);
}

inline constexpr double func_pi(double multiplier, int32_t passed) {
    if (passed) {
        return 3.14159265358979323846264338327950288419716939937510582 * multiplier;
    }
    return (3.14159265358979323846264338327950288419716939937510582);
}

// https://en.neurochispas.com/calculators/arcsec-calculator-inverse-secant-degrees-and-radians/
inline double func_arcsec(double num) {
    if (std::abs(num) < 1.0) {
        error(5);
        return 0.0;
    }
    return std::acos(1.0 / num);
}

// https://en.neurochispas.com/calculators/arccsc-calculator-inverse-cosecant-degrees-and-radians/
inline double func_arccsc(double num) {
    if (std::abs(num) < 1.0) {
        error(5);
        return 0.0;
    }
    return std::asin(1.0 / num);
}

inline double func_arccot(double num) {
    return 2 * std::atan(1) - std::atan(num);
}

inline double func_sech(double num) {
    if (num > 88.02969) {
        error(5);
        return 0;
    }
    if (std::exp(num) + std::exp(-num) == 0) {
        error(5);
        return 0;
    }
    return 2 / (std::exp(num) + std::exp(-num));
}

inline double func_csch(double num) {
    if (num > 88.02969) {
        error(5);
        return 0;
    }
    if (std::exp(num) - std::exp(-num) == 0) {
        error(5);
        return 0;
    }
    return 2 / (std::exp(num) - std::exp(-num));
}

inline double func_coth(double num) {
    if (num > 44.014845) {
        error(5);
        return 0;
    }
    if (2 * std::exp(num) - 1 == 0) {
        error(5);
        return 0;
    }
    return 2 * std::exp(num) - 1;
}

inline double func_sec(double num) {
    if (std::cos(num) == 0) {
        error(5);
        return 0;
    }
    return 1 / std::cos(num);
}

inline double func_csc(double num) {
    if (std::sin(num) == 0) {
        error(5);
        return 0;
    }
    return 1 / std::sin(num);
}

inline double func_cot(double num) {
    if (std::tan(num) == 0) {
        error(5);
        return 0;
    }
    return 1 / std::tan(num);
}

/// @brief Check if n is a power of 2
/// @param n A number
/// @return True if n is a power of 2
inline constexpr bool Math_IsPowerOf2(uint32_t n) {
    return n && !(n & (n - 1));
}

/// @brief Check if n is a power of 2
/// @param n A number
/// @return True if n is a power of 2
inline constexpr bool Math_IsPowerOf2(uint64_t n) {
    return n && !(n & (n - 1));
}

/// @brief Returns the next (ceiling) power of 2 for n. E.g. n = 600 then returns 1024
/// @param n Any number
/// @return Next (ceiling) power of 2 for n
inline constexpr uint32_t Math_RoundUpToPowerOf2(uint32_t n) {
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return ++n;
}

/// @brief Returns the next (ceiling) power of 2 for n. E.g. n = 600 then returns 1024
/// @param n Any number
/// @return Next (ceiling) power of 2 for n
inline constexpr uint64_t Math_RoundUpToPowerOf2(uint64_t n) {
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return ++n;
}

/// @brief Returns the previous (floor) power of 2 for n. E.g. n = 600 then returns 512
/// @param n Any number
/// @return Previous (floor) power of 2 for n
inline constexpr uint32_t Math_RoundDownToPowerOf2(uint32_t n) {
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
    return n - (n >> 1);
}

/// @brief Returns the previous (floor) power of 2 for n. E.g. n = 600 then returns 512
/// @param n Any number
/// @return Previous (floor) power of 2 for n
inline constexpr uint64_t Math_RoundDownToPowerOf2(uint64_t n) {
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n - (n >> 1);
}
