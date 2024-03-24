#pragma once

#ifdef QB64_MACOSX
void macMouseInit();
void macMouseDone();
void macMouseUpdatePosition(int x, int y);
#else
static inline void macMouseInit() {}
static inline void macMouseDone() {}
static inline void macMouseUpdatePosition(int x, int y) {}
#endif
