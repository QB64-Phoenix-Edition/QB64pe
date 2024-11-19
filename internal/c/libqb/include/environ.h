#pragma once

#include "qbs.h"
#include <stdint.h>

int32_t func__environcount();
qbs *func_environ(qbs *name);
qbs *func_environ(int32_t number);
void sub_environ(qbs *str);
