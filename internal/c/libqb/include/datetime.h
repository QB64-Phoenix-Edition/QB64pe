#ifndef INCLUDE_LIBQB_DATETIME_H
#define INCLUDE_LIBQB_DATETIME_H

#include <stdint.h>
#include "qbs.h"

#ifdef QB64_LINUX
// Initializes the clock returned by 'GetTicks()' so that it starts from zero
// Should be called at the very beginning of the program
void clock_init();
#else
static inline void clock_init() { }
#endif

int64_t GetTicks();

double func_timer(double accuracy, int32_t passed);
void sub__delay(double seconds);
void sub__limit(double fps);

// We provide a 'Sleep()' function for non-Windows platforms
#ifndef QB64_WINDOWS
void Sleep(uint32_t milliseconds);
#endif

qbs *func_time();
void sub_time(qbs *str);

qbs *func_date();
void sub_date(qbs *date);

#endif
