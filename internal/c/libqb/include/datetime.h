#ifndef INCLUDE_LIBQB_DATETIME_H
#define INCLUDE_LIBQB_DATETIME_H

#include <stdint.h>
#include "qbs.h"

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
