#pragma once

#include "libqb-common.h"

#include "qbs.h"
#include <cstdint>

extern int32_t console;
extern int32_t console_active;

int32_t func__console();
void sub__console(int32_t onoff);

void sub__consoletitle(qbs *title);
void sub__consolefont(qbs *fontName, int32_t fontSize);
void sub__console_cursor(int32_t visible, int32_t cursorsize, int32_t passed);
int32_t func__getconsoleinput();
int32_t func__cinp(int32_t toggle, int32_t passed);
int32_t func__console_mouse_x();
int32_t func__console_mouse_y();
int32_t func__console_mouse_buttons();
int32_t func__console_mouse_movementx();
int32_t func__console_mouse_movementy();
void sub__echo(qbs *message);
