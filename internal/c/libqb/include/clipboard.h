//----------------------------------------------------------------------------------------------------------------------
// QB64-PE cross-platform clipboard support
// Powered by clip (https://github.com/dacap/clip)
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

struct qbs;

qbs *func__clipboard();
void sub__clipboard(const qbs *qbsText);
int32_t func__clipboardimage();
void sub__clipboardimage(int32_t src);
