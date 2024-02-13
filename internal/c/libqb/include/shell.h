#pragma once

#include <stdint.h>
#include "qbs.h"

extern int32_t shell_call_in_progress;

int64_t func_shell(qbs *str);
int64_t func__shellhide(qbs *str);
void sub_shell(qbs *str, int32_t passed);
void sub_shell2(qbs *str, int32_t passed);
void sub_shell3(qbs *str, int32_t passed);
void sub_shell4(qbs *str, int32_t passed);
