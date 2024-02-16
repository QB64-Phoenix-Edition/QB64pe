#pragma once

#include <stdint.h>

#include "qbs.h"

extern qbs *func_command_str;

qbs *func_command(int32_t index, int32_t passed);
int32_t func__commandcount();
void command_initialize(int argc, char **argv);

