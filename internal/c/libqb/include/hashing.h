//-----------------------------------------------------------------------------------------------------
//  QB64-PE Hashing Library
//  Uses hash functions from miniz and FreeType
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

struct qbs;

uint32_t func__adler32(qbs *text);
uint32_t func__crc32(qbs *text);
qbs *func__md5(qbs *text);
