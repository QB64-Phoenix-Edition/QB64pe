#pragma once

#include "../../libqb.h"

extern int32 environment__window_width;
extern int32 environment__window_height;
extern int32 os_resize_event;
extern int32 resize_auto;
extern float resize_auto_ideal_aspect;
extern int32 fullscreen_allowedmode;
extern int32 fullscreen_allowedsmooth;
extern int32 fullscreen_smooth;
extern int32 fullscreen_width;
extern int32 fullscreen_height;
extern int32 screen_scale;
extern int32 resize_pending;
extern int32 resize_snapback;
extern int32 resize_snapback_x;
extern int32 resize_snapback_y;
extern int32 resize_event;
extern int32 resize_event_x;
extern int32 resize_event_y;
extern int32 ScreenResizeScale;
extern int32 ScreenResize;
extern int32 display_x;
extern int32 display_y;
extern int32 display_x_prev;
extern int32 display_y_prev;
extern int32 display_required_x;
extern int32 display_required_y;
extern int32 full_screen;
extern int32 full_screen_set;

void sync_resize_auto_aspect_constraint();
void window_update_for_frame(int32 frame_width, int32 frame_height);
void window_update_environment_size();
void GLUT_RESIZE_FUNC(int width, int height);

void sub__fullscreen(int32 method, int32 passed);
void sub__allowfullscreen(int32 method, int32 smooth);
int32 func__fullscreen();
int32 func__fullscreensmooth();

void sub__resize(int32 on_off, int32 stretch_smooth);
int32 func__resize();
int32 func__resizewidth();
int32 func__resizeheight();

int32_t func__desktopwidth();
int32_t func__desktopheight();
void sub_screenicon();
int32 func_screenicon();
int32_t func__hasfocus();
int32_t func__screenx();
int32_t func__screeny();
void sub__screenmove(int32 x, int32 y, int32 passed);
void sub__screenshow();
void sub__screenhide();
int32 func__screenhide();
void sub__title(qbs *title);
qbs *func__title();
