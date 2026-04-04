#include "error_handle.h"
#include "window.h"

// Most of these are stubs, but we could wire these up to to a library to do something useful in the future

extern void sub__consoletitle(qbs *);

extern int32_t force_display_update;
extern int32_t screen_hide;

int32_t environment__window_width = 0;
int32_t environment__window_height = 0;
int32_t os_resize_event = 0;
int32_t resize_auto = 0; // 1=_STRETCH, 2=_SMOOTH
float resize_auto_ideal_aspect = 640.0 / 400.0;
int32_t fullscreen_allowedmode = 0;
int32_t fullscreen_allowedsmooth = 0;
int32_t fullscreen_smooth = 0;
int32_t fullscreen_width = 0;
int32_t fullscreen_height = 0;
int32_t screen_scale = 0;
int32_t resize_pending = 1;
int32_t resize_snapback = 1;
int32_t resize_snapback_x = 640;
int32_t resize_snapback_y = 400;
int32_t resize_event = 0;
int32_t resize_event_x = 0;
int32_t resize_event_y = 0;
int32_t ScreenResizeScale = 0;
int32_t ScreenResize = 0;
int32_t display_x = 640;
int32_t display_y = 400;
int32_t display_x_prev = 640;
int32_t display_y_prev = 400;
int32_t display_required_x = 640;
int32_t display_required_y = 400;
int32_t full_screen = 0;      // 0,1(stretched/closest),2(1:1)
int32_t full_screen_set = -1; // 0(windowed),1(stretched/closest),2(1:1)

static int32_t acceptFileDrop = 0;
static int32_t droppedFileIndex = -1;

void GLUT_RESIZE_FUNC([[maybe_unused]] int width, [[maybe_unused]] int height) {
    resize_event_x = width;
    resize_event_y = height;
    resize_event = -1;
    display_x_prev = display_x;
    display_y_prev = display_y;
    display_x = width;
    display_y = height;
    resize_pending = 0;
    os_resize_event = 1;
}

void window_update_for_frame(int32_t frame_width, int32_t frame_height) {
    os_resize_event = 0;
    display_required_x = frame_width;
    display_required_y = frame_height;
    resize_auto_ideal_aspect = (float)frame_width / (float)frame_height;
    resize_snapback_x = display_required_x;
    resize_snapback_y = display_required_y;
    display_x = frame_width;
    display_y = frame_height;

    if (full_screen_set != -1) {
        full_screen = full_screen_set;
        full_screen_set = -1;
    }
}

void window_update_environment_size() {
    environment__window_width = display_x;
    environment__window_height = display_y;
}

void sub__fullscreen(int32_t method, int32_t passed) {
    int32_t x = 0;
    if (method == 0)
        x = 1;
    if (method == 1 || method == 4)
        x = 0;
    if (method == 2)
        x = 1;
    if (method == 3)
        x = 2;
    if (passed & 1)
        fullscreen_smooth = 1;
    else
        fullscreen_smooth = 0;
    if (full_screen != x)
        full_screen_set = x;
    force_display_update = 1;
}

void sub__allowfullscreen(int32_t method, int32_t smooth) {
    fullscreen_allowedmode = method;
    if (method == 3 || method == 5)
        fullscreen_allowedmode = -1;
    if (method == 4 || method == 0)
        fullscreen_allowedmode = 0;

    fullscreen_allowedsmooth = smooth;
    if (smooth == 2 || smooth == 4)
        fullscreen_allowedsmooth = -1;
    if (smooth == 3 || smooth == 0)
        fullscreen_allowedsmooth = 0;
}

int32_t func__fullscreen() {
    if (full_screen_set != -1)
        return full_screen_set;
    return full_screen;
}

int32_t func__fullscreensmooth() {
    return -fullscreen_smooth;
}

void sub__resize(int32_t on_off, int32_t stretch_smooth) {
    if (on_off == 1)
        resize_snapback = 0;
    if (on_off == 2)
        resize_snapback = 1;

    if (stretch_smooth) {
        resize_auto = stretch_smooth;
    } else {
        resize_auto = 0;
    }
}

int32_t func__resize() {
    if (resize_snapback)
        return 0;
    if (resize_event) {
        resize_event = 0;
        return -1;
    }
    return 0;
}

int32_t func__resizewidth() {
    return resize_event_x;
}

int32_t func__resizeheight() {
    return resize_event_y;
}

int32_t func__desktopwidth() {
    return 0;
}

int32_t func__desktopheight() {
    return 0;
}

void sub_screenicon() {}

int32_t func_screenicon() {
    return QB_FALSE;
}

int32_t func__hasfocus() {
    return QB_FALSE;
}

int32_t func__screenx() {
    return 0;
}

int32_t func__screeny() {
    return 0;
}

void sub__screenmove([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y, int32_t passed) {
    if (is_error_pending() || full_screen) {
        return;
    }

    if (!passed || passed == 3) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
    }
}

void sub__screenshow() {
    screen_hide = 0;
}

void sub__screenhide() {
    if (screen_hide)
        return;

    screen_hide = 1;
}

int32_t func__screenhide() {
    return -screen_hide;
}

void sub__title([[maybe_unused]] qbs *title) {
    sub__consoletitle(title);
}

qbs *func__title() {
    return qbs_new_txt("");
}

void GLUT_DROPFILES_FUNC([[maybe_unused]] int count, [[maybe_unused]] const char *paths[]) {}

void sub__filedrop(int32_t on_off) {
    if (on_off == 2) {
        acceptFileDrop = 0;
        sub__finishdrop();
        return;
    }

    if ((on_off == 0) || (on_off == 1)) {
        acceptFileDrop = -1;
    }
}

int32_t func__filedrop() {
    return acceptFileDrop;
}

void sub__finishdrop() {
    droppedFileIndex = -1;
}

int32_t func__totaldroppedfiles() {
    return 0;
}

qbs *func__droppedfile([[maybe_unused]] int32_t fileIndex, [[maybe_unused]] int32_t passed) {
    droppedFileIndex = -1;
    return qbs_new_txt("");
}

uintptr_t func__windowhandle() {
#ifdef QB64_WINDOWS
    static const void *generic_window_handle = nullptr;

    if (!generic_window_handle) {
        char pszConsoleTitle[1024];
        GetConsoleTitle(pszConsoleTitle, 1024);
        generic_window_handle = FindWindow(NULL, pszConsoleTitle);
    }

    return reinterpret_cast<uintptr_t>(generic_window_handle);
#else
    return 0;
#endif
}

int32_t func_windowexists() {
    return QB_TRUE;
}
