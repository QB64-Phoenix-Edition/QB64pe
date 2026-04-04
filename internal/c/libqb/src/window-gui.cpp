#include "error_handle.h"
#include "glut-thread.h"
#include "graphics.h"
#include "libqb-common.h"
#include "window.h"
#include <string>
#include <string_view>
#include <vector>

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
static std::vector<std::string> droppedFiles;

// GLFW_TODO: Implement a func_screenrefreshrate() function

static void sync_resize_auto_aspect_constraint() {
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
    // ref: "[{_OFF|_STRETCH|_SQUAREPIXELS|OFF}][, _SMOOTH]"
    //          1      2           3        4         1
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
    // ref: "[{_STRETCH|_SQUAREPIXELS|_OFF|_ALL|OFF}][, _SMOOTH|_OFF|_ALL|OFF]"
    //            1          2         3    4   5         1      2    3   4
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
    //                   0 1  2          0 1       2
    if (on_off == 1)
        resize_snapback = 0;
    if (on_off == 2)
        resize_snapback = 1;
    // no change if omitted

    if (stretch_smooth) {
        resize_auto = stretch_smooth;
    } else {
        resize_auto = 0; // revert if omitted
    }

    sync_resize_auto_aspect_constraint();
}

int32_t func__resize() {
    if (resize_snapback)
        return 0; // resize must be enabled
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
    return std::get<0>(GLUTEmu_ScreenGetMode());
}

int32_t func__desktopheight() {
    return std::get<1>(GLUTEmu_ScreenGetMode());
}

void sub_screenicon() {
    NEEDS_GLUT();
    GLUTEmu_WindowMinimize();
}

int32_t func_screenicon() {
    OPTIONAL_GLUT(QB_FALSE);
    return QB_BOOL(GLUTEmu_WindowIsMinimized());
}

int32_t func__hasfocus() {
    OPTIONAL_GLUT(QB_FALSE);
    return QB_BOOL(GLUTEmu_WindowIsFocused());
}

int32_t func__screenx() {
    NEEDS_GLUT(0);
    return GLUTEmu_WindowGetPosition().first;
}

int32_t func__screeny() {
    NEEDS_GLUT(0);
    return GLUTEmu_WindowGetPosition().second;
}

void sub__screenmove(int32_t x, int32_t y, int32_t passed) {
    if (is_error_pending() || full_screen) {
        return;
    }

    if (!passed || passed == 3) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }

    NEEDS_GLUT();

    if (passed == 2) {
        GLUTEmu_WindowMove(x, y);
    } else {
        GLUTEmu_WindowCenter();
    }
}

void sub__screenshow() {
    screen_hide = 0;

    // $SCREENHIDE programs will not have the window running
    libqb_start_glut_thread();
    GLUTEmu_WindowHide(false);
}

void sub__screenhide() {
    if (screen_hide)
        return;

    // This is probably unnecessary, no conditions allow for screen_hide==0 without GLUT running, but it doesn't hurt anything.
    libqb_start_glut_thread();
    GLUTEmu_WindowHide(true);

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

void GLUT_DROPFILES_FUNC(int count, const char *paths[]) {
    if (!acceptFileDrop) {
        return;
    }

    droppedFiles.clear();
    droppedFileIndex = -1;

    if (count <= 0 || !paths) {
        return;
    }

    droppedFiles.reserve(static_cast<std::size_t>(count));

    for (int index = 0; index < count; ++index) {
        if (paths[index]) {
            droppedFiles.emplace_back(paths[index]);
        }
    }
}

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
    droppedFiles.clear();
    droppedFileIndex = -1;
}

int32_t func__totaldroppedfiles() {
    return static_cast<int32_t>(droppedFiles.size());
}

qbs *func__droppedfile(int32_t fileIndex, int32_t passed) {
    if (droppedFiles.empty()) {
        droppedFileIndex = -1;
        return qbs_new_txt("");
    }

    if (passed) {
        droppedFileIndex = fileIndex - 1;
    } else {
        ++droppedFileIndex;
    }

    if ((droppedFileIndex < 0) || (droppedFileIndex >= static_cast<int32_t>(droppedFiles.size()))) {
        if (!passed) {
            sub__finishdrop();
        }

        droppedFileIndex = -1;
        return qbs_new_txt("");
    }

    const auto result = qbs_new_txt(droppedFiles[static_cast<std::size_t>(droppedFileIndex)].c_str());

    if (!passed && droppedFileIndex == static_cast<int32_t>(droppedFiles.size()) - 1) {
        sub__finishdrop();
    }

    return result;
}

uintptr_t func__windowhandle() {
    static const void *generic_window_handle = nullptr;

    if (!generic_window_handle) {
        generic_window_handle = GLUTEmu_WindowGetNativeHandle(0);
    }

    return reinterpret_cast<uintptr_t>(generic_window_handle);
}

int32_t func_windowexists() {
    return QB_BOOL(libqb_is_glut_up());
}
