#pragma once

#include "glut-emu.h"
#include "qbs.h"
#include <cstdint>

inline constexpr auto Mouse_MaxSupportedButtons = 8;
enum class Mouse_Button { None = 0, Left = 1, Right = 2, Middle = 3, Four = 4, Five = 5, Six = 6, Seven = 7, Eight = 8 };

void GLUT_MOUSE_POSITION_FUNC(double x, double y, GLUTEnum_MouseCursorMode mode);
void GLUT_MOUSE_BUTTON_FUNC(double x, double y, GLUTEmu_MouseButton button, GLUTEmu_ButtonAction action, GLUTEnum_MouseCursorMode mode, int modifiers);
void GLUT_MOUSE_SCROLL_FUNC(double x, double y, double xOffset, double yOffset, GLUTEnum_MouseCursorMode mode);

void Mouse_QueueButtonDownEvent(int button, double x, double y);
void Mouse_QueueButtonUpEvent(int button, double x, double y);
void Mouse_QueuePositionEvent(double x, double y, GLUTEnum_MouseCursorMode mode);
void Mouse_QueueScrollEvent(double x, double y, double xOffset, double yOffset);

void mouse_get_int33_status(uint16_t *buttons, double *x, double *y);

void sub__mousehide();
void sub__mouseshow(qbs *qbsStyle, int32_t passed);
int32_t func__mousehidden();
double func__mousemovementx();
double func__mousemovementy();
void sub__mousemove(double x, double y);
double func__mousex();
double func__mousey();
int32_t func__mouseinput();
int32_t func__mousebutton(int32_t i);
double func__mousewheel(int32_t axis = 0, int32_t passed = 0);
