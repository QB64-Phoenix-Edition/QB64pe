#pragma once

#include "error_handle.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

/* Extra maths functions - we do what we must because we can */
static inline constexpr double func_deg2rad(double value) {
    return (value * 0.01745329251994329576923690768489);
}

static inline constexpr double func_rad2deg(double value) {
    return (value * 57.29577951308232);
}

static inline constexpr double func_deg2grad(double value) {
    return (value * 1.111111111111111);
}

static inline constexpr double func_grad2deg(double value) {
    return (value * 0.9);
}

static inline constexpr double func_rad2grad(double value) {
    return (value * 63.66197723675816);
}

static inline constexpr double func_grad2rad(double value) {
    return (value * 0.01570796326794896);
}

static inline constexpr double func_pi(double multiplier, int32_t passed) {
    if (passed) {
        return 3.14159265358979323846264338327950288419716939937510582 * multiplier;
    }
    return (3.14159265358979323846264338327950288419716939937510582);
}

// https://en.neurochispas.com/calculators/arcsec-calculator-inverse-secant-degrees-and-radians/
static inline double func_arcsec(double num) {
    if (std::abs(num) < 1.0) {
        error(5);
        return 0.0;
    }
    return std::acos(1.0 / num);
}

// https://en.neurochispas.com/calculators/arccsc-calculator-inverse-cosecant-degrees-and-radians/
static inline double func_arccsc(double num) {
    if (std::abs(num) < 1.0) {
        error(5);
        return 0.0;
    }
    return std::asin(1.0 / num);
}

static inline double func_arccot(double num) {
    return 2 * std::atan(1) - std::atan(num);
}

static inline double func_sech(double num) {
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

static inline double func_csch(double num) {
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

static inline double func_coth(double num) {
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

static inline double func_sec(double num) {
    if (std::cos(num) == 0) {
        error(5);
        return 0;
    }
    return 1 / std::cos(num);
}

static inline double func_csc(double num) {
    if (std::sin(num) == 0) {
        error(5);
        return 0;
    }
    return 1 / std::sin(num);
}

static inline double func_cot(double num) {
    if (std::tan(num) == 0) {
        error(5);
        return 0;
    }
    return 1 / std::tan(num);
}

template <typename T> static inline constexpr bool Math_IsPowerOf2(T n) {
    static_assert(std::is_integral_v<T>, "Math_IsPowerOf2 requires an integral type");

    using UT = std::make_unsigned_t<T>;

    UT un = static_cast<UT>(n);

    return un && !(un & (un - 1));
}

template <typename T> static inline constexpr T Math_RoundUpToPowerOf2(T n) {
    static_assert(std::is_integral_v<T>, "Math_RoundUpToPowerOf2 requires an integral type");

    using UT = std::make_unsigned_t<T>;

    UT un = static_cast<UT>(n - 1);

    if constexpr (sizeof(UT) >= 1) {
        un |= un >> 1;
        un |= un >> 2;
        un |= un >> 4;
    }
    if constexpr (sizeof(UT) >= 2) {
        un |= un >> 8;
    }
    if constexpr (sizeof(UT) >= 4) {
        un |= un >> 16;
    }
    if constexpr (sizeof(UT) >= 8) {
        un |= un >> 32;
    }

    return static_cast<T>(un + 1);
}

template <typename T> static inline constexpr T Math_RoundDownToPowerOf2(T n) {
    static_assert(std::is_integral_v<T>, "Math_RoundDownToPowerOf2 requires an integral type");

    using UT = std::make_unsigned_t<T>;

    UT un = static_cast<UT>(n);

    if constexpr (sizeof(UT) >= 1) {
        un |= un >> 1;
        un |= un >> 2;
        un |= un >> 4;
    }
    if constexpr (sizeof(UT) >= 2) {
        un |= un >> 8;
    }
    if constexpr (sizeof(UT) >= 4) {
        un |= un >> 16;
    }
    if constexpr (sizeof(UT) >= 8) {
        un |= un >> 32;
    }

    return static_cast<T>(un - (un >> 1));
}
