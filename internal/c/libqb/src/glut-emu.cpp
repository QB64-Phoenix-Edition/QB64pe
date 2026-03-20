//--------------------------------------------------------------------------------------------------------
// QB64-PE GLUT-like emulation layer
// This abstracts the underlying windowing library and provides a GLUT-like API for QB64-PE
//
// TODO:
// Custom bitmap cursor support
// Custom window icon support
// Desktop image capture support
// Mouse capture/release support
//--------------------------------------------------------------------------------------------------------

#include "libqb-common.h"

#include "completion.h"
#include "logging.h"
#include "mutex.h"

#include "glut-emu.h"
#if defined(QB64_WINDOWS)
#    define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(QB64_MACOSX)
#    define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(QB64_LINUX)
#    include <X11/XKBlib.h>
#    define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

#include <algorithm>
#include <cmath>
#include <concepts>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>

class GLUTEmu {
  public:
    class Message {
      public:
        void Finish() {
            if (finished) {
                completion_finish(finished);
            } else {
                delete this;
            }
        }

        void WaitForResponse() {
            completion_wait(finished);
        }

        virtual ~Message() {
            if (finished) {
                completion_wait(finished);
                completion_clear(finished);
                delete finished;
            }
        }

        virtual void Execute() = 0;

      protected:
        Message(bool withCompletion) {
            if (withCompletion) {
                InitCompletion();
            }
        }

      private:
        void InitCompletion() {
            finished = new completion();
            completion_init(finished);
        }

        completion *finished = nullptr;
    };

    class MessageScreenGetMode : public Message {
      public:
        std::tuple<int, int, int> responseValue;

        MessageScreenGetMode() : Message(true), responseValue(0, 0, 0) {}

        void Execute() override {
            responseValue = GLUTEmu::Instance().ScreenGetMode();
        }
    };

    template <typename T> class MessageWindowSetHint : public Message {
      public:
        GLUTEmu_WindowHint hint;
        T value;

        MessageWindowSetHint(GLUTEmu_WindowHint hint, T value) : Message(false), hint(hint), value(value) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetHint<T>(hint, value);
        }
    };

    class MessageWindowSetTitle : public Message {
      public:
        std::string newTitle;

        MessageWindowSetTitle(const char *title) : Message(false), newTitle(title) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetTitle(newTitle.c_str());
        }
    };

    class MessageWindowSetIcon : public Message {
      public:
        int32_t imageHandle;

        MessageWindowSetIcon(int32_t imageHandle) : Message(false), imageHandle(imageHandle) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetIcon(imageHandle);
        }
    };

    class MessageWindowFullscreen : public Message {
      public:
        bool fullscreen;

        MessageWindowFullscreen(bool fullscreen) : Message(false), fullscreen(fullscreen) {}

        void Execute() override {
            GLUTEmu::Instance().WindowFullscreen(fullscreen);
        }
    };

    class MessageWindowMaximize : public Message {
      public:
        MessageWindowMaximize() : Message(false) {}

        void Execute() override {
            GLUTEmu::Instance().WindowMaximize();
        }
    };

    class MessageWindowMinimize : public Message {
      public:
        MessageWindowMinimize() : Message(false) {}

        void Execute() override {
            GLUTEmu::Instance().WindowMinimize();
        }
    };

    class MessageWindowRestore : public Message {
      public:
        MessageWindowRestore() : Message(false) {}

        void Execute() override {
            GLUTEmu::Instance().WindowRestore();
        }
    };

    class MessageWindowHide : public Message {
      public:
        bool hide;

        MessageWindowHide(bool hide) : Message(false), hide(hide) {}

        void Execute() override {
            GLUTEmu::Instance().WindowHide(hide);
        }
    };

    class MessageWindowFocus : public Message {
      public:
        MessageWindowFocus() : Message(false) {}

        void Execute() override {
            GLUTEmu::Instance().WindowFocus();
        }
    };

    class MessageWindowSetFloating : public Message {
      public:
        bool floating;

        MessageWindowSetFloating(bool floating) : Message(false), floating(floating) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetFloating(floating);
        }
    };

    class MessageWindowSetOpacity : public Message {
      public:
        float opacity;

        MessageWindowSetOpacity(float opacity) : Message(false), opacity(opacity) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetOpacity(opacity);
        }
    };

    class MessageWindowSetBordered : public Message {
      public:
        bool bordered;

        MessageWindowSetBordered(bool bordered) : Message(false), bordered(bordered) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetBordered(bordered);
        }
    };

    class MessageWindowSetMousePassthrough : public Message {
      public:
        bool passthrough;

        MessageWindowSetMousePassthrough(bool passthrough) : Message(false), passthrough(passthrough) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetMousePassthrough(passthrough);
        }
    };

    class MessageWindowResize : public Message {
      public:
        int width, height;

        MessageWindowResize(int width, int height) : Message(false), width(width), height(height) {}

        void Execute() override {
            GLUTEmu::Instance().WindowResize(width, height);
        }
    };

    class MessageWindowMove : public Message {
      public:
        int x, y;

        MessageWindowMove(int x, int y) : Message(false), x(x), y(y) {}

        void Execute() override {
            GLUTEmu::Instance().WindowMove(x, y);
        }
    };

    class MessageWindowCenter : public Message {
      public:
        MessageWindowCenter() : Message(false) {}

        void Execute() override {
            GLUTEmu::Instance().WindowCenter();
        }
    };

    class MessageSetWindowAspectRatio : public Message {
      public:
        int width, height;

        MessageSetWindowAspectRatio(int width, int height) : Message(false), width(width), height(height) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetAspectRatio(width, height);
        }
    };

    class MessageSetWindowSizeLimits : public Message {
      public:
        int minWidth, minHeight, maxWidth, maxHeight;

        MessageSetWindowSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight)
            : Message(false), minWidth(minWidth), minHeight(minHeight), maxWidth(maxWidth), maxHeight(maxHeight) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetSizeLimits(minWidth, minHeight, maxWidth, maxHeight);
        }
    };

    class MessageWindowSetCloseFunction : public Message {
      public:
        GLUTEmu_CallbackWindowClose func;

        MessageWindowSetCloseFunction(GLUTEmu_CallbackWindowClose func) : Message(false), func(func) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetCloseFunction(func);
        }
    };

    class MessageWindowSetRefreshFunction : public Message {
      public:
        GLUTEmu_CallbackWindowRefresh func;

        MessageWindowSetRefreshFunction(GLUTEmu_CallbackWindowRefresh func) : Message(false), func(func) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetRefreshFunction(func);
        }
    };

    class MessageKeyboardSetCharacterFunction : public Message {
      public:
        GLUTEmu_CallbackKeyboardCharacter func;

        MessageKeyboardSetCharacterFunction(GLUTEmu_CallbackKeyboardCharacter func) : Message(false), func(func) {}

        void Execute() override {
            GLUTEmu::Instance().KeyboardSetCharacterFunction(func);
        }
    };

    class MessageSetStandardCursor : public Message {
      public:
        GLUTEmu_MouseStandardCursor style;
        bool responseValue;

        MessageSetStandardCursor(GLUTEmu_MouseStandardCursor style) : Message(true), style(style), responseValue(false) {}

        void Execute() override {
            responseValue = GLUTEmu::Instance().MouseSetStandardCursor(style);
        }
    };

    class MessageSetCustomCursor : public Message {
      public:
        int32_t imageHandle;
        bool responseValue;

        MessageSetCustomCursor(int32_t imageHandle) : Message(true), imageHandle(imageHandle), responseValue(false) {}

        void Execute() override {
            responseValue = GLUTEmu::Instance().MouseSetCustomCursor(imageHandle);
        }
    };

    class MessageSetCursorMode : public Message {
      public:
        GLUTEnum_MouseCursorMode mode;

        MessageSetCursorMode(GLUTEnum_MouseCursorMode mode) : Message(false), mode(mode) {}

        void Execute() override {
            GLUTEmu::Instance().MouseSetCursorMode(mode);
        }
    };

    class MessageMouseMove : public Message {
      public:
        double x, y;

        MessageMouseMove(double x, double y) : Message(false), x(x), y(y) {}

        void Execute() override {
            GLUTEmu::Instance().MouseMove(x, y);
        }
    };

    class MessageMouseSetPositionFunction : public Message {
      public:
        GLUTEmu_CallbackMousePosition func;

        MessageMouseSetPositionFunction(GLUTEmu_CallbackMousePosition func) : Message(false), func(func) {}

        void Execute() override {
            GLUTEmu::Instance().MouseSetPositionFunction(func);
        }
    };

    class MessageMouseSetButtonFunction : public Message {
      public:
        GLUTEmu_CallbackMouseButton func;

        MessageMouseSetButtonFunction(GLUTEmu_CallbackMouseButton func) : Message(false), func(func) {}

        void Execute() override {
            GLUTEmu::Instance().MouseSetButtonFunction(func);
        }
    };

    class MessageMouseSetNotifyFunction : public Message {
      public:
        GLUTEmu_CallbackMouseNotify func;

        MessageMouseSetNotifyFunction(GLUTEmu_CallbackMouseNotify func) : Message(false), func(func) {}

        void Execute() override {
            GLUTEmu::Instance().MouseSetNotifyFunction(func);
        }
    };

    class MessageMouseSetScrollFunction : public Message {
      public:
        GLUTEmu_CallbackMouseScroll func;

        MessageMouseSetScrollFunction(GLUTEmu_CallbackMouseScroll func) : Message(false), func(func) {}

        void Execute() override {
            GLUTEmu::Instance().MouseSetScrollFunction(func);
        }
    };

    class MessageDropSetFilesFunction : public Message {
      public:
        GLUTEmu_CallbackDropFiles func;

        MessageDropSetFilesFunction(GLUTEmu_CallbackDropFiles func) : Message(false), func(func) {}

        void Execute() override {
            GLUTEmu::Instance().DropSetFilesFunction(func);
        }
    };

    class MessageProgramExit : public Message {
      public:
        int exitCode;

        MessageProgramExit(int exitCode) : Message(false), exitCode(exitCode) {
            libqb_log_trace("Program exit requested with code: %d", exitCode);
        }

        void Execute() override {
            libqb_log_trace("Program exiting with code %d", exitCode);
            GLUTEmu::Instance().WindowSetCloseFunction(nullptr);
            GLUTEmu::Instance().WindowSetShouldClose(true);
            exit(exitCode);
        }
    };

    std::tuple<int, int, int> ScreenGetMode() {
        monitor = WindowGetCurrentMonitor();
        if (monitor) {
            auto mode = glfwGetVideoMode(monitor);
            if (mode) {
                return {ToPixelCoordsX(mode->width), ToPixelCoordsY(mode->height), mode->refreshRate};
            }
        }

        return {0, 0, 0};
    }

    template <typename T> void WindowSetHint(GLUTEmu_WindowHint hint, T value) const {
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, unsigned int> || std::is_same_v<T, bool>) {
            glfwWindowHint(int(hint), value);
            libqb_log_trace("Window hint set: %d = %d", hint, value);
        } else if constexpr (std::is_same_v<T, const char *> || std::is_same_v<T, char *>) {
            glfwWindowHintString(int(hint), value);
            libqb_log_trace("Window hint set: %d = '%s'", hint, value);
        } else {
            static_assert(!sizeof(T), "Unsupported type");
        }
    }

    bool WindowCreate(const char *title, int width, int height) {
        if (window) {
            // GLFW_TODO: sure we can; maybe we'll use it in a future version of QB64-PE
            libqb_log_error("Window already created, cannot create another window");
        } else {
            if (MessageIsMainThread()) {
                // GLFW creates the window using screen coordinates, so we need to fix it below
                window = glfwCreateWindow(width, height, title, nullptr, nullptr);
                if (window) {
                    windowTitle = title;

                    glfwSetWindowUserPointer(window, this);
                    glfwMakeContextCurrent(window);

                    auto version = gladLoadGL(glfwGetProcAddress);
                    if (!version) {
                        libqb_log_error("Failed to initialize OpenGL context");
                        glfwDestroyWindow(window);
                        window = nullptr;
                        return false;
                    }

                    libqb_log_trace("GLAD %s initialized", GLAD_GENERATOR_VERSION);
                    libqb_log_trace("OpenGL %d.%d loaded", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

                    glfwSwapInterval(1);

                    // Get the current window content scale and set a callback to track changes
                    glfwGetWindowContentScale(window, &windowScaleX, &windowScaleY);
                    glfwSetWindowContentScaleCallback(window, [](GLFWwindow *win, float xScale, float yScale) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->windowScaleX = xScale;
                        instance->windowScaleY = yScale;
                    });

                    // Get the window size and set a callback to track changes
                    glfwGetWindowSize(window, &windowWidth, &windowHeight);
                    windowWidth = ToPixelCoordsX(windowWidth);
                    windowHeight = ToPixelCoordsY(windowHeight);
                    glfwSetWindowSizeCallback(window, [](GLFWwindow *win, int width, int height) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->windowWidth = instance->ToPixelCoordsX(width);
                        instance->windowHeight = instance->ToPixelCoordsY(height);

                        if (instance->windowResizedFunction) {
                            instance->windowResizedFunction(instance->windowWidth, instance->windowHeight);
                        }
                    });

                    // If the window size is not the same as requested, we are likely on a high-DPI display, so we need to adjust our size using the scale
                    // factor
                    if (windowWidth != width || windowHeight != height) {
                        libqb_log_trace("Window size (%dx%d) does not match requested size (%dx%d) due to %fx%f content scale, adjusting", windowWidth,
                                        windowHeight, width, height, windowScaleX, windowScaleY);
                        windowWidth = width;
                        windowHeight = height;
                        glfwSetWindowSize(window, ToScreenCoordsX(width), ToScreenCoordsY(height));
                    }

                    // Get the window position and the current monitor the window is on and set a callback to track changes
                    glfwGetWindowPos(window, &windowX, &windowY);
                    windowX = ToPixelCoordsX(windowX);
                    windowY = ToPixelCoordsY(windowY);
                    monitor = WindowGetCurrentMonitor();
                    glfwSetWindowPosCallback(window, [](GLFWwindow *win, int x, int y) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->windowX = instance->ToPixelCoordsX(x);
                        instance->windowY = instance->ToPixelCoordsY(y);
                        instance->monitor = instance->WindowGetCurrentMonitor();
                    });

                    // Get the framebuffer size (already in pixels) and set a callback to track changes
                    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
                    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *win, int width, int height) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->framebufferWidth = width;
                        instance->framebufferHeight = height;

                        if (instance->windowFramebufferResizedFunction) {
                            instance->windowFramebufferResizedFunction(instance->framebufferWidth, instance->framebufferHeight);
                        }
                    });

                    // Set a hook into the maximization callback to track restore events
                    glfwSetWindowMaximizeCallback(window, [](GLFWwindow *win, int maximized) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        glfwGetWindowSize(instance->window, &instance->windowWidth, &instance->windowHeight);
                        instance->windowWidth = instance->ToPixelCoordsX(instance->windowWidth);
                        instance->windowHeight = instance->ToPixelCoordsY(instance->windowHeight);
                        instance->isWindowMaximized = bool(maximized);

                        if (instance->windowMaximizedFunction) {
                            instance->windowMaximizedFunction(instance->windowWidth, instance->windowHeight, bool(maximized));
                        }
                    });

                    // Set a hook into the minimization callback to track restore events
                    glfwSetWindowIconifyCallback(window, [](GLFWwindow *win, int iconified) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->isWindowMinimized = bool(iconified);
                        if (instance->windowMinimizedFunction) {
                            if (iconified) {
                                instance->windowMinimizedFunction(instance->windowWidth, instance->windowHeight, true);
                            } else {
                                glfwGetWindowSize(instance->window, &instance->windowWidth, &instance->windowHeight);
                                instance->windowWidth = instance->ToPixelCoordsX(instance->windowWidth);
                                instance->windowHeight = instance->ToPixelCoordsY(instance->windowHeight);
                                instance->windowMinimizedFunction(instance->windowWidth, instance->windowHeight, false);
                            }
                        }
                    });

                    // Get and save the window focus state and set a callback to track changes
                    isWindowFocused = bool(glfwGetWindowAttrib(window, GLFW_FOCUSED));
                    glfwSetWindowFocusCallback(window, [](GLFWwindow *win, int focused) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->isWindowFocused = bool(focused);
                        if (instance->windowFocusedFunction) {
                            instance->windowFocusedFunction(bool(focused));
                        }
                    });

                    // Set a hook into the keyboard button callback to enable some backward compatibility
                    glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
                    keyboardModifiers = KeyboardUpdateLockKeyModifier(GLUTEmu_KeyboardKey::CapsLock, keyboardModifiers);
                    keyboardModifiers = KeyboardUpdateLockKeyModifier(GLUTEmu_KeyboardKey::NumLock, keyboardModifiers);
                    keyboardModifiers = KeyboardUpdateLockKeyModifier(GLUTEmu_KeyboardKey::ScrollLock, keyboardModifiers);
                    glfwSetKeyCallback(window, [](GLFWwindow *win, int key, int scancode, int action, int mods) {
                        auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
#if defined(QB64_MACOSX) || defined(QB64_LINUX)
                        if (key == int(GLUTEmu_KeyboardKey::ScrollLock) && action == GLFW_RELEASE) {
                            instance->keyboardScrollLockState = !instance->keyboardScrollLockState;
                        }
#endif
                        instance->keyboardModifiers = instance->KeyboardUpdateLockKeyModifier(GLUTEmu_KeyboardKey::ScrollLock, mods);
                        if (instance->keyboardButtonFunction) {
                            instance->keyboardButtonFunction(GLUTEmu_KeyboardKey(key), scancode, GLUTEmu_ButtonAction(action), instance->keyboardModifiers);
                        }
                    });

                    if (glfwRawMouseMotionSupported()) {
                        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                        libqb_log_trace("Raw mouse motion supported and enabled");
                    } else {
                        // GLFW_TODO: Currently this fails only on macOS. So, we'll need some kind of fallback (GCMouseInput?)
                        libqb_log_warn("Raw mouse motion not supported");
                    }

                    // Refresh state variables
                    isWindowFullscreen = glfwGetWindowMonitor(window) != nullptr;
                    isWindowMaximized = bool(glfwGetWindowAttrib(window, GLFW_MAXIMIZED));
                    isWindowMinimized = bool(glfwGetWindowAttrib(window, GLFW_ICONIFIED));
                    isWindowFocused = bool(glfwGetWindowAttrib(window, GLFW_FOCUSED));
                    isWindowHidden = !glfwGetWindowAttrib(window, GLFW_VISIBLE);
                    isWindowFloating = bool(glfwGetWindowAttrib(window, GLFW_FLOATING));
                    isWindowBordered = bool(glfwGetWindowAttrib(window, GLFW_DECORATED));
                    isWindowMousePassthrough = bool(glfwGetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH));
                    windowOpacity = glfwGetWindowOpacity(window);

                    libqb_log_trace("Window created (%u x %u)", width, height);

                    return true;
                } else {
                    libqb_log_error("Window must be created from the main thread");
                }
            } else {
                libqb_log_error("Failed to create window");
            }
        }

        return false;
    }

    bool WindowIsCreated() const {
        return window != nullptr;
    }

    void WindowSetTitle(const char *title) {
        if (window) {
            glfwSetWindowTitle(window, title);
            windowTitle = glfwGetWindowTitle(window);
        } else {
            libqb_log_error("Window not created, cannot set title");
        }
    }

    std::string_view WindowGetTitle() const {
        return windowTitle;
    }

    void WindowSetIcon(int32_t imageHandle) const {
        if (window) {
            // GLFW_TODO: implement icon support
            libqb_log_warn("WindowSetIcon is not implemented");
        } else {
            libqb_log_error("Window not created, cannot set icon");
        }
    }

    void WindowFullscreen(bool fullscreen) {
        if (monitor && window) {
            if (fullscreen) {
                if (isWindowFullscreen) {
                    libqb_log_trace("Window already in fullscreen mode, ignoring request");
                } else {
                    libqb_log_trace("Entering fullscreen mode");

                    if (isWindowMaximized || isWindowMinimized) {
                        glfwRestoreWindow(window);
                    }

                    glfwGetWindowPos(window, &windowedX, &windowedY);
                    glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
                    auto mode = glfwGetVideoMode(monitor);
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                    isWindowFullscreen = glfwGetWindowMonitor(window) != nullptr;
                }
            } else {
                if (isWindowFullscreen) {
                    libqb_log_trace("Exiting fullscreen mode");

                    glfwSetWindowMonitor(window, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0);
                    isWindowFullscreen = glfwGetWindowMonitor(window) != nullptr;

                    if (!isWindowFullscreen && isWindowMaximized) {
                        glfwMaximizeWindow(window);
                    }
                } else {
                    libqb_log_trace("Window already in windowed mode, ignoring request");
                }
            }
        } else {
            libqb_log_error("Window not created, cannot set fullscreen");
        }
    }

    bool WindowIsFullscreen() const {
        return isWindowFullscreen;
    }

    void WindowMaximize() {
        if (window) {
            glfwMaximizeWindow(window);
            isWindowMaximized = bool(glfwGetWindowAttrib(window, GLFW_MAXIMIZED));

            libqb_log_trace("Window maximized");
        } else {
            libqb_log_error("Window not created, cannot maximize");
        }
    }

    bool WindowIsMaximized() const {
        return isWindowMaximized;
    }

    void WindowMinimize() {
        if (window) {
            glfwIconifyWindow(window);
            isWindowMinimized = bool(glfwGetWindowAttrib(window, GLFW_ICONIFIED));

            libqb_log_trace("Window minimized");
        } else {
            libqb_log_error("Window not created, cannot minimize");
        }
    }

    bool WindowIsMinimized() const {
        return isWindowMinimized;
    }

    void WindowRestore() {
        if (window) {
            glfwRestoreWindow(window);
            isWindowMaximized = bool(glfwGetWindowAttrib(window, GLFW_MAXIMIZED));
            isWindowMinimized = bool(glfwGetWindowAttrib(window, GLFW_ICONIFIED));

            libqb_log_trace("Window restored");
        } else {
            libqb_log_error("Window not created, cannot restore");
        }
    }

    bool WindowIsRestored() const {
        return !isWindowMaximized && !isWindowMinimized;
    }

    void WindowHide(bool hide) {
        if (window) {
            if (hide) {
                glfwHideWindow(window);
            } else {
                glfwShowWindow(window);
            }

            isWindowHidden = !glfwGetWindowAttrib(window, GLFW_VISIBLE);

            libqb_log_trace("Window %s", isWindowHidden ? "hidden" : "shown");
        } else {
            libqb_log_error("Window not created, cannot hide");
        }
    }

    bool WindowIsHidden() const {
        if (window) {
            return isWindowHidden;
        } else {
            libqb_log_error("Window not created, cannot check visibility");
        }

        return false;
    }

    void WindowFocus() {
        if (window) {
            glfwFocusWindow(window);
            isWindowFocused = bool(glfwGetWindowAttrib(window, GLFW_FOCUSED));

            libqb_log_trace("Window focused");
        } else {
            libqb_log_error("Window not created, cannot focus");
        }
    }

    bool WindowIsFocused() const {
        return isWindowFocused;
    }

    void WindowSetFloating(bool floating) {
        if (window) {
            glfwSetWindowAttrib(window, GLFW_FLOATING, floating ? GLFW_TRUE : GLFW_FALSE);
            isWindowFloating = bool(glfwGetWindowAttrib(window, GLFW_FLOATING));

            libqb_log_trace("Window floating state set to %s", isWindowFloating ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set floating state");
        }
    }

    bool WindowIsFloating() const {
        return isWindowFloating;
    }

    void WindowSetOpacity(float opacity) {
        if (window) {
            glfwSetWindowOpacity(window, opacity);
            windowOpacity = glfwGetWindowOpacity(window);

            libqb_log_trace("Window opacity set to %f", windowOpacity);
        } else {
            libqb_log_error("Window not created, cannot set opacity");
        }
    }

    float WindowGetOpacity() const {
        return windowOpacity;
    }

    void WindowSetBordered(bool bordered) {
        if (window) {
            glfwSetWindowAttrib(window, GLFW_DECORATED, bordered ? GLFW_TRUE : GLFW_FALSE);
            isWindowBordered = bool(glfwGetWindowAttrib(window, GLFW_DECORATED));

            libqb_log_trace("Window border state set to %s", isWindowBordered ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set border state");
        }
    }

    bool WindowIsBordered() const {
        return isWindowBordered;
    }

    void WindowSetMousePassthrough(bool passthrough) {
        if (window) {
            glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, passthrough ? GLFW_TRUE : GLFW_FALSE);
            isWindowMousePassthrough = bool(glfwGetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH));

            libqb_log_trace("Window mouse passthrough set to %s", passthrough ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set mouse passthrough");
        }
    }

    bool WindowAllowsMousePassthrough() const {
        return isWindowMousePassthrough;
    }

    void WindowResize(int width, int height) {
        if (window) {
            glfwSetWindowSize(window, ToScreenCoordsX(width), ToScreenCoordsY(height));

            libqb_log_trace("Window resized to (%d x %d)", width, height);
        } else {
            libqb_log_error("Window not created, cannot resize");
        }
    }

    std::pair<int, int> WindowGetSize() const {
        return {windowWidth, windowHeight};
    }

    std::pair<int, int> WindowGetFramebufferSize() const {
        return {framebufferWidth, framebufferHeight};
    }

    void WindowMove(int x, int y) {
        if (window) {
            glfwSetWindowPos(window, ToScreenCoordsX(x), ToScreenCoordsY(y));

            libqb_log_trace("Window moved to (%d, %d)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move");
        }
    }

    std::pair<int, int> WindowGetPosition() const {
        return {windowX, windowY};
    }

    void WindowCenter() {
        if (monitor && window && !isWindowFullscreen && !isWindowMaximized && !isWindowMinimized && !isWindowHidden) {
            int mx, my;
            glfwGetMonitorPos(monitor, &mx, &my);

            int mw, mh;
            glfwGetMonitorWorkarea(monitor, nullptr, nullptr, &mw, &mh);

            int ww, wh;
            glfwGetWindowSize(window, &ww, &wh);

            auto x = mx + (mw - ww) / 2;
            auto y = my + (mh - wh) / 2;
            glfwSetWindowPos(window, x, y);

            libqb_log_trace("Window centered");
        } else {
            libqb_log_error("Window not created, cannot center");
        }
    }

    void WindowSetAspectRatio(int width, int height) {
        if (window) {
            width = (width < 0 ? GLFW_DONT_CARE : width);
            height = (height < 0 ? GLFW_DONT_CARE : height);
            glfwSetWindowAspectRatio(window, width, height);

            libqb_log_trace("Window aspect ratio set to %d:%d", width, height);
        } else {
            libqb_log_error("Window not created, cannot set aspect ratio");
        }
    }

    void WindowSetSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight) const {
        if (window) {
            minWidth = (minWidth < 0 ? GLFW_DONT_CARE : ToScreenCoordsX(minWidth));
            minHeight = (minHeight < 0 ? GLFW_DONT_CARE : ToScreenCoordsY(minHeight));
            maxWidth = (maxWidth < 0 ? GLFW_DONT_CARE : ToScreenCoordsX(maxWidth));
            maxHeight = (maxHeight < 0 ? GLFW_DONT_CARE : ToScreenCoordsY(maxHeight));

            glfwSetWindowSizeLimits(window, minWidth, minHeight, maxWidth, maxHeight);

            libqb_log_trace("Window size limits set to (%d, %d) to (%d, %d)", minWidth, minHeight, maxWidth, maxHeight);
        } else {
            libqb_log_error("Window not created, cannot set size limits");
        }
    }

    void WindowSetShouldClose(bool shouldClose) const {
        if (window) {
            glfwSetWindowShouldClose(window, shouldClose);

            libqb_log_trace("Window should close set to %s", shouldClose ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set should close");
        }
    }

    void WindowSwapBuffers() const {
        if (window) {
            glfwSwapBuffers(window);
        } else {
            libqb_log_error("Window not created, cannot swap buffers");
        }
    }

    void WindowRefresh() {
        if (windowRefreshFunction) {
            windowRefreshFunction();
        }
    }

    const void *WindowGetNativeHandle(int32_t type) const {
        if (window) {
            switch (type) {
            case 1:
#if defined(QB64_WINDOWS)
                return reinterpret_cast<const void *>(GetDC(glfwGetWin32Window(window)));
#elif defined(QB64_MACOSX)
                return reinterpret_cast<const void *>(glfwGetCocoaView(window));
#else
                return reinterpret_cast<const void *>(glfwGetX11Display());
#endif
                break;

            default:
#if defined(QB64_WINDOWS)
                return reinterpret_cast<const void *>(glfwGetWin32Window(window));
#elif defined(QB64_MACOSX)
                return reinterpret_cast<const void *>(glfwGetCocoaWindow(window));
#else
                return reinterpret_cast<const void *>(glfwGetX11Window(window));
#endif
            }
        } else {
            libqb_log_error("Window not created, cannot get handle");
        }

        return nullptr;
    }

    void WindowSetCloseFunction(GLUTEmu_CallbackWindowClose function) {
        if (window) {
            windowCloseFunction = function;

            glfwSetWindowCloseCallback(window, [](GLFWwindow *win) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->windowCloseFunction) {
                    instance->windowCloseFunction();
                }
            });

            libqb_log_trace("Window close function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set close function");
        }
    }

    void WindowSetResizedFunction(GLUTEmu_CallbackWindowResized function) {
        if (window) {
            windowResizedFunction = function;

            // We are already listening for window size changes

            libqb_log_trace("Window resize function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set resize function");
        }
    }

    void WindowSetFramebufferResizedFunction(GLUTEmu_CallbackWindowFramebufferResized function) {
        if (window) {
            windowFramebufferResizedFunction = function;

            // We are already listening for framebuffer size changes

            libqb_log_trace("Window framebuffer resize function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set framebuffer resize function");
        }
    }

    void WindowSetMaximizedFunction(GLUTEmu_CallbackWindowMaximized function) {
        if (window) {
            windowMaximizedFunction = function;

            // We are already listening for window maximization changes

            libqb_log_trace("Window maximized function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set maximized function");
        }
    }

    void WindowSetMinimizedFunction(GLUTEmu_CallbackWindowMinimized function) {
        if (window) {
            windowMinimizedFunction = function;

            // We are already listening for window minimization changes

            libqb_log_trace("Window minimized function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set minimized function");
        }
    }

    void WindowSetFocusedFunction(GLUTEmu_CallbackWindowFocused function) {
        if (window) {
            windowFocusedFunction = function;

            // We are already listening for window focus changes

            libqb_log_trace("Window focused function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set focused function");
        }
    }

    void WindowSetRefreshFunction(GLUTEmu_CallbackWindowRefresh function) {
        if (window) {
            windowRefreshFunction = function;

            glfwSetWindowRefreshCallback(window, [](GLFWwindow *win) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->windowRefreshFunction) {
                    instance->windowRefreshFunction();
                }
            });

            libqb_log_trace("Display function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set refresh function");
        }
    }

    void WindowSetIdleFunction(GLUTEmu_CallbackWindowIdle function) {
        if (window) {
            windowIdleFunction = function;

            libqb_log_trace("Idle function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set idle function");
        }
    }

    void KeyboardSetButtonFunction(GLUTEmu_CallbackKeyboardButton function) {
        if (window) {
            keyboardButtonFunction = function;

            // We are already listening for keyboard events

            libqb_log_trace("Keyboard function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set keyboard function");
        }
    }

    void KeyboardSetCharacterFunction(GLUTEmu_CallbackKeyboardCharacter function) {
        if (window) {
            keyboardCharacterFunction = function;

            glfwSetCharCallback(window, [](GLFWwindow *win, unsigned int codepoint) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->keyboardCharacterFunction) {
                    instance->keyboardCharacterFunction(char32_t(codepoint));
                }
            });

            libqb_log_trace("Keyboard char function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set keyboard char function");
        }
    }

    bool KeyboardIsKeyModifierSet(GLUTEmu_KeyboardKeyModifier modifier) const {
        return bool(keyboardModifiers & modifier);
    }

    bool MouseSetStandardCursor(GLUTEmu_MouseStandardCursor style) {
        if (window) {
            if (cursor) {
                glfwDestroyCursor(cursor);
                cursor = nullptr;
                libqb_log_trace("Mouse cursor freed");
            }

            cursor = glfwCreateStandardCursor(int(style));
            if (cursor) {
                glfwSetCursor(window, cursor);

                libqb_log_trace("Mouse cursor set to standard style %d", int(style));

                return true;
            } else {
                libqb_log_error("Failed to set standard cursor of style %d", int(style));
            }
        } else {
            libqb_log_error("Window not created, cannot set mouse cursor");
        }

        return false;
    }

    bool MouseSetCustomCursor(int32_t imageHandle) {
        if (window) {
            // GLFW_TODO: implement custom bitmap cursor support
            libqb_log_warn("MouseSetCustomCursor is not implemented");
            return false;
        } else {
            libqb_log_error("Window not created, cannot set custom mouse cursor");
        }

        return false;
    }

    void MouseSetCursorMode(GLUTEnum_MouseCursorMode mode) {
        if (window) {
            glfwSetInputMode(window, GLFW_CURSOR, int(mode));
            cursorMode = GLUTEnum_MouseCursorMode(glfwGetInputMode(window, GLFW_CURSOR));

            libqb_log_trace("Mouse cursor mode set to %d", int(mode));
        } else {
            libqb_log_error("Window not created, cannot set mouse cursor mode");
        }
    }

    GLUTEnum_MouseCursorMode MouseGetCursorMode() const {
        return cursorMode;
    }

    void MouseMove(double x, double y) {
        if (window) {
            cursorMode = GLUTEnum_MouseCursorMode(glfwGetInputMode(window, GLFW_CURSOR));
            if (cursorMode == GLUTEnum_MouseCursorMode::Disabled) {
                glfwSetCursorPos(window, x, y);
            } else {
                glfwSetCursorPos(window, ToScreenCoordsX(x), ToScreenCoordsY(y));
            }

            libqb_log_trace("Mouse moved to (%f, %f)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move mouse");
        }
    }

    void MouseSetPositionFunction(GLUTEmu_CallbackMousePosition function) {
        if (window) {
            mousePositionFunction = function;

            glfwSetCursorPosCallback(window, [](GLFWwindow *win, double xPos, double yPos) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->mousePositionFunction) {
                    instance->cursorMode = GLUTEnum_MouseCursorMode(glfwGetInputMode(win, GLFW_CURSOR));
                    if (instance->cursorMode != GLUTEnum_MouseCursorMode::Disabled) {
                        xPos = instance->ToPixelCoordsX(xPos);
                        yPos = instance->ToPixelCoordsY(yPos);
                    }
                    instance->mousePositionFunction(xPos, yPos, instance->cursorMode);
                }
            });

            libqb_log_trace("Mouse position function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set mouse position function");
        }
    }

    void MouseSetButtonFunction(GLUTEmu_CallbackMouseButton function) {
        if (window) {
            mouseButtonFunction = function;

            glfwSetMouseButtonCallback(window, [](GLFWwindow *win, int button, int action, int mods) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->mouseButtonFunction) {
                    double xPos, yPos;
                    glfwGetCursorPos(win, &xPos, &yPos);
                    instance->cursorMode = GLUTEnum_MouseCursorMode(glfwGetInputMode(win, GLFW_CURSOR));
                    if (instance->cursorMode != GLUTEnum_MouseCursorMode::Disabled) {
                        xPos = instance->ToPixelCoordsX(xPos);
                        yPos = instance->ToPixelCoordsY(yPos);
                    }
                    instance->mouseButtonFunction(xPos, yPos, GLUTEmu_MouseButton(button), GLUTEmu_ButtonAction(action), instance->cursorMode,
                                                  instance->KeyboardUpdateLockKeyModifier(GLUTEmu_KeyboardKey::ScrollLock, mods));
                }
            });

            libqb_log_trace("Mouse button function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set mouse button function");
        }
    }

    void MouseSetNotifyFunction(GLUTEmu_CallbackMouseNotify function) {
        if (window) {
            mouseNotifyFunction = function;

            glfwSetCursorEnterCallback(window, [](GLFWwindow *win, int entered) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->mouseNotifyFunction) {
                    double x, y;
                    glfwGetCursorPos(win, &x, &y);
                    instance->cursorMode = GLUTEnum_MouseCursorMode(glfwGetInputMode(win, GLFW_CURSOR));
                    if (instance->cursorMode != GLUTEnum_MouseCursorMode::Disabled) {
                        x = instance->ToPixelCoordsX(x);
                        y = instance->ToPixelCoordsY(y);
                    }
                    instance->mouseNotifyFunction(x, y, bool(entered), instance->cursorMode);
                }
            });

            libqb_log_trace("Mouse notify function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set mouse notify function");
        }
    }

    void MouseSetScrollFunction(GLUTEmu_CallbackMouseScroll function) {
        if (window) {
            mouseScrollFunction = function;

            glfwSetScrollCallback(window, [](GLFWwindow *win, double scrollX, double scrollY) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->mouseScrollFunction) {
                    double x, y;
                    glfwGetCursorPos(win, &x, &y);
                    instance->cursorMode = GLUTEnum_MouseCursorMode(glfwGetInputMode(win, GLFW_CURSOR));
                    if (instance->cursorMode != GLUTEnum_MouseCursorMode::Disabled) {
                        x = instance->ToPixelCoordsX(x);
                        y = instance->ToPixelCoordsY(y);
                    }
                    instance->mouseScrollFunction(x, y, scrollX, scrollY, instance->cursorMode);
                }
            });

            libqb_log_trace("Mouse scroll function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set mouse scroll function");
        }
    }

    void DropSetFilesFunction(GLUTEmu_CallbackDropFiles function) {
        if (window) {
            dropFilesFunction = function;

            glfwSetDropCallback(window, [](GLFWwindow *win, int count, const char *paths[]) {
                auto instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->dropFilesFunction) {
                    instance->dropFilesFunction(count, paths);
                }
            });

            libqb_log_trace("Drop files function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set drop files function");
        }
    }

    void MainLoop() {
        libqb_log_trace("Entering main loop");

        if (MessageIsMainThread()) {
            while (!glfwWindowShouldClose(window)) {
                MessageProcess();

                if (windowIdleFunction) {
                    windowIdleFunction();

                    glfwPollEvents();
                } else {
                    glfwWaitEvents();
                }
            }
        } else {
            libqb_log_error("Main loop must be called from the main thread");
        }

        libqb_log_trace("Exiting main loop");
    }

    static GLUTEmu &Instance() {
        static GLUTEmu instance;
        return instance;
    }

    bool MessageIsMainThread() const {
        return std::this_thread::get_id() == mainThreadId;
    }

    void MessageQueue(Message *msg) {
        {
            std::lock_guard guard(msgQueueMutex);
            msgQueue.push(msg);
        }
        glfwPostEmptyEvent();
    }

  private:
    GLUTEmu()
        : monitor(nullptr), window(nullptr), windowX(0), windowY(0), windowWidth(0), windowHeight(0), windowScaleX(1.0f), windowScaleY(1.0f),
          isWindowFullscreen(false), isWindowMaximized(false), isWindowMinimized(false), isWindowFocused(false), isWindowHidden(false), isWindowFloating(false),
          windowOpacity(1.0f), isWindowBordered(true), isWindowMousePassthrough(false), windowedX(0), windowedY(0), windowedWidth(0), windowedHeight(0),
          framebufferWidth(0), framebufferHeight(0), cursor(nullptr), cursorMode(GLUTEnum_MouseCursorMode::Normal), keyboardModifiers(0),
          windowCloseFunction(nullptr), windowResizedFunction(nullptr), windowFramebufferResizedFunction(nullptr), windowMaximizedFunction(nullptr),
          windowMinimizedFunction(nullptr), windowFocusedFunction(nullptr), windowRefreshFunction(nullptr), windowIdleFunction(nullptr),
          keyboardButtonFunction(nullptr), keyboardCharacterFunction(nullptr), mousePositionFunction(nullptr), mouseButtonFunction(nullptr),
          mouseNotifyFunction(nullptr), mouseScrollFunction(nullptr), dropFilesFunction(nullptr) {
        mainThreadId = std::this_thread::get_id();

#if defined(QB64_MACOSX) || defined(QB64_LINUX)
        keyboardScrollLockState = false;
#endif
        // Listen for GLFW error messages and route them to libqb logging
        glfwSetErrorCallback([](int error_code, const char *description) { libqb_log_error("GLFW error %d: %s", error_code, description); });

#ifdef QB64_LINUX
        if (glfwPlatformSupported(GLFW_PLATFORM_X11)) {
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
            libqb_log_trace("Forcing GLFW to use X11 platform");
        } else {
            libqb_log_warn("X11 platform not supported by GLFW, some features may not work correctly");
        }
#endif

        if (glfwInit()) {
            libqb_log_trace("GLFW %s initialized", glfwGetVersionString());
        } else {
            // This will get caught outside because the window creation will fail
            libqb_log_error("Failed to initialize GLFW");
        }
    }

    ~GLUTEmu() {
        if (cursor) {
            glfwDestroyCursor(cursor);
            cursor = nullptr;
            libqb_log_trace("Mouse cursor destroyed");
        }

        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
            libqb_log_trace("Window closed");
        }

        glfwTerminate();
        libqb_log_trace("GLFW terminated");
    }

    GLUTEmu(const GLUTEmu &) = delete;
    GLUTEmu &operator=(const GLUTEmu &) = delete;
    GLUTEmu(GLUTEmu &&) = delete;
    GLUTEmu &operator=(GLUTEmu &&) = delete;

    template <typename T>
    requires std::is_arithmetic_v<T>
    T ToPixelCoordsX(T x) const {
        if constexpr (std::integral<T>) {
            return static_cast<T>(std::round(x * windowScaleX));
        } else {
            return x * windowScaleX;
        }
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    T ToPixelCoordsY(T y) const {
        if constexpr (std::integral<T>) {
            return static_cast<T>(std::round(y * windowScaleY));
        } else {
            return y * windowScaleY;
        }
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    T ToScreenCoordsX(T x) const {
        if constexpr (std::integral<T>) {
            return static_cast<T>(std::round(x / windowScaleX));
        } else {
            return x / windowScaleX;
        }
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    T ToScreenCoordsY(T y) const {
        if constexpr (std::integral<T>) {
            return static_cast<T>(std::round(y / windowScaleY));
        } else {
            return y / windowScaleY;
        }
    }

    GLFWmonitor *WindowGetCurrentMonitor() const {
        GLFWmonitor *best = nullptr;

        if (window) {
            best = glfwGetWindowMonitor(window);
            if (!best) {
                int wx, wy, ww, wh;
                glfwGetWindowPos(window, &wx, &wy);
                glfwGetWindowSize(window, &ww, &wh);

                int count = 0;
                auto monitors = glfwGetMonitors(&count);
                int bestOverlap = 0;

                for (int i = 0; i < count; i++) {
                    int mx, my, mw, mh;
                    glfwGetMonitorPos(monitors[i], &mx, &my);
                    auto mode = glfwGetVideoMode(monitors[i]);
                    mw = mode->width;
                    mh = mode->height;

                    auto overlapW = std::max(0, std::min(wx + ww, mx + mw) - std::max(wx, mx));
                    auto overlapH = std::max(0, std::min(wy + wh, my + mh) - std::max(wy, my));
                    auto overlap = overlapW * overlapH;

                    if (overlap > bestOverlap) {
                        bestOverlap = overlap;
                        best = monitors[i];
                    }
                }
            }
        } else {
            libqb_log_warn("Window not created, using primary monitor");
        }

        return best ? best : glfwGetPrimaryMonitor();
    }

    int KeyboardUpdateLockKeyModifier(GLUTEmu_KeyboardKey key, int mods) {
#if defined(QB64_WINDOWS)
        switch (key) {
        case GLUTEmu_KeyboardKey::ScrollLock:
            mods = (GetKeyState(VK_SCROLL) & 0x0001) ? (mods | GLUTEmu_KeyboardKeyModifier::ScrollLock) : (mods & ~GLUTEmu_KeyboardKeyModifier::ScrollLock);
            break;

        case GLUTEmu_KeyboardKey::CapsLock:
            mods = (GetKeyState(VK_CAPITAL) & 0x0001) ? (mods | GLUTEmu_KeyboardKeyModifier::CapsLock) : (mods & ~GLUTEmu_KeyboardKeyModifier::CapsLock);
            break;

        case GLUTEmu_KeyboardKey::NumLock:
            mods = (GetKeyState(VK_NUMLOCK) & 0x0001) ? (mods | GLUTEmu_KeyboardKeyModifier::NumLock) : (mods & ~GLUTEmu_KeyboardKeyModifier::NumLock);
            break;

        default:
            break;
        }
#elif defined(QB64_LINUX)
        unsigned int n = 0;
        if (XkbGetIndicatorState(glfwGetX11Display(), XkbUseCoreKbd, &n) == Success) {
            switch (key) {
            case GLUTEmu_KeyboardKey::ScrollLock:
                mods = (n & 0x04) ? (mods | GLUTEmu_KeyboardKeyModifier::ScrollLock) : (mods & ~GLUTEmu_KeyboardKeyModifier::ScrollLock);
                break;

            case GLUTEmu_KeyboardKey::CapsLock:
                mods = (n & 0x01) ? (mods | GLUTEmu_KeyboardKeyModifier::CapsLock) : (mods & ~GLUTEmu_KeyboardKeyModifier::CapsLock);
                break;

            case GLUTEmu_KeyboardKey::NumLock:
                mods = (n & 0x02) ? (mods | GLUTEmu_KeyboardKeyModifier::NumLock) : (mods & ~GLUTEmu_KeyboardKeyModifier::NumLock);
                break;

            default:
                break;
            }
        } else // note the else here
#elif defined(QB64_MACOSX) || defined(QB64_LINUX)
        {
            // No indicator API, toggle manually
            if (key == GLUTEmu_KeyboardKey::ScrollLock) {
                // We need this for scroll lock only since GLFW supports caps lock and num lock natively
                mods = keyboardScrollLockState ? (mods | GLUTEmu_KeyboardKeyModifier::ScrollLock) : (mods & ~GLUTEmu_KeyboardKeyModifier::ScrollLock);
            } else {
                libqb_log_warn("Unsupported platform for keyboard lock key modifier retrieval");
            }
        }
#endif

        return mods;
    }

    void MessageProcess() {
        std::queue<Message *> localQueue;
        {
            std::lock_guard guard(msgQueueMutex);
            std::swap(msgQueue, localQueue);
        }

        while (!localQueue.empty()) {
            auto msg = localQueue.front();
            localQueue.pop();
            msg->Execute();
            msg->Finish();
        }
    }

    // GLFW_TODO: we will need to move all of these to an std::vector or similar if we want to support multiple windows in the future
    GLFWmonitor *monitor;                    // current monitor
    GLFWwindow *window;                      // current window
    std::string windowTitle;                 // current window title
    int windowX, windowY;                    // current window position (in pixel coordinates)
    int windowWidth, windowHeight;           // current window size (in pixel coordinates)
    float windowScaleX, windowScaleY;        // window scaling factors
    bool isWindowFullscreen;                 // whether the window is in fullscreen mode
    bool isWindowMaximized;                  // whether the window is currently maximized
    bool isWindowMinimized;                  // whether the window is currently minimized
    bool isWindowFocused;                    // whether the window is currently focused
    bool isWindowHidden;                     // whether the window is currently hidden
    bool isWindowFloating;                   // whether the window is currently floating
    float windowOpacity;                     // current window opacity
    bool isWindowBordered;                   // whether the window is currently bordered
    bool isWindowMousePassthrough;           // whether the window is currently allowing mouse passthrough
    int windowedX, windowedY;                // windowed mode position for restoring from fullscreen (in screen coordinates)
    int windowedWidth, windowedHeight;       // windowed mode size for restoring from fullscreen (in screen coordinates)
    int framebufferWidth, framebufferHeight; // current framebuffer size (in pixel coordinates)
    GLFWcursor *cursor;                      // current mouse cursor
    GLUTEnum_MouseCursorMode cursorMode;     // current mouse cursor mode (normal, hidden, disabled, captured)
    int keyboardModifiers;                   // current keyboard modifiers
#if defined(QB64_MACOSX) || defined(QB64_LINUX)
    bool keyboardScrollLockState; // scroll Lock state for macOS and Linux
#endif
    GLUTEmu_CallbackWindowClose windowCloseFunction;
    GLUTEmu_CallbackWindowResized windowResizedFunction;
    GLUTEmu_CallbackWindowFramebufferResized windowFramebufferResizedFunction;
    GLUTEmu_CallbackWindowMaximized windowMaximizedFunction;
    GLUTEmu_CallbackWindowMinimized windowMinimizedFunction;
    GLUTEmu_CallbackWindowFocused windowFocusedFunction;
    GLUTEmu_CallbackWindowRefresh windowRefreshFunction;
    GLUTEmu_CallbackWindowIdle windowIdleFunction;
    GLUTEmu_CallbackKeyboardButton keyboardButtonFunction;
    GLUTEmu_CallbackKeyboardCharacter keyboardCharacterFunction;
    GLUTEmu_CallbackMousePosition mousePositionFunction;
    GLUTEmu_CallbackMouseButton mouseButtonFunction;
    GLUTEmu_CallbackMouseNotify mouseNotifyFunction;
    GLUTEmu_CallbackMouseScroll mouseScrollFunction;
    GLUTEmu_CallbackDropFiles dropFilesFunction;
    std::thread::id mainThreadId;
    std::mutex msgQueueMutex;
    std::queue<Message *> msgQueue;
};

std::tuple<int, int, int> GLUTEmu_ScreenGetMode() {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        return GLUTEmu::Instance().ScreenGetMode();
    } else {
        GLUTEmu::MessageScreenGetMode msg;
        GLUTEmu::Instance().MessageQueue(&msg);
        msg.WaitForResponse();
        return msg.responseValue;
    }
}

template <typename T> void GLUTEmu_WindowSetHint(GLUTEmu_WindowHint hint, const T value) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetHint<T>(hint, value);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowSetHint<T>(hint, value));
    }
}

// We only need to instantiate the template for the types we need
template void GLUTEmu_WindowSetHint<bool>(GLUTEmu_WindowHint, bool);
template void GLUTEmu_WindowSetHint<int>(GLUTEmu_WindowHint, int);
template void GLUTEmu_WindowSetHint<char *>(GLUTEmu_WindowHint, char *);
template void GLUTEmu_WindowSetHint<const char *>(GLUTEmu_WindowHint, const char *);

bool GLUTEmu_WindowCreate(const char *title, int width, int height) {
    return GLUTEmu::Instance().WindowCreate(title, width, height);
}

bool GLUTEmu_WindowIsCreated() {
    return GLUTEmu::Instance().WindowIsCreated();
}

void GLUTEmu_WindowSetTitle(const char *title) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetTitle(title);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowSetTitle(title));
    }
}

std::string_view GLUTEmu_WindowGetTitle() {
    return GLUTEmu::Instance().WindowGetTitle();
}

void GLUTEmu_WindowSetIcon(int32_t imageHandle) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetIcon(imageHandle);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowSetIcon(imageHandle));
    }
}

void GLUTEmu_WindowFullScreen(bool fullscreen) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowFullscreen(fullscreen);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowFullscreen(fullscreen));
    }
}

bool GLUTEmu_WindowIsFullscreen() {
    return GLUTEmu::Instance().WindowIsFullscreen();
}

void GLUTEmu_WindowMaximize() {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowMaximize();
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowMaximize());
    }
}

bool GLUTEmu_WindowIsMaximized() {
    return GLUTEmu::Instance().WindowIsMaximized();
}

void GLUTEmu_WindowMinimize() {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowMinimize();
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowMinimize());
    }
}

bool GLUTEmu_WindowIsMinimized() {
    return GLUTEmu::Instance().WindowIsMinimized();
}

void GLUTEmu_WindowRestore() {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowRestore();
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowRestore());
    }
}

bool GLUTEmu_WindowIsRestored() {
    return GLUTEmu::Instance().WindowIsRestored();
}

void GLUTEmu_WindowHide(bool hide) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowHide(hide);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowHide(hide));
    }
}

bool GLUTEmu_WindowIsHidden() {
    return GLUTEmu::Instance().WindowIsHidden();
}

void GLUTEmu_WindowFocus() {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowFocus();
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowFocus());
    }
}

bool GLUTEmu_WindowIsFocused() {
    return GLUTEmu::Instance().WindowIsFocused();
}

void GLUTEmu_WindowSetFloating(bool floating) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetFloating(floating);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowSetFloating(floating));
    }
}

bool GLUTEmu_WindowIsFloating() {
    return GLUTEmu::Instance().WindowIsFloating();
}

void GLUTEmu_WindowSetOpacity(float opacity) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetOpacity(opacity);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowSetOpacity(opacity));
    }
}

float GLUTEmu_WindowGetOpacity() {
    return GLUTEmu::Instance().WindowGetOpacity();
}

void GLUTEmu_WindowSetBordered(bool bordered) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetBordered(bordered);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowSetBordered(bordered));
    }
}

bool GLUTEmu_WindowIsBordered() {
    return GLUTEmu::Instance().WindowIsBordered();
}

void GLUTEmu_WindowSetMousePassthrough(bool passthrough) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetMousePassthrough(passthrough);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowSetMousePassthrough(passthrough));
    }
}

bool GLUTEmu_WindowAllowsMousePassthrough() {
    return GLUTEmu::Instance().WindowAllowsMousePassthrough();
}

void GLUTEmu_WindowResize(int width, int height) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowResize(width, height);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowResize(width, height));
    }
}

std::pair<int, int> GLUTEmu_WindowGetSize() {
    return GLUTEmu::Instance().WindowGetSize();
}

std::pair<int, int> GLUTEmu_WindowGetFramebufferSize() {
    return GLUTEmu::Instance().WindowGetFramebufferSize();
}

void GLUTEmu_WindowMove(int x, int y) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowMove(x, y);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowMove(x, y));
    }
}

std::pair<int, int> GLUTEmu_WindowGetPosition() {
    return GLUTEmu::Instance().WindowGetPosition();
}

void GLUTEmu_WindowCenter() {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowCenter();
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowCenter());
    }
}

void GLUTEmu_WindowSetAspectRatio(int width, int height) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetAspectRatio(width, height);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageSetWindowAspectRatio(width, height));
    }
}

void GLUTEmu_WindowSetSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetSizeLimits(minWidth, minHeight, maxWidth, maxHeight);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageSetWindowSizeLimits(minWidth, minHeight, maxWidth, maxHeight));
    }
}

void GLUTEmu_WindowSetShouldClose(bool shouldClose) {
    GLUTEmu::Instance().WindowSetShouldClose(shouldClose);
}

void GLUTEmu_WindowSwapBuffers() {
    GLUTEmu::Instance().WindowSwapBuffers();
}

void GLUTEmu_WindowRefresh() {
    GLUTEmu::Instance().WindowRefresh();
}

const void *GLUTEmu_WindowGetNativeHandle(int32_t type) {
    return GLUTEmu::Instance().WindowGetNativeHandle(type);
}

void GLUTEmu_WindowSetCloseFunction(GLUTEmu_CallbackWindowClose func) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetCloseFunction(func);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowSetCloseFunction(func));
    }
}

void GLUTEmu_WindowSetResizedFunction(GLUTEmu_CallbackWindowResized func) {
    GLUTEmu::Instance().WindowSetResizedFunction(func);
}

void GLUTEmu_WindowSetFramebufferResizedFunction(GLUTEmu_CallbackWindowFramebufferResized func) {
    GLUTEmu::Instance().WindowSetFramebufferResizedFunction(func);
}

void GLUTEmu_WindowSetMaximizedFunction(GLUTEmu_CallbackWindowMaximized func) {
    GLUTEmu::Instance().WindowSetMaximizedFunction(func);
}

void GLUTEmu_WindowSetMinimizedFunction(GLUTEmu_CallbackWindowMinimized func) {
    GLUTEmu::Instance().WindowSetMinimizedFunction(func);
}

void GLUTEmu_WindowSetFocusedFunction(GLUTEmu_CallbackWindowFocused func) {
    GLUTEmu::Instance().WindowSetFocusedFunction(func);
}

void GLUTEmu_WindowSetRefreshFunction(GLUTEmu_CallbackWindowRefresh func) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetRefreshFunction(func);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowSetRefreshFunction(func));
    }
}

void GLUTEmu_WindowSetIdleFunction(GLUTEmu_CallbackWindowIdle func) {
    GLUTEmu::Instance().WindowSetIdleFunction(func);
}

void GLUTEmu_KeyboardSetButtonFunction(GLUTEmu_CallbackKeyboardButton func) {
    GLUTEmu::Instance().KeyboardSetButtonFunction(func);
}

void GLUTEmu_KeyboardSetCharacterFunction(GLUTEmu_CallbackKeyboardCharacter func) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().KeyboardSetCharacterFunction(func);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageKeyboardSetCharacterFunction(func));
    }
}

bool GLUTEmu_KeyboardIsKeyModifierSet(GLUTEmu_KeyboardKeyModifier modifier) {
    return GLUTEmu::Instance().KeyboardIsKeyModifierSet(modifier);
}

bool GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor style) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        return GLUTEmu::Instance().MouseSetStandardCursor(style);
    } else {
        GLUTEmu::MessageSetStandardCursor msg(style);
        GLUTEmu::Instance().MessageQueue(&msg);
        msg.WaitForResponse();
        return msg.responseValue;
    }
}

bool GLUTEmu_MouseSetCustomCursor(int32_t imageHandle) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        return GLUTEmu::Instance().MouseSetCustomCursor(imageHandle);
    } else {
        GLUTEmu::MessageSetCustomCursor msg(imageHandle);
        GLUTEmu::Instance().MessageQueue(&msg);
        msg.WaitForResponse();
        return msg.responseValue;
    }
}

void GLUTEmu_MouseSetCursorMode(GLUTEnum_MouseCursorMode mode) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().MouseSetCursorMode(mode);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageSetCursorMode(mode));
    }
}

GLUTEnum_MouseCursorMode GLUTEmu_MouseGetCursorMode() {
    return GLUTEmu::Instance().MouseGetCursorMode();
}

void GLUTEmu_MouseMove(double x, double y) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().MouseMove(x, y);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageMouseMove(x, y));
    }
}

void GLUTEmu_MouseSetPositionFunction(GLUTEmu_CallbackMousePosition func) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().MouseSetPositionFunction(func);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageMouseSetPositionFunction(func));
    }
}

void GLUTEmu_MouseSetButtonFunction(GLUTEmu_CallbackMouseButton func) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().MouseSetButtonFunction(func);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageMouseSetButtonFunction(func));
    }
}

void GLUTEmu_MouseSetNotifyFunction(GLUTEmu_CallbackMouseNotify func) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().MouseSetNotifyFunction(func);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageMouseSetNotifyFunction(func));
    }
}

void GLUTEmu_MouseSetScrollFunction(GLUTEmu_CallbackMouseScroll func) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().MouseSetScrollFunction(func);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageMouseSetScrollFunction(func));
    }
}

void GLUTEmu_DropSetFilesFunction(GLUTEmu_CallbackDropFiles func) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().DropSetFilesFunction(func);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageDropSetFilesFunction(func));
    }
}

void GLUTEmu_MainLoop() {
    GLUTEmu::Instance().MainLoop();
}

void GLUTEmu_ProgramExit(int exitCode) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        libqb_log_trace("Program exiting with code %d", exitCode);
        GLUTEmu::Instance().WindowSetCloseFunction(nullptr);
        GLUTEmu::Instance().WindowSetShouldClose(true);
        exit(exitCode);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageProgramExit(exitCode));
    }
}
