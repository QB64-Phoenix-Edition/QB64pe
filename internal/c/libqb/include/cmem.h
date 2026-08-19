#pragma once

#include "qbs.h"
#include <stdint.h>

extern qbs *nothingstring;

extern uint8_t cmem[1114099]; // 16*65535+65535+3 (enough for highest referencable dword in conv memory)
extern intptr_t dblock;       // Required for Play(). Did not find this declared anywhere
