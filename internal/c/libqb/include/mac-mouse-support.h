#pragma once

#ifdef QB64_MACOSX
void macMouseInit();
void macMouseDone();
void macMouseUpdatePosition(int x, int y);
void macMouseAssociateMouseAndMouseCursorPosition(bool connected);
#else
static inline void macMouseInit() {}

static inline void macMouseDone() {}

static inline void macMouseUpdatePosition(int x, int y) {
    (void)x;
    (void)y;
}

static inline void macMouseAssociateMouseAndMouseCursorPosition(bool connected) {
    (void)connected;
}
#endif
