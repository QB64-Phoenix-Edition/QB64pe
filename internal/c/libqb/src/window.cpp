#include "window.h"

#include "error_handle.h"
#include "glut-thread.h"
#include "graphics.h"

extern int32_t force_display_update;
extern int32_t screen_hide;
extern void set_view(int32_t new_mode);

const void *generic_window_handle = nullptr;

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

// GLFW_TODO: Implement a func_screenrefreshrate() function

void sync_resize_auto_aspect_constraint() {
    static int32_t last_constraint_enabled = -1;
    static int32_t last_constraint_w = 0;
    static int32_t last_constraint_h = 0;

    const int32_t should_enable = (resize_auto && full_screen == 0 && full_screen_set == -1) ? 1 : 0;
    if (should_enable) {
        const int32_t target_w = display_required_x;
        const int32_t target_h = display_required_y;
        if (target_w > 0 && target_h > 0) {
            if (!last_constraint_enabled || target_w != last_constraint_w || target_h != last_constraint_h) {
                GLUTEmu_WindowSetAspectRatio(target_w, target_h);
                last_constraint_enabled = 1;
                last_constraint_w = target_w;
                last_constraint_h = target_h;
            }
        }
    } else if (last_constraint_enabled != 0) {
        GLUTEmu_WindowSetAspectRatio(-1, -1);
        last_constraint_enabled = 0;
        last_constraint_w = 0;
        last_constraint_h = 0;
    }
}

void GLUT_RESIZE_FUNC(int width, int height) {
    resize_event_x = width;
    resize_event_y = height;
    resize_event = -1;
    display_x_prev = display_x;
    display_y_prev = display_y;
    display_x = width;
    display_y = height;
    resize_pending = 0;
    os_resize_event = 1;
    sync_resize_auto_aspect_constraint();
    set_view(VIEW_MODE__UNKNOWN);
}

void window_update_for_frame(int32_t frame_width, int32_t frame_height) {
    os_resize_event = 0;

    if ((full_screen == 0) && (full_screen_set == -1)) {
        display_required_x = frame_width;
        display_required_y = frame_height;

        int32_t framesize_changed = 0;
        if ((display_required_x != resize_snapback_x) || (display_required_y != resize_snapback_y))
            framesize_changed = 1;

        resize_auto_ideal_aspect = (float)frame_width / (float)frame_height;
        resize_snapback_x = display_required_x;
        resize_snapback_y = display_required_y;

        sync_resize_auto_aspect_constraint();

        if ((display_required_x != display_x) || (display_required_y != display_y)) {
            if (resize_snapback || framesize_changed) {
                GLUTEmu_WindowResize(display_required_x, display_required_y);
                GLUTEmu_WindowRefresh();
                resize_pending = 1;
            }
        }
    }

    if (full_screen_set != -1) {
        if (full_screen_set == 0) {
            if (full_screen != 0) {
                GLUTEmu_WindowFullScreen(false);
                GLUTEmu_WindowRefresh();
            }
            full_screen = 0;
            full_screen_set = -1;
        } else {
            if (resize_pending && full_screen == 0) {
                if (display_x == frame_width && display_y == frame_height) {
                    resize_pending = 0;
                }
            }

            if (!resize_pending) {
                if (full_screen == 0) {
                    GLUTEmu_WindowFullScreen(true);
                }
                full_screen = full_screen_set;
                full_screen_set = -1;
            }
        }
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

    sync_resize_auto_aspect_constraint();
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
#ifdef QB64_GUI
    return std::get<0>(GLUTEmu_ScreenGetMode());
#else
    return 0;
#endif
}

int32_t func__desktopheight() {
#ifdef QB64_GUI
    return std::get<1>(GLUTEmu_ScreenGetMode());
#else
    return 0;
#endif
}

void sub_screenicon() {
#ifdef QB64_GUI
    NEEDS_GLUT();
    GLUTEmu_WindowMinimize();
#endif
}

int32_t func_screenicon() {
#ifdef QB64_GUI
    return QB_BOOL(GLUTEmu_WindowIsMinimized());
#else
    return QB_FALSE;
#endif
}

int32_t func__hasfocus() {
#ifdef QB64_GUI
    OPTIONAL_GLUT(QB_FALSE);
    return QB_BOOL(GLUTEmu_WindowIsFocused());
#endif
    return QB_TRUE;
}

int32_t func__screenx() {
#ifdef QB64_GUI
    NEEDS_GLUT(0);
    return GLUTEmu_WindowGetPosition().first;
#endif
    return 0;
}

int32_t func__screeny() {
#ifdef QB64_GUI
    NEEDS_GLUT(0);
    return GLUTEmu_WindowGetPosition().second;
#endif
    return 0;
}

void sub__screenmove(int32_t x, int32_t y, int32_t passed) {
    if (is_error_pending() || full_screen) {
        return;
    }

    if (!passed || passed == 3) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }

#ifdef QB64_GUI
    NEEDS_GLUT();

    if (passed == 2) {
        GLUTEmu_WindowMove(x, y);
    } else {
        GLUTEmu_WindowCenter();
    }
#endif
}

void sub__screenshow() {
#ifdef QB64_GUI
    screen_hide = 0;
    libqb_start_glut_thread();
    GLUTEmu_WindowHide(false);
#endif
}

void sub__screenhide() {
    if (screen_hide)
        return;

#ifdef QB64_GUI
    libqb_start_glut_thread();
    GLUTEmu_WindowHide(true);
#endif

    screen_hide = 1;
}

int32_t func__screenhide() {
    return -screen_hide;
}

void sub__title(qbs *title) {
    GLUTEmu_WindowSetTitle(std::string_view(reinterpret_cast<const char *>(title->chr), title->len));
}

qbs *func__title() {
    auto svTitle = GLUTEmu_WindowGetTitle();
    return qbs_new_txt_len(svTitle.data(), svTitle.length());
}

uintptr_t func__windowhandle() {
#ifdef QB64_WINDOWS
#    ifdef DEPENDENCY_CONSOLE_ONLY
    if (!generic_window_handle) {
        char pszConsoleTitle[1024];
        GetConsoleTitle(pszConsoleTitle, 1024);
        generic_window_handle = FindWindow(NULL, pszConsoleTitle);
    }
    return reinterpret_cast<uintptr_t>(generic_window_handle);
#    endif
#endif

#ifdef QB64_GUI
    OPTIONAL_GLUT(0);

    generic_window_handle = GLUTEmu_WindowGetNativeHandle(0);

    return reinterpret_cast<uintptr_t>(generic_window_handle);
#else
    return 0;
#endif
}

int32_t func_windowexists() {
#ifdef QB64_GUI
    return QB_BOOL(libqb_is_glut_up());
#else
    return QB_TRUE;
#endif
}
