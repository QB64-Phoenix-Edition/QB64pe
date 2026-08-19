//-----------------------------------------------------------------------------------------------------
// PCX Loader for QB64-PE by a740g
//
// References:
// https://github.com/EzArIk/PcxFileType
// https://github.com/mackron/dr_pcx
// http://fileformats.archiveteam.org/wiki/PCX
// https://en.wikipedia.org/wiki/PCX
// https://moddingwiki.shikadi.net/wiki/PCX_Format
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <cstdlib>

uint32_t *pcx_load_memory(const void *data, size_t dataSize, int *x, int *y, int *components);
uint32_t *pcx_load_file(const char *filename, int *x, int *y, int *components);
