#include "libqb-common.h"

#include "completion.h"
#include "glut-thread.h"
#include "gui.h"
#include "keyboard.h"
#include "logging.h"
#include "thread.h"
#include <cstdint>
#include <cstdlib>

// FIXME: These extern variable and function definitions should probably go
// somewhere more global so that they can be referenced by libqb.cpp
extern uint8_t *window_title;
extern int32_t framebufferobjects_supported;
extern int32_t screen_hide;
extern const void *generic_window_handle;

extern void MAIN_LOOP(void *);
extern void GLUT_EXIT_FUNC();
extern void GLUT_RESIZE_FUNC(int width, int height);
extern void GLUT_DISPLAY_REQUEST();
extern void GLUT_IDLE_FUNC();
extern void GLUT_MOUSE_BUTTON_FUNC(double x, double y, GLUTEmu_MouseButton button, GLUTEmu_ButtonAction action, GLUTEnum_MouseCursorMode mode, int modifiers);
extern void GLUT_MOUSE_SCROLL_FUNC(double x, double y, double xOffset, double yOffset, GLUTEnum_MouseCursorMode mode);
extern void GLUT_MOUSE_POSITION_FUNC(double x, double y, GLUTEnum_MouseCursorMode mode);

// Performs all of the FreeGLUT initialization except for calling glutMainLoop()
static void initialize_glut() {
    GLUTEmu_WindowSetHint(GLUTEmu_WindowHint::FramebufferSamples, 4);
    GLUTEmu_WindowSetHint(GLUTEmu_WindowHint::FramebufferDoubleBuffer, true);
    GLUTEmu_WindowSetHint(GLUTEmu_WindowHint::WindowScaleToMonitor, true);
    GLUTEmu_WindowSetHint(GLUTEmu_WindowHint::WindowScaleFramebuffer, true);
    GLUTEmu_WindowSetHint(GLUTEmu_WindowHint::WindowVisible, !screen_hide);

    if (!GLUTEmu_WindowCreate(window_title ? reinterpret_cast<const char *>(window_title) : "Untitled", 640, 400)) {
        gui_alert("Failed to initialize window");
        exit(EXIT_FAILURE);
    }

    if (GLAD_GL_EXT_framebuffer_object) {
        framebufferobjects_supported = 1;

        libqb_log_trace("GLAD_GL_EXT_framebuffer_object supported");
    }

    generic_window_handle = GLUTEmu_WindowGetNativeHandle(0);

    // GLFW_TODO: check implementation - glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    GLUTEmu_WindowSetCloseFunction(GLUT_EXIT_FUNC);
    GLUTEmu_WindowSetResizedFunction(GLUT_RESIZE_FUNC);
    // GLFW_TODO: Framebuffer resize handling
    // GLFW_TODO: Maximize/Minimize handling
    // GLFW_TODO: Focus handling
    GLUTEmu_WindowSetRefreshFunction(GLUT_DISPLAY_REQUEST);
    GLUTEmu_WindowSetIdleFunction(GLUT_IDLE_FUNC);

    GLUTEmu_KeyboardSetButtonFunction(GLUT_KEYBOARD_BUTTON_FUNC);
    // GLFW_TODO: Character input handling?

    GLUTEmu_MouseSetButtonFunction(GLUT_MOUSE_BUTTON_FUNC);
    GLUTEmu_MouseSetPositionFunction(GLUT_MOUSE_POSITION_FUNC);
    GLUTEmu_MouseSetScrollFunction(GLUT_MOUSE_SCROLL_FUNC);
    // GLFW_TODO: Mouse enter/leave handling

    // GLFW_TODO: File drop handling
}

static bool glut_is_started;
static struct completion glut_thread_starter;
static struct completion *glut_thread_initialized;

void libqb_start_glut_thread() {
    if (glut_is_started)
        return;

    struct completion init;
    completion_init(&init);

    glut_thread_initialized = &init;

    completion_finish(&glut_thread_starter);

    completion_wait(&init);
    completion_clear(&init);
}

// Checks whether the GLUT thread is running
bool libqb_is_glut_up() {
    return glut_is_started;
}

void libqb_glut_presetup() {
    if (!screen_hide) {
        initialize_glut(); // Initialize GLUT if the screen isn't hidden
        glut_is_started = true;
    } else {
        completion_init(&glut_thread_starter);
    }
}

void libqb_start_main_thread() {
    // Start the 'MAIN_LOOP' in a separate thread, as GLUT has to run on the
    // initial thread.
    struct libqb_thread *main_loop = libqb_thread_new();
    libqb_thread_start(main_loop, MAIN_LOOP, NULL);

    // This happens for $SCREENHIDE programs. This thread waits on the
    // `glut_thread_starter` completion, which will get completed if a
    // _ScreenShow is used.
    if (!glut_is_started) {
        completion_wait(&glut_thread_starter);

        initialize_glut();
        glut_is_started = true;

        if (glut_thread_initialized)
            completion_finish(glut_thread_initialized);
    }

    GLUTEmu_MainLoop();
}

// Due to GLUT making use of cleanup via atexit, we have to call exit() from
// the same thread handling the GLUT logic so that the atexit handler also runs
// from that thread (not doing that can result in a segfault due to using GLUT
// from two threads at the same time).
//
// This is accomplished by simply queuing a GLUT message that calls exit() for us.
void libqb_exit(int exitcode) {
    if (libqb_is_glut_up()) {
        GLUTEmu_ProgramExit(exitcode);
    } else {
        // If GLUT isn't running then we're free to do the exit() call from here
        exit(exitcode);
    }
}
