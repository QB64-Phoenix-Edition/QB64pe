//-----------------------------------------------------------------------------------------------------
//  QB64-PE Compression Library
//  Powered by miniz (https://github.com/richgel999/miniz)
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

struct qbs;

qbs *func__deflate(qbs *src, int32_t level = -1, int32_t passed = 0);
qbs *func__inflate(qbs *text, int64_t originalsize, int32_t passed);
