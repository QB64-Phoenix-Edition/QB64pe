//-----------------------------------------------------------------------------------------------------
// PCX Loader for QB64-PE by a740g
//
// Uses code and ideas from:
// https://github.com/EzArIk/PcxFileType
// https://github.com/mackron/dr_pcx
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <cstdlib>

uint32_t *pcx_load_memory(const void *data, size_t dataSize, int *x, int *y, int *components);
uint32_t *pcx_load_file(const char *filename, int *x, int *y, int *components);
