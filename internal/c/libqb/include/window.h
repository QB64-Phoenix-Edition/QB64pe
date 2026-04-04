#pragma once

#include <cstdint>

struct qbs;

extern int32_t environment__window_width;
extern int32_t environment__window_height;
extern int32_t os_resize_event;
extern int32_t resize_auto;
extern float resize_auto_ideal_aspect;
extern int32_t fullscreen_allowedmode;
extern int32_t fullscreen_allowedsmooth;
extern int32_t fullscreen_smooth;
extern int32_t fullscreen_width;
extern int32_t fullscreen_height;
extern int32_t screen_scale;
extern int32_t resize_pending;
extern int32_t resize_snapback;
extern int32_t resize_snapback_x;
extern int32_t resize_snapback_y;
extern int32_t resize_event;
extern int32_t resize_event_x;
extern int32_t resize_event_y;
extern int32_t ScreenResizeScale;
extern int32_t ScreenResize;
extern int32_t display_x;
extern int32_t display_y;
extern int32_t display_x_prev;
extern int32_t display_y_prev;
extern int32_t display_required_x;
extern int32_t display_required_y;
extern int32_t full_screen;
extern int32_t full_screen_set;

void window_update_for_frame(int32_t frame_width, int32_t frame_height);
void window_update_environment_size();
void GLUT_RESIZE_FUNC(int width, int height);
void GLUT_DROPFILES_FUNC(int count, const char *paths[]);

void sub__fullscreen(int32_t method, int32_t passed);
void sub__allowfullscreen(int32_t method, int32_t smooth);
int32_t func__fullscreen();
int32_t func__fullscreensmooth();

void sub__resize(int32_t on_off, int32_t stretch_smooth);
int32_t func__resize();
int32_t func__resizewidth();
int32_t func__resizeheight();

int32_t func__desktopwidth();
int32_t func__desktopheight();
void sub_screenicon();
int32_t func_screenicon();
int32_t func__hasfocus();
int32_t func__screenx();
int32_t func__screeny();
void sub__screenmove(int32_t x, int32_t y, int32_t passed);
void sub__screenshow();
void sub__screenhide();
int32_t func__screenhide();
void sub__title(qbs *title);
qbs *func__title();
void sub__filedrop(int32_t on_off);
int32_t func__filedrop();
void sub__finishdrop();
int32_t func__totaldroppedfiles();
qbs *func__droppedfile(int32_t fileIndex, int32_t passed);

uintptr_t func__windowhandle();
int32_t func_windowexists();
