//-----------------------------------------------------------------------------------------------------
//  QB64-PE Checksum Library
//  Powered by miniz (https://github.com/richgel999/miniz)
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

struct qbs;

uint32_t func__adler32(qbs *text);
uint32_t func__crc32(qbs *text);
