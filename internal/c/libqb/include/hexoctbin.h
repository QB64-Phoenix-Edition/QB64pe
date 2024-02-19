#pragma once

#include <stdint.h>

#include "qbs.h"

qbs *func__bin(int64_t value, int32_t neg_bits);
qbs *func__bin_float(long double value);
qbs *func_oct(int64_t value, int32_t neg_bits);
qbs *func_oct_float(long double value);
qbs *func_hex(int64_t value, int32_t neg_size);
qbs *func_hex_float(long double value);
