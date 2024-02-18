
#include "libqb-common.h"

#include <math.h>

#include "error_handle.h"
#include "qbmath.h"

double func_log(double value) {
    if (value <= 0) {
        error(5);
        return 0;
    }
    return std::log(value);
}

// FIX
double func_fix_double(double value) {
    if (value < 0)
        return std::ceil(value);
    else
        return std::floor(value);
}

long double func_fix_float(long double value) {
    if (value < 0)
        return std::ceil(value);
    else
        return std::floor(value);
}

// EXP
double func_exp_single(double value) {
    if (value <= 88.02969) {
        return std::exp(value);
    }
    error(6);
    return 0;
}

long double func_exp_float(long double value) {
    if (value <= 709.782712893) {
        return std::exp(value);
    }
    error(6);
    return 0;
}

double func_sqr(double value) {
    if (value < 0) {
        error(5);
        return 0;
    }
    return std::sqrt(value);
}

long double pow2(long double x, long double y) {
    if (x < 0) {
        if (y != std::floor(y)) {
            error(5);
            return 0;
        }
    }
    return std::pow(x, y);
}

