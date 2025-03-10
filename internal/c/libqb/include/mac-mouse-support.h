#pragma once

#ifdef QB64_MACOSX
void MacMouse_UpdatePosition(int x, int y);
#else
static inline void MacMouse_UpdatePosition(int x, int y) {
    (void)x;
    (void)y;
}
#endif
