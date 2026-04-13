//--------------------------------------------------------------------------------------------------------
// QB64-PE GLUT-like emulation layer
// This abstracts the underlying windowing library and provides a GLUT-like API for QB64-PE
//
// TODO:
// Custom bitmap cursor support
// Desktop image capture support
// Mouse capture/release support
//--------------------------------------------------------------------------------------------------------

#include "libqb-common.h"

#include "completion.h"
#include "graphics.h"
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
#include <atomic>
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
            if (finished != nullptr) {
                completion_finish(finished);
            } else {
                delete this;
            }
        }

        void WaitForResponse() {
            completion_wait(finished);
        }

        virtual ~Message() {
            if (finished != nullptr) {
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

    class MessageWindowSetTitle : public Message {
      public:
        std::string newTitle;

        MessageWindowSetTitle(std::string_view title) : Message(true), newTitle(title) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetTitle(newTitle);
        }
    };

    class MessageWindowSetIcon : public Message {
      public:
        int32_t largeImageHandle;
        int32_t smallImageHandle;

        MessageWindowSetIcon(int32_t largeImageHandle, int32_t smallImageHandle)
            : Message(false), largeImageHandle(largeImageHandle), smallImageHandle(smallImageHandle) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetIcon(largeImageHandle, smallImageHandle);
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

    class MessageSetWindowMinimumSizeLimits : public Message {
      public:
        int minWidth, minHeight;

        MessageSetWindowMinimumSizeLimits(int minWidth, int minHeight) : Message(false), minWidth(minWidth), minHeight(minHeight) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetMinimumSizeLimits(minWidth, minHeight);
        }
    };

    class MessageSetWindowMaximumSizeLimits : public Message {
      public:
        int maxWidth, maxHeight;

        MessageSetWindowMaximumSizeLimits(int maxWidth, int maxHeight) : Message(false), maxWidth(maxWidth), maxHeight(maxHeight) {}

        void Execute() override {
            GLUTEmu::Instance().WindowSetMaximumSizeLimits(maxWidth, maxHeight);
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
        return screenMode;
    }

    template <typename T> void WindowSetHint(GLUTEmu_WindowHint hint, T value) const {
        if (std::this_thread::get_id() == mainThreadId) {
            if constexpr (std::is_same_v<T, int> || std::is_same_v<T, unsigned int>) {
                glfwWindowHint(static_cast<int>(hint), value);
                libqb_log_trace("Window hint set: %d = %d", static_cast<int>(hint), value);
            } else if constexpr (std::is_same_v<T, bool>) {
                glfwWindowHint(static_cast<int>(hint), value ? GLFW_TRUE : GLFW_FALSE);
                libqb_log_trace("Window hint set: %d = %s", static_cast<int>(hint), value ? "true" : "false");
            } else if constexpr (std::is_same_v<T, const char *> || std::is_same_v<T, char *>) {
                glfwWindowHintString(static_cast<int>(hint), value);
                libqb_log_trace("Window hint set: %d = '%s'", static_cast<int>(hint), value);
            } else if constexpr (std::is_same_v<T, GLUTEmu_WindowHintValue>) {
                glfwWindowHint(static_cast<int>(hint), static_cast<int>(value));
                libqb_log_trace("Window hint set: %d = %d", static_cast<int>(hint), static_cast<int>(value));
            } else {
                static_assert(!sizeof(T), "Unsupported type");
            }
        } else {
            libqb_log_error("Window hints must be set from the main thread");
        }
    }

    [[nodiscard]] bool WindowCreate(int width, int height) {
        if (window != nullptr) {
            // GLFW_TODO: sure we can; maybe we'll use it in a future version of QB64-PE
            libqb_log_error("Window already created, cannot create another window");
        } else {
            if (std::this_thread::get_id() == mainThreadId) {
                // GLFW creates the window using screen coordinates, so we need to fix it below
                window = glfwCreateWindow(width, height, windowTitle.empty() ? "Untitled" : windowTitle.c_str(), nullptr, nullptr);
                if (window != nullptr) {
                    glfwSetWindowUserPointer(window, this);
                    glfwMakeContextCurrent(window);

                    auto version = gladLoadGL(glfwGetProcAddress);
                    if (version == 0) {
                        libqb_log_error("Failed to initialize OpenGL context");
                        glfwDestroyWindow(window);
                        window = nullptr;
                        return false;
                    }

                    libqb_log_trace("GLAD %s initialized", GLAD_GENERATOR_VERSION);
                    libqb_log_trace("GLAD loaded with OpenGL %d.%d", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
                    libqb_log_trace("OpenGL renderer: %s", glGetString(GL_RENDERER));
                    libqb_log_trace("OpenGL vendor: %s", glGetString(GL_VENDOR));
                    libqb_log_trace("OpenGL version: %s", glGetString(GL_VERSION));
                    libqb_log_trace("OpenGL shading language version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

                    glfwSwapInterval(1);

                    // Get the current window content scale and set a callback to track changes
                    glfwGetWindowContentScale(window, &windowScaleX, &windowScaleY);
                    glfwSetWindowContentScaleCallback(window, [](GLFWwindow *win, float xScale, float yScale) {
                        auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->windowScaleX = xScale;
                        instance->windowScaleY = yScale;
                        instance->monitor = instance->WindowGetCurrentMonitorInfo();

                        libqb_log_trace("Window content scale changed to (%fx%f)", xScale, yScale);
                    });

                    // Get the window size and scale to actual pixel size and set a callback to track changes
                    glfwGetWindowSize(window, &windowWidth, &windowHeight);
                    windowWidth = ToPixelCoordsX(windowWidth);
                    windowHeight = ToPixelCoordsY(windowHeight);
                    glfwSetWindowSizeCallback(window, [](GLFWwindow *win, int width, int height) {
                        auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->windowWidth = instance->ToPixelCoordsX(width);
                        instance->windowHeight = instance->ToPixelCoordsY(height);

                        libqb_log_trace("Window resized to (%d x %d)", instance->windowWidth, instance->windowHeight);

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
                    monitor = WindowGetCurrentMonitorInfo();
                    windowX = ToPixelDesktopCoordsX(windowX);
                    windowY = ToPixelDesktopCoordsY(windowY);
                    glfwSetWindowPosCallback(window, [](GLFWwindow *win, int x, int y) {
                        auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->windowX = instance->ToPixelDesktopCoordsX(x);
                        instance->windowY = instance->ToPixelDesktopCoordsY(y);
                        instance->monitor = instance->WindowGetCurrentMonitorInfo();

                        libqb_log_trace("Window moved to (%d, %d)", instance->windowX, instance->windowY);
                    });

                    // Get the framebuffer size (already in pixels) and set a callback to track changes
                    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *win, int width, int height) {
                        auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->framebufferWidth = width;
                        instance->framebufferHeight = height;

                        libqb_log_trace("Window framebuffer resized to (%d x %d)", width, height);

                        if (instance->windowFramebufferResizedFunction) {
                            instance->windowFramebufferResizedFunction(instance->framebufferWidth, instance->framebufferHeight);
                        }
                    });

                    // Set a hook into the maximization callback to track restore events
                    glfwSetWindowMaximizeCallback(window, [](GLFWwindow *win, int maximized) {
                        auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        glfwGetWindowSize(instance->window, &instance->windowWidth, &instance->windowHeight);
                        instance->windowWidth = instance->ToPixelCoordsX(instance->windowWidth);
                        instance->windowHeight = instance->ToPixelCoordsY(instance->windowHeight);
                        instance->isWindowMaximized = (maximized == GLFW_TRUE);

                        libqb_log_trace("Window %s", maximized ? "maximized" : "restored");

                        if (instance->windowMaximizedFunction) {
                            instance->windowMaximizedFunction(instance->windowWidth, instance->windowHeight, (maximized == GLFW_TRUE));
                        }
                    });

                    // Set a hook into the minimization callback to track restore events
                    glfwSetWindowIconifyCallback(window, [](GLFWwindow *win, int iconified) {
                        auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->isWindowMinimized = (iconified == GLFW_TRUE);

                        libqb_log_trace("Window %s", iconified == GLFW_TRUE ? "minimized" : "restored");

                        if (instance->windowMinimizedFunction) {
                            if (iconified == GLFW_TRUE) {
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
                    isWindowFocused = (glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE);
                    glfwSetWindowFocusCallback(window, [](GLFWwindow *win, int focused) {
                        auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                        instance->isWindowFocused = (focused == GLFW_TRUE);

                        libqb_log_trace("Window %s", focused == GLFW_TRUE ? "focused" : "unfocused");

                        if (instance->windowFocusedFunction) {
                            instance->windowFocusedFunction(focused == GLFW_TRUE);
                        }
                    });

                    // Set a hook into the keyboard button callback to enable some backward compatibility
                    glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
                    keyboardModifiers = KeyboardUpdateLockKeyModifier(GLUTEmu_KeyboardKey::CapsLock, keyboardModifiers);
                    keyboardModifiers = KeyboardUpdateLockKeyModifier(GLUTEmu_KeyboardKey::NumLock, keyboardModifiers);
                    keyboardModifiers = KeyboardUpdateLockKeyModifier(GLUTEmu_KeyboardKey::ScrollLock, keyboardModifiers);
                    glfwSetKeyCallback(window, [](GLFWwindow *win, int key, int scancode, int action, int mods) {
                        auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
#if defined(QB64_MACOSX) || defined(QB64_LINUX)
                        if (key == static_cast<int>(GLUTEmu_KeyboardKey::ScrollLock) && action == GLFW_RELEASE) {
                            instance->keyboardScrollLockState = !instance->keyboardScrollLockState;
                        }
#endif
                        instance->keyboardModifiers = instance->KeyboardUpdateLockKeyModifier(GLUTEmu_KeyboardKey::ScrollLock, mods);
                        if (instance->keyboardButtonFunction) {
                            instance->keyboardButtonFunction(GLUTEmu_KeyboardKey(key), scancode, GLUTEmu_ButtonAction(action), instance->keyboardModifiers);
                        }
                    });

                    if (glfwRawMouseMotionSupported() == GLFW_TRUE) {
                        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                        libqb_log_trace("Raw mouse motion supported and enabled");
                    } else {
                        // GLFW_TODO: Currently this fails only on macOS. So, we'll need some kind of fallback (GCMouseInput?)
                        libqb_log_warn("Raw mouse motion not supported");
                    }

                    // Refresh state variables
                    isWindowFullscreen = (glfwGetWindowMonitor(window) != nullptr);
                    isWindowMaximized = (glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE);
                    isWindowMinimized = (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE);
                    isWindowFocused = (glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE);
                    isWindowHidden = (glfwGetWindowAttrib(window, GLFW_VISIBLE) == GLFW_FALSE);
                    isWindowFloating = (glfwGetWindowAttrib(window, GLFW_FLOATING) == GLFW_TRUE);
                    isWindowBordered = (glfwGetWindowAttrib(window, GLFW_DECORATED) == GLFW_TRUE);
                    isWindowMousePassthrough = (glfwGetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH) == GLFW_TRUE);
                    windowOpacity = glfwGetWindowOpacity(window);

                    libqb_log_trace("Window created (%u x %u)", width, height);

                    return true;
                } else {
                    libqb_log_error("Failed to create window (%dx%d)", width, height);
                }
            } else {
                libqb_log_error("Window must be created from the main thread");
            }
        }

        return false;
    }

    [[nodiscard]] bool WindowIsCreated() const {
        return window != nullptr;
    }

    void WindowSetTitle(std::string_view title) {
        windowTitle = title;
        if (window != nullptr) {
            glfwSetWindowTitle(window, windowTitle.c_str());
            windowTitle = glfwGetWindowTitle(window);
        } else {
            libqb_log_warn("Window not created, cannot set title");
        }
    }

    [[nodiscard]] std::string_view WindowGetTitle() const {
        return {windowTitle.data(), windowTitle.size()};
    }

    void WindowSetIcon(int32_t largeImageHandle, int32_t smallImageHandle) {
        if (window != nullptr) {
            if (!Image_IsHandleValid(largeImageHandle) && !Image_IsHandleValid(smallImageHandle)) {
                libqb_log_warn("Neither large nor small icon handle is valid, attempting to load resource icons");

                if (WindowLoadResourceIconNative()) {
                    libqb_log_trace("Resource icons loaded successfully");
                } else {
                    libqb_log_warn("Failed to load resource icons, using default QB64-PE icons");

                    GLFWimage image[2];
                    image[0].pixels = const_cast<uint8_t *>(Icon32bppRGBA32x32);
                    image[0].width = 32;
                    image[0].height = 32;
                    image[1].pixels = const_cast<uint8_t *>(Icon32bppRGBA16x16);
                    image[1].width = 16;
                    image[1].height = 16;

                    glfwSetWindowIcon(window, 2, image);
                }

                return;
            }

            if (!Image_IsHandleValid(largeImageHandle) && Image_IsHandleValid(smallImageHandle)) {
                largeImageHandle = smallImageHandle;
            }

            if (!Image_IsHandleValid(smallImageHandle) && Image_IsHandleValid(largeImageHandle)) {
                smallImageHandle = largeImageHandle;
            }

            auto imgSmall = Image_GetDescriptor(smallImageHandle);
            auto imgLarge = Image_GetDescriptor(largeImageHandle);

            if (imgLarge != nullptr && imgSmall != nullptr) {
                if (imgLarge->text || imgSmall->text || imgLarge->width <= 0 || imgLarge->height <= 0 || imgSmall->width <= 0 || imgSmall->height <= 0) {
                    libqb_log_error("Invalid image handles provided for window icon (large handle: %d, small handle: %d)", largeImageHandle, smallImageHandle);
                    error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                    return;
                }

                libqb_log_trace("Setting window icons (large handle: %d, small handle: %d)", largeImageHandle, smallImageHandle);

                uint8_t *imgLargeData = nullptr;
                uint8_t *imgSmallData = nullptr;
                std::vector<uint32_t> imgLargeConvertedData(0);
                std::vector<uint32_t> imgSmallConvertedData(0);

                // Ensure both icons are in 32bpp RGBA format, converting if necessary
                auto convertTo32bpp = [](img_struct *img, std::vector<uint32_t> &converted) -> uint8_t * {
                    if (img->bits_per_pixel != 32) {
                        libqb_log_trace("Converting large icon to 32bpp RGBA format");

                        converted.resize(img->width * img->height);

                        auto src = img->offset;
                        for (size_t i = 0; i < converted.size(); i++) {
                            converted[i] = image_swap_red_blue(img->pal[*src]);
                            ++src;
                        }

                        return reinterpret_cast<uint8_t *>(converted.data());
                    } else {
                        return reinterpret_cast<uint8_t *>(img->offset32);
                    }
                };

                imgLargeData = convertTo32bpp(imgLarge, imgLargeConvertedData);
                imgSmallData = convertTo32bpp(imgSmall, imgSmallConvertedData);

                GLFWimage images[2];
                images[0].pixels = imgLargeData;
                images[0].width = imgLarge->width;
                images[0].height = imgLarge->height;
                images[1].pixels = imgSmallData;
                images[1].width = imgSmall->width;
                images[1].height = imgSmall->height;

                glfwSetWindowIcon(window, 2, images);
            }
        } else {
            libqb_log_error("Window not created, cannot set icon");
        }
    }

    void WindowFullscreen(bool fullscreen) {
        if (window != nullptr) {
            monitor = WindowGetCurrentMonitorInfo();
            if (monitor != nullptr) {
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
                        const auto *mode = glfwGetVideoMode(monitor);
                        if (mode == nullptr) {
                            libqb_log_error("Failed to query monitor video mode");
                            return;
                        }
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
                libqb_log_error("No monitor available");
            }
        } else {
            libqb_log_error("Window not created, cannot set fullscreen");
        }
    }

    [[nodiscard]] bool WindowIsFullscreen() const {
        return isWindowFullscreen;
    }

    void WindowMaximize() {
        if (window != nullptr) {
            glfwMaximizeWindow(window);
            isWindowMaximized = (glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE);

            libqb_log_trace("Window maximized");
        } else {
            libqb_log_error("Window not created, cannot maximize");
        }
    }

    [[nodiscard]] bool WindowIsMaximized() const {
        return isWindowMaximized;
    }

    void WindowMinimize() {
        if (window != nullptr) {
            glfwIconifyWindow(window);
            isWindowMinimized = (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE);

            libqb_log_trace("Window minimized");
        } else {
            libqb_log_error("Window not created, cannot minimize");
        }
    }

    [[nodiscard]] bool WindowIsMinimized() const {
        return isWindowMinimized;
    }

    void WindowRestore() {
        if (window != nullptr) {
            glfwRestoreWindow(window);
            isWindowMaximized = (glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE);
            isWindowMinimized = (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE);

            libqb_log_trace("Window restored");
        } else {
            libqb_log_error("Window not created, cannot restore");
        }
    }

    [[nodiscard]] bool WindowIsRestored() const {
        return !isWindowMaximized && !isWindowMinimized;
    }

    void WindowHide(bool hide) {
        if (window != nullptr) {
            if (hide) {
                glfwHideWindow(window);
            } else {
                glfwShowWindow(window);
            }

            isWindowHidden = (glfwGetWindowAttrib(window, GLFW_VISIBLE) == GLFW_FALSE);

            libqb_log_trace("Window %s", isWindowHidden ? "hidden" : "shown");
        } else {
            libqb_log_error("Window not created, cannot hide");
        }
    }

    [[nodiscard]] bool WindowIsHidden() const {
        if (window != nullptr) {
            return isWindowHidden;
        } else {
            libqb_log_error("Window not created, cannot check visibility");
        }

        return false;
    }

    void WindowFocus() {
        if (window != nullptr) {
            glfwFocusWindow(window);
            isWindowFocused = (glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE);

            libqb_log_trace("Window focused");
        } else {
            libqb_log_error("Window not created, cannot focus");
        }
    }

    [[nodiscard]] bool WindowIsFocused() const {
        return isWindowFocused;
    }

    void WindowSetFloating(bool floating) {
        if (window != nullptr) {
            glfwSetWindowAttrib(window, GLFW_FLOATING, floating ? GLFW_TRUE : GLFW_FALSE);
            isWindowFloating = (glfwGetWindowAttrib(window, GLFW_FLOATING) == GLFW_TRUE);

            libqb_log_trace("Window floating state set to %s", isWindowFloating ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set floating state");
        }
    }

    [[nodiscard]] bool WindowIsFloating() const {
        return isWindowFloating;
    }

    void WindowSetOpacity(float opacity) {
        if (window != nullptr) {
            glfwSetWindowOpacity(window, opacity);
            windowOpacity = glfwGetWindowOpacity(window);

            libqb_log_trace("Window opacity set to %f", windowOpacity);
        } else {
            libqb_log_error("Window not created, cannot set opacity");
        }
    }

    [[nodiscard]] float WindowGetOpacity() const {
        return windowOpacity;
    }

    void WindowSetBordered(bool bordered) {
        if (window != nullptr) {
            glfwSetWindowAttrib(window, GLFW_DECORATED, bordered ? GLFW_TRUE : GLFW_FALSE);
            isWindowBordered = (glfwGetWindowAttrib(window, GLFW_DECORATED) == GLFW_TRUE);

            libqb_log_trace("Window border state set to %s", isWindowBordered ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set border state");
        }
    }

    [[nodiscard]] bool WindowIsBordered() const {
        return isWindowBordered;
    }

    void WindowSetMousePassthrough(bool passthrough) {
        if (window != nullptr) {
            glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, passthrough ? GLFW_TRUE : GLFW_FALSE);
            isWindowMousePassthrough = (glfwGetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH) == GLFW_TRUE);

            libqb_log_trace("Window mouse passthrough set to %s", isWindowMousePassthrough ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set mouse passthrough");
        }
    }

    [[nodiscard]] bool WindowAllowsMousePassthrough() const {
        return isWindowMousePassthrough;
    }

    void WindowResize(int width, int height) {
        if (window != nullptr) {
            glfwSetWindowSize(window, ToScreenCoordsX(width), ToScreenCoordsY(height));

            libqb_log_trace("Window resized to (%d x %d)", width, height);
        } else {
            libqb_log_error("Window not created, cannot resize");
        }
    }

    [[nodiscard]] std::pair<int, int> WindowGetSize() const {
        return {windowWidth, windowHeight};
    }

    [[nodiscard]] std::pair<int, int> WindowGetFramebufferSize() const {
        return {framebufferWidth, framebufferHeight};
    }

    void WindowMove(int x, int y) {
        if (window != nullptr) {
            glfwSetWindowPos(window, ToScreenDesktopCoordsX(x), ToScreenDesktopCoordsY(y));

            libqb_log_trace("Window moved to (%d, %d)", x, y);
        } else {
            libqb_log_error("Window not created, cannot move");
        }
    }

    [[nodiscard]] std::pair<int, int> WindowGetPosition() const {
        return {windowX, windowY};
    }

    void WindowCenter() {
        if ((monitor != nullptr) && (window != nullptr) && !isWindowFullscreen && !isWindowMaximized && !isWindowMinimized && !isWindowHidden) {
            int mx, my;
            glfwGetMonitorPos(monitor, &mx, &my);

            int mw, mh;
            glfwGetMonitorWorkarea(monitor, nullptr, nullptr, &mw, &mh);

            int ww, wh;
            glfwGetWindowSize(window, &ww, &wh);

            auto x = mx + ((mw - ww) / 2);
            auto y = my + ((mh - wh) / 2);
            glfwSetWindowPos(window, x, y);

            libqb_log_trace("Window centered");
        } else {
            libqb_log_error("Window not created, cannot center");
        }
    }

    void WindowSetAspectRatio(int width, int height) {
        if (window != nullptr) {
            width = (width < 0 ? GLFW_DONT_CARE : width);
            height = (height < 0 ? GLFW_DONT_CARE : height);
            glfwSetWindowAspectRatio(window, width, height);

            libqb_log_trace("Window aspect ratio set to %d:%d", width, height);
        } else {
            libqb_log_error("Window not created, cannot set aspect ratio");
        }
    }

    void WindowSetSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight) {
        windowMinWidthLimit = minWidth;
        windowMinHeightLimit = minHeight;
        windowMaxWidthLimit = maxWidth;
        windowMaxHeightLimit = maxHeight;

        if (window != nullptr) {
            const auto minWidthPixels = (windowMinWidthLimit < 0 ? GLFW_DONT_CARE : ToScreenCoordsX(windowMinWidthLimit));
            const auto minHeightPixels = (windowMinHeightLimit < 0 ? GLFW_DONT_CARE : ToScreenCoordsY(windowMinHeightLimit));
            const auto maxWidthPixels = (windowMaxWidthLimit < 0 ? GLFW_DONT_CARE : ToScreenCoordsX(windowMaxWidthLimit));
            const auto maxHeightPixels = (windowMaxHeightLimit < 0 ? GLFW_DONT_CARE : ToScreenCoordsY(windowMaxHeightLimit));

            glfwSetWindowSizeLimits(window, minWidthPixels, minHeightPixels, maxWidthPixels, maxHeightPixels);

            libqb_log_trace("Window size limits set to (%d, %d) to (%d, %d)", minWidthPixels, minHeightPixels, maxWidthPixels, maxHeightPixels);
        } else {
            libqb_log_trace("Window not created; cached size limits (%d, %d) to (%d, %d)", windowMinWidthLimit, windowMinHeightLimit, windowMaxWidthLimit,
                            windowMaxHeightLimit);
        }
    }

    void WindowSetMinimumSizeLimits(int minWidth, int minHeight) {
        WindowSetSizeLimits(minWidth, minHeight, windowMaxWidthLimit, windowMaxHeightLimit);
    }

    void WindowSetMaximumSizeLimits(int maxWidth, int maxHeight) {
        WindowSetSizeLimits(windowMinWidthLimit, windowMinHeightLimit, maxWidth, maxHeight);
    }

    void WindowSetShouldClose(bool shouldClose) const {
        if (window != nullptr) {
            glfwSetWindowShouldClose(window, static_cast<int>(shouldClose));

            libqb_log_trace("Window should close set to %s", shouldClose ? "true" : "false");
        } else {
            libqb_log_error("Window not created, cannot set should close");
        }
    }

    void WindowSwapBuffers() const {
        if (window != nullptr) {
            glfwSwapBuffers(window);
        } else {
            libqb_log_error("Window not created, cannot swap buffers");
        }
    }

    void WindowRefresh() {
        // We will avoid refreshing the window if it is hidden
        // GLFW_TODO: check if not calling the refresh callback when the window is hidden causes any issues
        if (windowRefreshFunction != nullptr && !isWindowHidden) {
            windowRefreshFunction();
        }
    }

    [[nodiscard]] const void *WindowGetNativeHandle(int32_t type) const {
        if (window != nullptr) {
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
        if (window != nullptr) {
            windowCloseFunction = function;

            glfwSetWindowCloseCallback(window, [](GLFWwindow *win) {
                auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
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
        if (window != nullptr) {
            windowResizedFunction = function;

            // We are already listening for window size changes, but send the new size to the callback immediately
            if (windowResizedFunction != nullptr) {
                windowResizedFunction(windowWidth, windowHeight);
            }

            libqb_log_trace("Window resize function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set resize function");
        }
    }

    void WindowSetFramebufferResizedFunction(GLUTEmu_CallbackWindowFramebufferResized function) {
        if (window != nullptr) {
            windowFramebufferResizedFunction = function;

            // We are already listening for framebuffer size changes, but send the new size to the callback immediately
            if (windowFramebufferResizedFunction != nullptr) {
                windowFramebufferResizedFunction(framebufferWidth, framebufferHeight);
            }

            libqb_log_trace("Window framebuffer resize function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set framebuffer resize function");
        }
    }

    void WindowSetMaximizedFunction(GLUTEmu_CallbackWindowMaximized function) {
        if (window != nullptr) {
            windowMaximizedFunction = function;

            // We are already listening for window maximization changes

            libqb_log_trace("Window maximized function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set maximized function");
        }
    }

    void WindowSetMinimizedFunction(GLUTEmu_CallbackWindowMinimized function) {
        if (window != nullptr) {
            windowMinimizedFunction = function;

            // We are already listening for window minimization changes

            libqb_log_trace("Window minimized function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set minimized function");
        }
    }

    void WindowSetFocusedFunction(GLUTEmu_CallbackWindowFocused function) {
        if (window != nullptr) {
            windowFocusedFunction = function;

            // We are already listening for window focus changes

            libqb_log_trace("Window focused function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set focused function");
        }
    }

    void WindowSetRefreshFunction(GLUTEmu_CallbackWindowRefresh function) {
        if (window != nullptr) {
            windowRefreshFunction = function;

            glfwSetWindowRefreshCallback(window, [](GLFWwindow *win) {
                auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
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
        if (window != nullptr) {
            windowIdleFunction = function;

            libqb_log_trace("Idle function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set idle function");
        }
    }

    void KeyboardSetButtonFunction(GLUTEmu_CallbackKeyboardButton function) {
        if (window != nullptr) {
            keyboardButtonFunction = function;

            // We are already listening for keyboard events

            libqb_log_trace("Keyboard function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set keyboard function");
        }
    }

    void KeyboardSetCharacterFunction(GLUTEmu_CallbackKeyboardCharacter function) {
        if (window != nullptr) {
            keyboardCharacterFunction = function;

            glfwSetCharModsCallback(window, [](GLFWwindow *win, unsigned int codepoint, int mods) {
                auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->keyboardCharacterFunction) {
                    instance->keyboardCharacterFunction(static_cast<char32_t>(codepoint), mods);
                }
            });

            libqb_log_trace("Keyboard char function set: %p", function);
        } else {
            libqb_log_error("Window not created, cannot set keyboard char function");
        }
    }

    [[nodiscard]] bool KeyboardIsKeyModifierSet(GLUTEmu_KeyboardKeyModifier modifier) const {
        return (keyboardModifiers & modifier) != 0;
    }

    bool MouseSetStandardCursor(GLUTEmu_MouseStandardCursor style) {
        if (window != nullptr) {
            if (cursor != nullptr) {
                glfwDestroyCursor(cursor);
                cursor = nullptr;
                libqb_log_trace("Mouse cursor freed");
            }

            cursor = glfwCreateStandardCursor(static_cast<int>(style));
            if (cursor != nullptr) {
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
        if (window != nullptr) {
            // GLFW_TODO: implement custom bitmap cursor support
            libqb_log_warn("MouseSetCustomCursor is not implemented");
            return false;
        } else {
            libqb_log_error("Window not created, cannot set custom mouse cursor");
        }

        return false;
    }

    void MouseSetCursorMode(GLUTEnum_MouseCursorMode mode) {
        if (window != nullptr) {
            glfwSetInputMode(window, GLFW_CURSOR, static_cast<int>(mode));
            cursorMode = GLUTEnum_MouseCursorMode(glfwGetInputMode(window, GLFW_CURSOR));

            libqb_log_trace("Mouse cursor mode set to %d", int(mode));
        } else {
            libqb_log_error("Window not created, cannot set mouse cursor mode");
        }
    }

    [[nodiscard]] GLUTEnum_MouseCursorMode MouseGetCursorMode() const {
        return cursorMode;
    }

    void MouseMove(double x, double y) {
        if (window != nullptr) {
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
        if (window != nullptr) {
            mousePositionFunction = function;

            glfwSetCursorPosCallback(window, [](GLFWwindow *win, double xPos, double yPos) {
                auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
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
        if (window != nullptr) {
            mouseButtonFunction = function;

            glfwSetMouseButtonCallback(window, [](GLFWwindow *win, int button, int action, int mods) {
                auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
                if (instance->mouseButtonFunction) {
                    double xPos;
                    double yPos;
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
        if (window != nullptr) {
            mouseNotifyFunction = function;

            glfwSetCursorEnterCallback(window, [](GLFWwindow *win, int entered) {
                auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
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
        if (window != nullptr) {
            mouseScrollFunction = function;

            glfwSetScrollCallback(window, [](GLFWwindow *win, double scrollX, double scrollY) {
                auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
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
        if (window != nullptr) {
            dropFilesFunction = function;

            glfwSetDropCallback(window, [](GLFWwindow *win, int count, const char *paths[]) {
                auto *instance = reinterpret_cast<GLUTEmu *>(glfwGetWindowUserPointer(win));
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

        if (std::this_thread::get_id() == mainThreadId) {
            if (window != nullptr) {
                if (!isMainLoopRunning) {
                    isMainLoopRunning = true;

                    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
                        if (windowIdleFunction != nullptr) {
                            glfwPollEvents();
                        } else {
                            glfwWaitEventsTimeout(1.0);
                        }

                        MessageProcess();

                        if (windowIdleFunction != nullptr) {
                            windowIdleFunction();
                        }
                    }

                    isMainLoopRunning = false;
                } else {
                    libqb_log_warn("Main loop is already running");
                }
            } else {
                libqb_log_error("Window not created, cannot enter main loop");
            }
        } else {
            libqb_log_error("Main loop must be called from the main thread");
        }

        libqb_log_trace("Exiting main loop");
    }

    [[nodiscard]] bool MainLoopIsRunning() const {
        return isMainLoopRunning;
    }

    [[nodiscard]] bool MessageIsMainThread() const {
        return std::this_thread::get_id() == mainThreadId;
    }

    void MessageQueue(Message *msg) {
        {
            libqb_mutex_guard guard(msgQueueMutex);
            msgQueue.push(msg);
        }
        glfwPostEmptyEvent();
    }

    [[nodiscard]] double TimeGet() const {
        return glfwGetTime();
    }

    void TimeSet(double time) const {
        glfwSetTime(time);
    }

    static GLUTEmu &Instance() {
        static GLUTEmu instance;
        return instance;
    }

    GLUTEmu(const GLUTEmu &) = delete;
    GLUTEmu &operator=(const GLUTEmu &) = delete;
    GLUTEmu(GLUTEmu &&) = delete;
    GLUTEmu &operator=(GLUTEmu &&) = delete;

  private:
    GLUTEmu()
        : monitor(nullptr), monitorScaleX(1.0f), monitorScaleY(1.0f), window(nullptr), windowX(0), windowY(0), windowWidth(0), windowHeight(0),
          windowScaleX(1.0f), windowScaleY(1.0f), isWindowFullscreen(false), isWindowMaximized(false), isWindowMinimized(false), isWindowFocused(false),
          isWindowHidden(false), isWindowFloating(false), windowOpacity(1.0f), isWindowBordered(true), isWindowMousePassthrough(false), windowedX(0),
          windowedY(0), windowedWidth(0), windowedHeight(0), windowMinWidthLimit(GLFW_DONT_CARE), windowMinHeightLimit(GLFW_DONT_CARE),
          windowMaxWidthLimit(GLFW_DONT_CARE), windowMaxHeightLimit(GLFW_DONT_CARE), framebufferWidth(0), framebufferHeight(0), screenMode(0, 0, 0),
          cursor(nullptr), cursorMode(GLUTEnum_MouseCursorMode::Normal), keyboardModifiers(0), windowCloseFunction(nullptr), windowResizedFunction(nullptr),
          windowFramebufferResizedFunction(nullptr), windowMaximizedFunction(nullptr), windowMinimizedFunction(nullptr), windowFocusedFunction(nullptr),
          windowRefreshFunction(nullptr), windowIdleFunction(nullptr), keyboardButtonFunction(nullptr), keyboardCharacterFunction(nullptr),
          mousePositionFunction(nullptr), mouseButtonFunction(nullptr), mouseNotifyFunction(nullptr), mouseScrollFunction(nullptr), dropFilesFunction(nullptr),
          isMainLoopRunning(false) {
        mainThreadId = std::this_thread::get_id();
        msgQueueMutex = libqb_mutex_new();

#if defined(QB64_MACOSX) || defined(QB64_LINUX)
        keyboardScrollLockState = false;
#endif

#ifdef QB64_WINDOWS
        // Set the Windows multimedia timer resolution to 1ms, matching FreeGLUT's behavior.
        auto result = timeBeginPeriod(1);
        if (result == TIMERR_NOERROR) {
            libqb_log_trace("Windows timer resolution set to 1ms");
        } else {
            libqb_log_warn("Failed to set Windows timer resolution, result code: %d", result);
        }
#endif

        // Listen for GLFW error messages and route them to libqb logging
        glfwSetErrorCallback([](int error_code, const char *description) { libqb_log_error("GLFW error %d: %s", error_code, description); });

#ifdef QB64_LINUX
        if (glfwPlatformSupported(GLFW_PLATFORM_X11) == GLFW_TRUE) {
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
            libqb_log_trace("Forcing GLFW to use X11 platform");
        } else {
            libqb_log_warn("X11 platform not supported by GLFW, some features may not work correctly");
        }
#endif

        if (glfwInit() == GLFW_TRUE) {
            libqb_log_trace("GLFW %s initialized", glfwGetVersionString());
            monitor = WindowGetCurrentMonitorInfo();
        } else {
            // This will get caught outside because the window creation will fail
            libqb_log_error("Failed to initialize GLFW");
        }
    }

    ~GLUTEmu() {
        if (cursor != nullptr) {
            glfwDestroyCursor(cursor);
            cursor = nullptr;
            libqb_log_trace("Mouse cursor destroyed");
        }

        if (window != nullptr) {
            glfwDestroyWindow(window);
            window = nullptr;
            libqb_log_trace("Window closed");
        }

        glfwTerminate();
        libqb_log_trace("GLFW terminated");

#ifdef QB64_WINDOWS
        auto result = timeEndPeriod(1);
        if (result == TIMERR_NOERROR) {
            libqb_log_trace("Windows timer resolution reset to default");
        } else {
            libqb_log_warn("Failed to reset Windows timer resolution, result code: %d", result);
        }
#endif

        if (msgQueueMutex != nullptr) {
            libqb_mutex_free(msgQueueMutex);
            msgQueueMutex = nullptr;
        }
    }

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

    template <typename T>
    requires std::is_arithmetic_v<T>
    T ToPixelDesktopCoordsX(T x) const {
        if constexpr (std::integral<T>) {
            return static_cast<T>(std::round(x * monitorScaleX));
        } else {
            return x * monitorScaleX;
        }
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    T ToPixelDesktopCoordsY(T y) const {
        if constexpr (std::integral<T>) {
            return static_cast<T>(std::round(y * monitorScaleY));
        } else {
            return y * monitorScaleY;
        }
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    T ToScreenDesktopCoordsX(T x) const {
        if constexpr (std::integral<T>) {
            return static_cast<T>(std::round(x / monitorScaleX));
        } else {
            return x / monitorScaleX;
        }
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    T ToScreenDesktopCoordsY(T y) const {
        if constexpr (std::integral<T>) {
            return static_cast<T>(std::round(y / monitorScaleY));
        } else {
            return y / monitorScaleY;
        }
    }

    GLFWmonitor *WindowGetCurrentMonitorInfo() {
        GLFWmonitor *best = nullptr;

        if (window != nullptr) {
            best = glfwGetWindowMonitor(window);
            if (best == nullptr) {
                int wx, wy, ww, wh;
                glfwGetWindowPos(window, &wx, &wy);
                glfwGetWindowSize(window, &ww, &wh);

                int count = 0;
                auto *monitors = glfwGetMonitors(&count);
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

        if (best == nullptr) {
            best = glfwGetPrimaryMonitor();
        }

        if (best != nullptr) {
            const auto *mode = glfwGetVideoMode(best);
            if (mode != nullptr) {
                glfwGetMonitorContentScale(best, &monitorScaleX, &monitorScaleY);
                screenMode = {static_cast<int>(std::round(mode->width * monitorScaleX)), static_cast<int>(std::round(mode->height * monitorScaleY)),
                              mode->refreshRate};
            }
        }

        return best;
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
                mods = ((n & 0x04) != 0u) ? (mods | GLUTEmu_KeyboardKeyModifier::ScrollLock) : (mods & ~GLUTEmu_KeyboardKeyModifier::ScrollLock);
                break;

            case GLUTEmu_KeyboardKey::CapsLock:
                mods = ((n & 0x01) != 0u) ? (mods | GLUTEmu_KeyboardKeyModifier::CapsLock) : (mods & ~GLUTEmu_KeyboardKeyModifier::CapsLock);
                break;

            case GLUTEmu_KeyboardKey::NumLock:
                mods = ((n & 0x02) != 0u) ? (mods | GLUTEmu_KeyboardKeyModifier::NumLock) : (mods & ~GLUTEmu_KeyboardKeyModifier::NumLock);
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

#if defined(QB64_WINDOWS)
    bool WindowLoadResourceIconNative(int resourceId = 0) {
        libqb_mutex_guard guard(msgQueueMutex);

        static HICON hBigIcon = nullptr;
        static HICON hSmallIcon = nullptr;

        auto hWnd = glfwGetWin32Window(window);
        if (hWnd == nullptr) {
            libqb_log_error("Failed to get native window handle for setting icon");
            return false;
        }

        if (hBigIcon != nullptr) {
            DestroyIcon(hBigIcon);
            hBigIcon = nullptr;
        }

        if (hSmallIcon != nullptr) {
            DestroyIcon(hSmallIcon);
            hSmallIcon = nullptr;
        }

        if (hBigIcon == nullptr && hSmallIcon == nullptr) {
            auto hInst = GetModuleHandle(nullptr);

            hBigIcon = reinterpret_cast<HICON>(
                LoadImageA(hInst, MAKEINTRESOURCEA(resourceId), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));

            if (hBigIcon == nullptr) {
                hBigIcon = reinterpret_cast<HICON>(LoadImageA(hInst, MAKEINTRESOURCEA(resourceId), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
            }

            if (hBigIcon == nullptr) {
                hBigIcon = reinterpret_cast<HICON>(LoadImageA(hInst, MAKEINTRESOURCEA(resourceId), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
            }

            hSmallIcon = reinterpret_cast<HICON>(
                LoadImageA(hInst, MAKEINTRESOURCEA(resourceId), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));

            if (hSmallIcon == nullptr) {
                hSmallIcon = reinterpret_cast<HICON>(LoadImageA(hInst, MAKEINTRESOURCEA(resourceId), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
            }

            if (hSmallIcon == nullptr) {
                hSmallIcon = reinterpret_cast<HICON>(LoadImageA(hInst, MAKEINTRESOURCEA(resourceId), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
            }
        }

        if (hBigIcon != nullptr) {
            SendMessage(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hBigIcon));
            libqb_log_trace("Big resource icon set successfully");
        } else {
            libqb_log_error("Failed to create big resource icon");
        }

        if (hSmallIcon != nullptr) {
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hSmallIcon));
            libqb_log_trace("Small resource icon set successfully");
        } else {
            libqb_log_error("Failed to create small resource icon");
        }

        return hBigIcon != nullptr && hSmallIcon != nullptr;
    }
#elif defined(QB64_LINUX)
    bool WindowLoadResourceIconNative(int resourceId = 0) {
        (void)resourceId;
        libqb_log_warn("WindowLoadResourceIconNative is not implemented on Linux, using default icon");
        return false;
    }
#elif defined(QB64_MACOSX)
    bool WindowLoadResourceIconNative(int resourceId = 0) {
        (void)resourceId;
        libqb_log_warn("WindowLoadResourceIconNative is not implemented on macOS, using default icon");
        return false;
    }
#endif

    void MessageProcess() {
        std::queue<Message *> localQueue;
        {
            libqb_mutex_guard guard(msgQueueMutex);
            std::swap(msgQueue, localQueue);
        }

        while (!localQueue.empty()) {
            auto *msg = localQueue.front();
            localQueue.pop();
            msg->Execute();
            msg->Finish();
        }
    }

    static constexpr uint8_t Icon32bppRGBA16x16[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x05, 0x07, 0x07, 0x38, 0x55, 0x12, 0x12, 0x56, 0x96, 0x59, 0x51, 0x31, 0xBE, 0x4F,
        0x47, 0x25, 0x8B, 0x05, 0x04, 0x03, 0x0F, 0x19, 0x08, 0x06, 0x30, 0x6E, 0x1C, 0x11, 0x99, 0x6F, 0x24, 0x1B, 0xBD, 0x17, 0x12, 0x4F, 0x9A, 0x07, 0x07,
        0x37, 0x54, 0x01, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x09, 0x09, 0x09, 0xA3,
        0xC9, 0x06, 0x06, 0xC3, 0xDB, 0x3A, 0x35, 0x22, 0x79, 0x52, 0x4B, 0x27, 0x93, 0x52, 0x4A, 0x26, 0x92, 0x38, 0x32, 0x1B, 0x67, 0x32, 0x0E, 0x0A, 0x52,
        0xAA, 0x2B, 0x19, 0xE2, 0xAD, 0x2B, 0x19, 0xE2, 0x2E, 0x10, 0x0E, 0x5D, 0x07, 0x07, 0xBE, 0xD7, 0x0A, 0x0A, 0x96, 0xBB, 0x02, 0x02, 0x02, 0x09, 0x00,
        0x00, 0x00, 0x00, 0x05, 0x05, 0x1A, 0x2D, 0x0B, 0x0B, 0x6C, 0x97, 0x07, 0x07, 0xE0, 0xFC, 0x07, 0x07, 0x55, 0x71, 0x1E, 0x1B, 0x0F, 0x3F, 0x7A, 0x6F,
        0x38, 0xCE, 0x85, 0x78, 0x3B, 0xD2, 0x23, 0x20, 0x12, 0x4D, 0x30, 0x0E, 0x0A, 0x52, 0xA7, 0x2A, 0x18, 0xDF, 0x9C, 0x28, 0x19, 0xDA, 0x31, 0x0E, 0x0A,
        0x52, 0x07, 0x07, 0x5E, 0x7A, 0x07, 0x07, 0xE2, 0xFD, 0x0A, 0x0A, 0x71, 0x9C, 0x05, 0x05, 0x1B, 0x2F, 0x07, 0x07, 0x8A, 0xA7, 0x06, 0x06, 0xD5, 0xEA,
        0x09, 0x09, 0xC8, 0xEE, 0x07, 0x07, 0x60, 0x7D, 0x00, 0x00, 0x00, 0x00, 0x08, 0x1A, 0x22, 0x47, 0x23, 0x4E, 0x58, 0xB4, 0x0D, 0x0C, 0x07, 0x23, 0x01,
        0x01, 0x01, 0x07, 0x0D, 0x23, 0x4D, 0x85, 0x0D, 0x26, 0x55, 0x8B, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x72, 0x8F, 0x0A, 0x0A, 0xC4, 0xED, 0x05, 0x05,
        0xDA, 0xEE, 0x07, 0x07, 0x89, 0xA6, 0x0C, 0x0C, 0x6B, 0x9C, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0x02, 0xF7, 0xFF, 0x08, 0x08, 0x9B, 0xBB, 0x03, 0x09, 0x0C,
        0x1B, 0x0F, 0x85, 0xB5, 0xE0, 0x10, 0x70, 0x97, 0xCD, 0x04, 0x12, 0x17, 0x2B, 0x09, 0x1D, 0x43, 0x65, 0x13, 0x44, 0x9C, 0xE6, 0x0D, 0x3D, 0x94, 0xBE,
        0x01, 0x01, 0x02, 0x07, 0x07, 0x07, 0x9D, 0xBB, 0x02, 0x02, 0xF6, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0C, 0x0C, 0x75, 0xA6, 0x0F, 0x0F, 0x96, 0xCF, 0x0E,
        0x0E, 0x9C, 0xD3, 0x00, 0x00, 0xFE, 0xFF, 0x0F, 0x0F, 0x6D, 0xAA, 0x07, 0x14, 0x1B, 0x3A, 0x10, 0x78, 0xA3, 0xD6, 0x10, 0x74, 0x9E, 0xD1, 0x06, 0x1E,
        0x28, 0x42, 0x09, 0x1D, 0x41, 0x63, 0x12, 0x38, 0x7E, 0xC1, 0x0F, 0x45, 0xA9, 0xD8, 0x05, 0x06, 0x0A, 0x21, 0x0F, 0x0F, 0x6B, 0xA8, 0x00, 0x00, 0xFD,
        0xFF, 0x0E, 0x0E, 0xAC, 0xE5, 0x0F, 0x0F, 0xA4, 0xDE, 0x07, 0x07, 0xAA, 0xC8, 0x03, 0x03, 0xF2, 0xFE, 0x05, 0x05, 0xEC, 0xFF, 0x0D, 0x0D, 0x9B, 0xCF,
        0x0A, 0x0A, 0x8F, 0xB8, 0x04, 0x04, 0x09, 0x1D, 0x02, 0x02, 0x02, 0x0E, 0x08, 0x08, 0x58, 0x78, 0x08, 0x08, 0x9D, 0xBF, 0x09, 0x09, 0x33, 0x59, 0x03,
        0x03, 0x05, 0x10, 0x0B, 0x0B, 0x82, 0xAF, 0x0C, 0x0C, 0x8E, 0xBF, 0x06, 0x06, 0xE9, 0xFF, 0x02, 0x02, 0xF6, 0xFF, 0x07, 0x07, 0xB0, 0xCE, 0x04, 0x04,
        0x18, 0x29, 0x0E, 0x0E, 0x92, 0xCD, 0x09, 0x09, 0xCA, 0xEF, 0x04, 0x04, 0xF1, 0xFF, 0x0F, 0x0F, 0xB6, 0xF2, 0x0B, 0x0B, 0xC3, 0xEE, 0x0E, 0x0E, 0x3B,
        0x74, 0x0B, 0x0B, 0x9A, 0xC6, 0x05, 0x05, 0xE7, 0xFC, 0x07, 0x07, 0x1D, 0x38, 0x0B, 0x0B, 0xBE, 0xEA, 0x0F, 0x0F, 0xAE, 0xEA, 0x04, 0x04, 0xEF, 0xFF,
        0x09, 0x09, 0xC8, 0xEB, 0x0F, 0x0F, 0x8C, 0xC8, 0x04, 0x04, 0x15, 0x26, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x30, 0x49, 0x0A, 0x0A, 0xC0, 0xE9, 0x08,
        0x08, 0xDE, 0xFF, 0x06, 0x06, 0xE6, 0xFE, 0x10, 0x7D, 0xBB, 0xFF, 0x12, 0x95, 0x9B, 0xEC, 0x0A, 0x44, 0xB9, 0xF6, 0x04, 0x0A, 0xE7, 0xFF, 0x12, 0xA7,
        0xA8, 0xF0, 0x14, 0x92, 0xA4, 0xFF, 0x07, 0x09, 0xBD, 0xFF, 0x08, 0x08, 0xE1, 0xFF, 0x0B, 0x0B, 0xB0, 0xDE, 0x06, 0x06, 0x27, 0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x04, 0x0E, 0x07, 0x07, 0x53, 0x70, 0x11, 0x11, 0x5E, 0xA4, 0x12, 0xBD, 0xB6, 0xFC,
        0x0D, 0x82, 0xA9, 0xFF, 0x0B, 0x62, 0xA3, 0xFF, 0x08, 0x37, 0xD9, 0xFF, 0x0F, 0xBF, 0xBE, 0xFF, 0x10, 0x66, 0x7D, 0xF2, 0x0F, 0x0F, 0x5E, 0xAB, 0x08,
        0x08, 0x47, 0x65, 0x01, 0x01, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x14, 0x13, 0x32, 0x0E, 0x9D, 0x95, 0xE7, 0x0B, 0x28, 0x4B, 0xAF, 0x01, 0x01, 0xDA, 0xFF, 0x0A, 0x68, 0xD6,
        0xFF, 0x10, 0x91, 0x8A, 0xEA, 0x0C, 0x42, 0x3F, 0xB0, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0x08, 0x1B, 0x07,
        0x17, 0x1D, 0x6A, 0x0C, 0x0C, 0xAC, 0xDF, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0x17, 0xD3, 0xFF, 0x0F, 0x33, 0x87, 0xF7, 0x07, 0x19, 0x1C, 0x6C, 0x00, 0x00,
        0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x05, 0x0C, 0x20, 0x0D, 0x0D, 0x8B, 0xBD, 0x09, 0x09, 0xD9, 0xFE,
        0x0B, 0x0B, 0xCC, 0xF7, 0x0D, 0x0D, 0x92, 0xC7, 0x04, 0x04, 0x08, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x09, 0xAD, 0xD0, 0x0E, 0x0E, 0x9D, 0xD4, 0x0E, 0x0E, 0x8B, 0xC2, 0x08, 0x08, 0xB5, 0xD7, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x12, 0x24, 0x10,
        0x10, 0x4D, 0x8F, 0x10, 0x10, 0x51, 0x90, 0x05, 0x05, 0x19, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x05, 0x15, 0x08, 0x08, 0x0E, 0x2D, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    static constexpr uint8_t Icon32bppRGBA32x32[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x09, 0x0B, 0x2F, 0x0F, 0x0F, 0x3C, 0x7B, 0x1C, 0x1C, 0x26, 0x9A, 0x15, 0x14, 0x14, 0x6F, 0x0A, 0x0A,
        0x09, 0x3A, 0x01, 0x01, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x1B, 0x08, 0x08, 0x08,
        0x33, 0x08, 0x08, 0x08, 0x33, 0x10, 0x10, 0x10, 0x5F, 0x1E, 0x1E, 0x22, 0x9D, 0x12, 0x12, 0x42, 0x89, 0x0B, 0x0B, 0x10, 0x3A, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04,
        0x04, 0x12, 0x0F, 0x0F, 0x3E, 0x79, 0x0F, 0x0F, 0xA1, 0xDA, 0x0D, 0x0D, 0xCA, 0xFB, 0x22, 0x20, 0x46, 0xB3, 0x85, 0x79, 0x3F, 0xEE, 0xAC, 0x9B, 0x4B,
        0xFF, 0xAC, 0x9B, 0x4B, 0xFF, 0x86, 0x79, 0x3F, 0xEB, 0x13, 0x12, 0x0C, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x1D, 0x14, 0xA4,
        0xDC, 0x31, 0x1B, 0xFF, 0xD0, 0x31, 0x1B, 0xFF, 0xD3, 0x31, 0x1B, 0xFF, 0xBB, 0x2F, 0x1B, 0xFA, 0x33, 0x1C, 0x31, 0xB3, 0x0D, 0x0D, 0xBA, 0xF1, 0x0F,
        0x0F, 0xA2, 0xDC, 0x0F, 0x0F, 0x37, 0x71, 0x02, 0x02, 0x02, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x2C,
        0x5C, 0x0B, 0x0B, 0xC1, 0xF0, 0x00, 0x00, 0xFF, 0xFF, 0x08, 0x08, 0xDA, 0xF9, 0x0B, 0x0B, 0x1D, 0x4A, 0x66, 0x5D, 0x31, 0xC2, 0xA4, 0x94, 0x49, 0xFF,
        0x14, 0x13, 0x0E, 0x4E, 0x13, 0x12, 0x0E, 0x4D, 0xA4, 0x93, 0x49, 0xFF, 0x67, 0x5E, 0x32, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63,
        0x1D, 0x14, 0xA4, 0xE0, 0x32, 0x1A, 0xFF, 0x15, 0x15, 0x15, 0x8C, 0x1B, 0x15, 0x14, 0x8B, 0xDD, 0x31, 0x1B, 0xFF, 0x6E, 0x1F, 0x16, 0xB2, 0x08, 0x08,
        0x10, 0x32, 0x09, 0x09, 0xCB, 0xF2, 0x00, 0x00, 0xFD, 0xFF, 0x0E, 0x0E, 0xB2, 0xE7, 0x0A, 0x0A, 0x1C, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x14, 0x03, 0x03, 0x03, 0x11,
        0x0E, 0x0E, 0xA2, 0xD9, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0x01, 0xFC, 0xFF, 0x0F, 0x0F, 0x37, 0x74, 0x00, 0x00, 0x00, 0x00, 0x76, 0x6B, 0x38, 0xD7, 0x91,
        0x83, 0x43, 0xFD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x82, 0x43, 0xFC, 0x78, 0x6D, 0x38, 0xD9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x63, 0x1D, 0x14, 0xA4, 0xE6, 0x32, 0x1A, 0xFF, 0xCF, 0x31, 0x1B, 0xFF, 0xD4, 0x31, 0x1B, 0xFF, 0xE6, 0x32, 0x1A, 0xFF, 0x41, 0x18, 0x13,
        0x91, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x30, 0x6B, 0x01, 0x01, 0xFC, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x0F, 0x8A, 0xC3, 0x02, 0x02, 0x02, 0x0A,
        0x05, 0x05, 0x05, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x0F, 0x13, 0x13, 0x86, 0xD1, 0x0A,
        0x0A, 0x0A, 0x34, 0x0E, 0x0E, 0xC2, 0xF8, 0x00, 0x00, 0xFF, 0xFF, 0x0C, 0x0C, 0xC0, 0xF3, 0x02, 0x02, 0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x66, 0x5D,
        0x31, 0xC2, 0xA4, 0x94, 0x49, 0xFF, 0x14, 0x13, 0x0E, 0x4E, 0x13, 0x12, 0x0E, 0x4D, 0xA4, 0x94, 0x49, 0xFF, 0x67, 0x5E, 0x32, 0xC4, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x1D, 0x14, 0xA4, 0xE0, 0x32, 0x1A, 0xFF, 0x13, 0x13, 0x13, 0x81, 0x11, 0x10, 0x10, 0x6D, 0xC9, 0x30, 0x1C, 0xFF,
        0x8D, 0x26, 0x19, 0xD3, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x0B, 0x0C, 0x0C, 0xCA, 0xFA, 0x00, 0x00, 0xFF, 0xFF, 0x0E, 0x0E, 0xC2, 0xF8, 0x0A,
        0x0A, 0x0A, 0x31, 0x12, 0x12, 0x93, 0xDC, 0x04, 0x04, 0x04, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x63, 0xA4, 0x00, 0x00,
        0xFE, 0xFF, 0x0E, 0x0E, 0x20, 0x57, 0x0E, 0x0E, 0xC2, 0xF9, 0x00, 0x00, 0xFF, 0xFF, 0x0D, 0x0D, 0x93, 0xCA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x13, 0x11, 0x0C, 0x3C, 0x85, 0x79, 0x3E, 0xEB, 0xAC, 0x9B, 0x4B, 0xFF, 0xAD, 0x9C, 0x4B, 0xFF, 0xB2, 0xA0, 0x4D, 0xFF, 0x26, 0x23, 0x17, 0x71,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x1D, 0x14, 0xA4, 0xDB, 0x31, 0x1B, 0xFF, 0xD0, 0x31, 0x1B, 0xFF, 0xD2, 0x31, 0x1B, 0xFF, 0xC5,
        0x30, 0x1C, 0xFE, 0x37, 0x14, 0x0F, 0x75, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x0D, 0xAD, 0xE4, 0x00, 0x00, 0xFF, 0xFF, 0x0D, 0x0D,
        0xC8, 0xFD, 0x0F, 0x0F, 0x2A, 0x64, 0x00, 0x00, 0xFF, 0xFF, 0x10, 0x10, 0x67, 0xA7, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x0A, 0x2A, 0x07, 0x07, 0xE3,
        0xFE, 0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x0F, 0x6F, 0xAB, 0x0E, 0x0E, 0xA3, 0xDB, 0x00, 0x00, 0xFF, 0xFF, 0x0E, 0x0E, 0x9C, 0xD4, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x09, 0x0C, 0x0C, 0x0B, 0x49, 0x17, 0x1B, 0x1B, 0x8E, 0x59, 0x53, 0x33, 0xD9, 0x35,
        0x31, 0x1D, 0x8B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x1A, 0x08, 0x08, 0x08, 0x33, 0x11, 0x16, 0x21, 0x7C, 0x13, 0x1E,
        0x32, 0x96, 0x08, 0x09, 0x0B, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x04, 0x0D, 0x0D, 0xC3, 0xF7, 0x00, 0x00, 0xFF,
        0xFF, 0x0E, 0x0E, 0xA0, 0xD8, 0x0F, 0x0F, 0x7E, 0xBA, 0x00, 0x00, 0xFF, 0xFF, 0x07, 0x07, 0xE0, 0xFE, 0x07, 0x07, 0x08, 0x26, 0x0E, 0x0E, 0x3C, 0x74,
        0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x06, 0x06, 0xE6, 0xFE, 0x19, 0x19, 0x7E, 0xE0, 0x00, 0x00, 0xFF, 0xFF, 0x08, 0x08, 0xDD, 0xFD, 0x07,
        0x07, 0x08, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x08, 0x12, 0x5C, 0x7A, 0xC3, 0x0F, 0xA6, 0xE4, 0xFF, 0x0C, 0x26,
        0x31, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x17, 0x2D, 0x67, 0x0F, 0x59, 0xDE,
        0xFE, 0x0D, 0x5E, 0xF2, 0xFF, 0x0B, 0x14, 0x25, 0x5E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x14, 0x43, 0x03, 0x03, 0xF4, 0xFF,
        0x00, 0x00, 0xFF, 0xFF, 0x1B, 0x1B, 0x71, 0xDD, 0x05, 0x05, 0xED, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0E, 0x0E, 0x3E, 0x76, 0x0D,
        0x0D, 0x26, 0x5A, 0x02, 0x02, 0xF6, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x08, 0x08, 0xE0, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        0xFF, 0xFF, 0x10, 0x10, 0x5E, 0x9F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x11, 0x48, 0x5F, 0xAA, 0x0D, 0xAE, 0xF1, 0xFF, 0x13, 0x84, 0xB2,
        0xF3, 0x0D, 0x2D, 0x3B, 0x7A, 0x01, 0x01, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x14, 0x25, 0x5A, 0x10, 0x56, 0xD8, 0xFD,
        0x15, 0x47, 0xA5, 0xF6, 0x0D, 0x5E, 0xF2, 0xFF, 0x0B, 0x14, 0x25, 0x5E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x62, 0x9F, 0x00,
        0x00, 0xFF, 0xFF, 0x02, 0x02, 0xF9, 0xFF, 0x07, 0x07, 0xE3, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0x01, 0xFB, 0xFF, 0x0E, 0x0E,
        0x27, 0x5E, 0x11, 0x11, 0x15, 0x59, 0x11, 0x11, 0x7A, 0xBC, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF,
        0xFF, 0x03, 0x03, 0xF3, 0xFF, 0x0C, 0x0C, 0x1C, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x24, 0x2E, 0x69, 0x0E, 0xAA, 0xEB, 0xFF, 0x11, 0x72, 0x9A, 0xD9,
        0x11, 0x64, 0x85, 0xC8, 0x0E, 0xAA, 0xEB, 0xFF, 0x10, 0x45, 0x5C, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x08, 0x0E, 0x17, 0x41, 0x10, 0x54, 0xD2, 0xFA, 0x15,
        0x46, 0x9E, 0xF3, 0x14, 0x2C, 0x57, 0xB3, 0x0D, 0x5E, 0xF2, 0xFF, 0x12, 0x23, 0x44, 0x9A, 0x04, 0x05, 0x06, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x0D,
        0x1D, 0x4F, 0x02, 0x02, 0xF5, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x0F, 0x98,
        0xD5, 0x13, 0x13, 0x1A, 0x65, 0x15, 0x15, 0x9F, 0xF5, 0x10, 0x10, 0x24, 0x62, 0x0F, 0x0F, 0x99, 0xD5, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF,
        0x00, 0x00, 0xFF, 0xFF, 0x0E, 0x0E, 0xB2, 0xE9, 0x0C, 0x0C, 0x0C, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x41, 0x55, 0x93, 0x0D, 0xAF, 0xF2, 0xFF, 0x10,
        0x2A, 0x35, 0x87, 0x0C, 0x18, 0x1D, 0x5E, 0x10, 0x9F, 0xD9, 0xFF, 0x11, 0x65, 0x88, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x1C, 0x36, 0x77, 0x12, 0x53,
        0xCA, 0xFF, 0x12, 0x53, 0xCA, 0xFF, 0x11, 0x56, 0xD5, 0xFF, 0x0D, 0x5E, 0xF2, 0xFF, 0x11, 0x55, 0xD0, 0xFF, 0x0B, 0x11, 0x1D, 0x57, 0x00, 0x00, 0x00,
        0x00, 0x0C, 0x0C, 0x0C, 0x3A, 0x0E, 0x0E, 0xB7, 0xED, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0D, 0x0D, 0xB6, 0xE7,
        0x15, 0x15, 0x3C, 0x90, 0x14, 0x14, 0xAB, 0xFB, 0x0E, 0x0E, 0xB2, 0xE9, 0x06, 0x06, 0xE4, 0xFD, 0x14, 0x14, 0x54, 0xA4, 0x15, 0x15, 0x83, 0xD5, 0x01,
        0x01, 0xFA, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0E, 0x0E, 0x96, 0xCD, 0x15, 0x15, 0x64, 0xB8, 0x09, 0x09, 0x11, 0x35, 0x05, 0x06, 0x07, 0x20, 0x12, 0x68,
        0x8B, 0xD2, 0x10, 0x9E, 0xD9, 0xFF, 0x0F, 0xA2, 0xDE, 0xFF, 0x12, 0x7A, 0xA6, 0xE7, 0x08, 0x13, 0x17, 0x40, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01,
        0x04, 0x02, 0x02, 0x02, 0x10, 0x0C, 0x0C, 0x12, 0x45, 0x1B, 0x2A, 0x45, 0xBF, 0x13, 0x50, 0xC0, 0xFF, 0x0D, 0x13, 0x20, 0x62, 0x00, 0x00, 0x00, 0x02,
        0x08, 0x08, 0x0C, 0x2B, 0x16, 0x16, 0x52, 0xAC, 0x0E, 0x0E, 0x96, 0xCD, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0x02, 0xF9, 0xFF, 0x16, 0x16, 0x8F, 0xE7, 0x16,
        0x16, 0x6F, 0xC7, 0x04, 0x04, 0xF0, 0xFF, 0x0E, 0x0E, 0xB9, 0xEF, 0x0E, 0x0E, 0x85, 0xBD, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0C, 0x0C,
        0xCD, 0xFB, 0x13, 0x13, 0xB1, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0C, 0x0C, 0xC2, 0xF3, 0x11, 0x11, 0x55, 0x99, 0x0C, 0x0C, 0xB1, 0xE3, 0x07, 0x07, 0x09,
        0x24, 0x00, 0x00, 0x00, 0x01, 0x05, 0x05, 0x05, 0x23, 0x06, 0x06, 0x06, 0x2A, 0x01, 0x01, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0A, 0x0E, 0x35,
        0x0E, 0x0E, 0x3B, 0x72, 0x0F, 0x0F, 0x56, 0x8F, 0x14, 0x14, 0x9A, 0xEB, 0x04, 0x04, 0x04, 0x16, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x05, 0x05, 0x19, 0x0E, 0x0E, 0xA3, 0xDA, 0x10, 0x10, 0x40, 0x7F, 0x0D, 0x0D, 0xBA, 0xEE, 0x01, 0x01, 0xFA, 0xFF, 0x15, 0x15, 0xAC, 0xFF, 0x0A, 0x0A,
        0xD8, 0xFE, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0E, 0x0E, 0x90, 0xC8, 0x0E, 0x0E, 0x2C, 0x63, 0x02, 0x02, 0xF9, 0xFF, 0x00, 0x00, 0xFF,
        0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFE, 0xFF, 0x16, 0x16, 0x57, 0xB0, 0x0B, 0x0B, 0xC2, 0xEF,
        0x0B, 0x0B, 0xBE, 0xEB, 0x0C, 0x0C, 0x20, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x0C, 0x11, 0x11, 0x69, 0xAB, 0x05,
        0x05, 0xEB, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x06, 0x06, 0xE3, 0xFC, 0x0D, 0x0D, 0x2F, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0A,
        0x15, 0x3D, 0x0D, 0x0D, 0xAD, 0xE1, 0x0D, 0x0D, 0xB5, 0xE8, 0x13, 0x13, 0x42, 0x90, 0x01, 0x01, 0xFB, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF,
        0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0x01, 0xFB, 0xFF, 0x0E, 0x0E, 0x38, 0x72, 0x01, 0x01, 0x01, 0x03, 0x11, 0x11, 0x5E, 0xA1,
        0x08, 0x08, 0xDE, 0xFE, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x03, 0xF1, 0xFF, 0x1A,
        0x1A, 0x68, 0xD3, 0x0C, 0x0C, 0xCA, 0xF9, 0x04, 0x04, 0xEF, 0xFF, 0x0F, 0x0F, 0x7D, 0xBB, 0x08, 0x08, 0x10, 0x2E, 0x12, 0x12, 0x4F, 0x97, 0x10, 0x10,
        0x83, 0xC4, 0x0D, 0x0D, 0xC9, 0xFD, 0x00, 0x00, 0xFF, 0xFF, 0x0D, 0x0D, 0xBE, 0xF4, 0x00, 0x00, 0x00, 0x01, 0x06, 0x06, 0x0B, 0x25, 0x0F, 0x0F, 0x71,
        0xAD, 0x06, 0x06, 0xE6, 0xFE, 0x0B, 0x0B, 0xC9, 0xF5, 0x19, 0x19, 0x57, 0xBB, 0x05, 0x05, 0xE9, 0xFE, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF,
        0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x0A, 0xD2, 0xFA, 0x10, 0x10, 0x55, 0x95, 0x01, 0x01, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x17, 0x17, 0x23, 0x7F, 0x1B, 0x1B, 0x4A, 0xB8, 0x15, 0x15, 0x7C, 0xD0, 0x0F, 0x0F, 0xAF, 0xEC, 0x09, 0x09, 0xDA, 0xFF, 0x02, 0x02,
        0xF9, 0xFF, 0x01, 0x01, 0xFD, 0xFF, 0x15, 0x15, 0xAA, 0xFE, 0x16, 0x16, 0xA7, 0xFF, 0x02, 0x02, 0xF8, 0xFF, 0x0F, 0x0F, 0x77, 0xB2, 0x11, 0x11, 0x15,
        0x59, 0x0D, 0x0D, 0x22, 0x58, 0x02, 0x02, 0xF9, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x08, 0x08, 0xE0, 0xFF, 0x06, 0x06, 0x06, 0x1F, 0x0F, 0x0F, 0x61, 0x9C,
        0x01, 0x01, 0xFA, 0xFF, 0x15, 0x15, 0xA9, 0xFF, 0x16, 0x16, 0xA2, 0xFA, 0x01, 0x01, 0xFA, 0xFF, 0x01, 0x01, 0xFB, 0xFF, 0x09, 0x09, 0xDA, 0xFF, 0x0E,
        0x0E, 0xAB, 0xE7, 0x14, 0x14, 0x76, 0xC7, 0x1B, 0x1B, 0x40, 0xAC, 0x16, 0x16, 0x20, 0x7A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x0D, 0x2C, 0x0D, 0x0D, 0xAD, 0xE4, 0x07, 0x07, 0xE4, 0xFF, 0x0C, 0x0C, 0xCD, 0xFE, 0x10, 0x10, 0xC0,
        0xFF, 0x10, 0x10, 0xBD, 0xFF, 0x02, 0x02, 0xF6, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0C, 0x26, 0xC3, 0xFF, 0x17, 0x82, 0x99, 0xFF, 0x14, 0x7E, 0xA5, 0xFD,
        0x10, 0x71, 0x6C, 0xB6, 0x12, 0x2B, 0x77, 0xDA, 0x00, 0x00, 0xF7, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0x05, 0xE0, 0xFF, 0x14, 0x78, 0x73, 0xCA, 0x13,
        0x7D, 0x9F, 0xF8, 0x17, 0x81, 0x9B, 0xFF, 0x12, 0x7D, 0xAC, 0xFF, 0x08, 0x0E, 0xB7, 0xFF, 0x00, 0x00, 0xFC, 0xFF, 0x0E, 0x0E, 0xC5, 0xFF, 0x10, 0x10,
        0xBF, 0xFE, 0x0D, 0x0D, 0xCB, 0xFE, 0x08, 0x08, 0xDF, 0xFF, 0x11, 0x11, 0x92, 0xD3, 0x07, 0x07, 0x0A, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x16, 0x10, 0x10, 0x68, 0xA8, 0x06, 0x06, 0xE8, 0xFF,
        0x00, 0x00, 0xFF, 0xFF, 0x01, 0x01, 0xFC, 0xFF, 0x12, 0x12, 0xB5, 0xFB, 0x04, 0x04, 0xED, 0xFF, 0x0F, 0x6C, 0xB8, 0xFF, 0x0F, 0xE2, 0xD6, 0xFF, 0x12,
        0x9F, 0x9F, 0xFF, 0x11, 0xC8, 0xC0, 0xFF, 0x10, 0xDB, 0xD0, 0xFF, 0x07, 0x09, 0xA5, 0xFF, 0x00, 0x00, 0xFD, 0xFF, 0x0B, 0x24, 0xC0, 0xFF, 0x0D, 0xF2,
        0xE5, 0xFF, 0x13, 0xB1, 0xAB, 0xFF, 0x12, 0xA9, 0xA7, 0xFF, 0x13, 0xA1, 0xA0, 0xFF, 0x09, 0x09, 0x76, 0xFF, 0x0C, 0x0C, 0xCC, 0xFF, 0x00, 0x00, 0xFF,
        0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x0A, 0x0A, 0xCF, 0xF7, 0x0F, 0x0F, 0x45, 0x83, 0x02, 0x02, 0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,
        0x09, 0x11, 0x37, 0x0F, 0x0F, 0x71, 0xAD, 0x0A, 0x0A, 0xD5, 0xFB, 0x0C, 0x0C, 0xBB, 0xEA, 0x14, 0x14, 0x22, 0x73, 0x14, 0x9F, 0x9F, 0xF3, 0x13, 0xA9,
        0xA4, 0xFF, 0x01, 0x01, 0x7E, 0xFF, 0x10, 0x5A, 0x92, 0xFF, 0x0E, 0xEB, 0xDE, 0xFF, 0x09, 0x0C, 0x7A, 0xFF, 0x00, 0x00, 0xF9, 0xFF, 0x0E, 0x55, 0xBB,
        0xFF, 0x0E, 0xEC, 0xDF, 0xFF, 0x11, 0x37, 0x59, 0xFF, 0x0C, 0x30, 0x79, 0xFF, 0x10, 0x20, 0x3F, 0xDA, 0x11, 0x11, 0x20, 0x8C, 0x0A, 0x0A, 0xCD, 0xF5,
        0x0B, 0x0B, 0xC3, 0xF3, 0x0F, 0x0F, 0x56, 0x92, 0x05, 0x05, 0x06, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x05, 0x05, 0x19, 0x0E, 0x0E, 0x36, 0x6F, 0x15, 0x15, 0x63, 0xC2, 0x12, 0xCC, 0xC1,
        0xFF, 0x10, 0xDD, 0xD1, 0xFF, 0x13, 0xC4, 0xBA, 0xFF, 0x0E, 0xE8, 0xDC, 0xFF, 0x13, 0x8F, 0x93, 0xFF, 0x01, 0x01, 0xA1, 0xFF, 0x00, 0x00, 0xFC, 0xFF,
        0x11, 0x87, 0xB6, 0xFF, 0x0D, 0xF0, 0xE3, 0xFF, 0x0E, 0xEB, 0xDE, 0xFF, 0x0E, 0xEB, 0xDE, 0xFF, 0x14, 0x5C, 0x5D, 0xF0, 0x12, 0x12, 0x5D, 0xC4, 0x0E,
        0x0E, 0x2F, 0x68, 0x03, 0x03, 0x03, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x12, 0x11, 0x4B,
        0x0D, 0xEE, 0xE2, 0xFF, 0x12, 0x7D, 0x77, 0xF0, 0x0E, 0x56, 0x52, 0xD2, 0x11, 0x3D, 0x47, 0xE3, 0x03, 0x03, 0x87, 0xFF, 0x00, 0x00, 0xE9, 0xFF, 0x01,
        0x01, 0xF8, 0xFF, 0x13, 0xB7, 0xB7, 0xFF, 0x12, 0x90, 0x89, 0xF7, 0x0B, 0x0B, 0x0B, 0xB3, 0x0E, 0x0E, 0x0E, 0xAF, 0x04, 0x04, 0x04, 0x74, 0x00, 0x00,
        0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D,
        0x3F, 0x3D, 0x7F, 0x0D, 0xF0, 0xE3, 0xFF, 0x0A, 0x17, 0x16, 0xAE, 0x01, 0x01, 0x01, 0x29, 0x0D, 0x0D, 0x94, 0xDE, 0x00, 0x00, 0xF8, 0xFF, 0x00, 0x00,
        0xFF, 0xFF, 0x07, 0x07, 0xD3, 0xFF, 0x0F, 0xE2, 0xD6, 0xFF, 0x10, 0xDE, 0xD2, 0xFF, 0x12, 0xCC, 0xC2, 0xFF, 0x12, 0xCC, 0xC2, 0xFF, 0x0D, 0x28, 0x26,
        0x9E, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x0A, 0x22, 0x21, 0x67, 0x0E, 0x4D, 0x4A, 0xC9, 0x02, 0x02, 0x02, 0x68, 0x10, 0x10, 0x45, 0x89, 0x02, 0x02, 0xF7, 0xFF, 0x00, 0x00, 0xFF,
        0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x06, 0x09, 0xD6, 0xFF, 0x0E, 0x54, 0x8D, 0xFF, 0x0E, 0x55, 0x7E, 0xFF, 0x11, 0x58, 0x65, 0xF0, 0x0E, 0x53, 0x4F, 0xCE,
        0x05, 0x07, 0x07, 0x7C, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x17, 0x0C, 0x0C, 0x26, 0x62, 0x07, 0x07, 0xE1, 0xFE, 0x19, 0x19, 0x91, 0xF5,
        0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0xEB, 0xFF, 0x19, 0x19, 0x72, 0xEF, 0x07, 0x07, 0xC6, 0xFC, 0x0A,
        0x0A, 0x19, 0x59, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x0E, 0x29, 0x62, 0x12, 0x12, 0x6F, 0xB5, 0x0E,
        0x0E, 0xA6, 0xE0, 0x09, 0x09, 0xDD, 0xFF, 0x03, 0x03, 0xF3, 0xFF, 0x0A, 0x0A, 0xD8, 0xFF, 0x06, 0x06, 0xE9, 0xFF, 0x0F, 0x0F, 0xA6, 0xE0, 0x12, 0x12,
        0x77, 0xBD, 0x0D, 0x0D, 0x1D, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x06, 0x1D, 0x10, 0x10,
        0x21, 0x61, 0x03, 0x03, 0xF5, 0xFF, 0x0F, 0x0F, 0xC0, 0xFA, 0x0B, 0x0B, 0xD1, 0xFE, 0x0E, 0x0E, 0xB8, 0xEF, 0x0D, 0x0D, 0xBA, 0xF1, 0x02, 0x02, 0xF9,
        0xFF, 0x13, 0x13, 0x31, 0x80, 0x03, 0x03, 0x03, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x02, 0x10, 0x10, 0x95, 0xD4, 0x00, 0x00, 0xFF, 0xFF, 0x0D, 0x0D, 0xAD, 0xE3, 0x0E, 0x0E, 0xAB, 0xE2, 0x0E, 0x0E, 0x9C, 0xD3, 0x0E, 0x0E, 0x95, 0xCC,
        0x00, 0x00, 0xFF, 0xFF, 0x0F, 0x0F, 0xAA, 0xE7, 0x01, 0x01, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x34, 0x6E, 0x05, 0x05, 0xEA, 0xFE, 0x0E, 0x0E, 0x9A, 0xD1, 0x0E, 0x0E, 0x82, 0xB9, 0x0E, 0x0E, 0x81, 0xB8, 0x0D,
        0x0D, 0x7C, 0xB3, 0x03, 0x03, 0xF2, 0xFF, 0x0F, 0x0F, 0x3B, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x48, 0x89, 0x0E, 0x0E, 0x85, 0xBE, 0x0E, 0x0E, 0x58, 0x8F, 0x0D, 0x0D,
        0x66, 0x9C, 0x0D, 0x0D, 0x6C, 0xA3, 0x11, 0x11, 0x60, 0xA3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x06, 0x17, 0x17, 0x29, 0x88, 0x0E, 0x0E, 0x2F,
        0x66, 0x0E, 0x0E, 0x4A, 0x81, 0x16, 0x16, 0x28, 0x81, 0x03, 0x03, 0x03, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x04,
        0x0C, 0x0C, 0x0E, 0x3D, 0x10, 0x10, 0x28, 0x66, 0x01, 0x01, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x13, 0x0F, 0x0F, 0x0F, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // GLFW_TODO: we will need to move all of these to an std::vector or similar if we want to support multiple windows in the future
    GLFWmonitor *monitor;                          // current monitor
    float monitorScaleX, monitorScaleY;            // current monitor content scale for DPI scaling
    GLFWwindow *window;                            // current window
    std::string windowTitle;                       // current window title
    int windowX, windowY;                          // current window position (in pixel coordinates)
    int windowWidth, windowHeight;                 // current window size (in pixel coordinates)
    float windowScaleX, windowScaleY;              // window scaling factors
    bool isWindowFullscreen;                       // whether the window is in fullscreen mode
    bool isWindowMaximized;                        // whether the window is currently maximized
    bool isWindowMinimized;                        // whether the window is currently minimized
    bool isWindowFocused;                          // whether the window is currently focused
    bool isWindowHidden;                           // whether the window is currently hidden
    bool isWindowFloating;                         // whether the window is currently floating
    float windowOpacity;                           // current window opacity
    bool isWindowBordered;                         // whether the window is currently bordered
    bool isWindowMousePassthrough;                 // whether the window is currently allowing mouse passthrough
    int windowedX, windowedY;                      // windowed mode position for restoring from fullscreen (in screen coordinates)
    int windowedWidth, windowedHeight;             // windowed mode size for restoring from fullscreen (in screen coordinates)
    int windowMinWidthLimit, windowMinHeightLimit; // current window size limits (in screen coordinates, -1 for no limit)
    int windowMaxWidthLimit, windowMaxHeightLimit; // current window size limits (in screen coordinates, -1 for no limit)
    int framebufferWidth, framebufferHeight;       // current framebuffer size (in pixel coordinates)
    std::tuple<int, int, int> screenMode;          // current screen mode (width, height, refresh rate)
    GLFWcursor *cursor;                            // current mouse cursor
    GLUTEnum_MouseCursorMode cursorMode;           // current mouse cursor mode (normal, hidden, disabled, captured)
    int keyboardModifiers;                         // current keyboard modifiers
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
    std::atomic_bool isMainLoopRunning;
    libqb_mutex *msgQueueMutex;
    std::queue<Message *> msgQueue;
};

std::tuple<int, int, int> GLUTEmu_ScreenGetMode() {
    return GLUTEmu::Instance().ScreenGetMode();
}

template <typename T> void GLUTEmu_WindowSetHint(GLUTEmu_WindowHint hint, const T value) {
    GLUTEmu::Instance().WindowSetHint<T>(hint, value);
}

// We only need to instantiate the template for the types we need
template void GLUTEmu_WindowSetHint<bool>(GLUTEmu_WindowHint, bool);
template void GLUTEmu_WindowSetHint<int>(GLUTEmu_WindowHint, int);
template void GLUTEmu_WindowSetHint<char *>(GLUTEmu_WindowHint, char *);
template void GLUTEmu_WindowSetHint<const char *>(GLUTEmu_WindowHint, const char *);
template void GLUTEmu_WindowSetHint<GLUTEmu_WindowHintValue>(GLUTEmu_WindowHint, GLUTEmu_WindowHintValue);

bool GLUTEmu_WindowCreate(int width, int height) {
    return GLUTEmu::Instance().WindowCreate(width, height);
}

bool GLUTEmu_WindowIsCreated() {
    return GLUTEmu::Instance().WindowIsCreated();
}

void GLUTEmu_WindowSetTitle(std::string_view title) {
    if (GLUTEmu::Instance().MessageIsMainThread() || (!GLUTEmu::Instance().WindowIsCreated() && !GLUTEmu::Instance().MainLoopIsRunning())) {
        GLUTEmu::Instance().WindowSetTitle(title);
    } else {
        GLUTEmu::MessageWindowSetTitle msg(title);
        GLUTEmu::Instance().MessageQueue(&msg);
        msg.WaitForResponse();
    }
}

std::string_view GLUTEmu_WindowGetTitle() {
    return GLUTEmu::Instance().WindowGetTitle();
}

void GLUTEmu_WindowSetIcon(int32_t largeImageHandle, int32_t smallImageHandle) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetIcon(largeImageHandle, smallImageHandle);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageWindowSetIcon(largeImageHandle, smallImageHandle));
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

void GLUTEmu_WindowSetMinimumSizeLimits(int minWidth, int minHeight) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetMinimumSizeLimits(minWidth, minHeight);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageSetWindowMinimumSizeLimits(minWidth, minHeight));
    }
}

void GLUTEmu_WindowSetMaximumSizeLimits(int maxWidth, int maxHeight) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        GLUTEmu::Instance().WindowSetMaximumSizeLimits(maxWidth, maxHeight);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageSetWindowMaximumSizeLimits(maxWidth, maxHeight));
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
    }
    GLUTEmu::MessageSetStandardCursor msg(style);
    GLUTEmu::Instance().MessageQueue(&msg);
    msg.WaitForResponse();
    return msg.responseValue;
}

bool GLUTEmu_MouseSetCustomCursor(int32_t imageHandle) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        return GLUTEmu::Instance().MouseSetCustomCursor(imageHandle);
    }
    GLUTEmu::MessageSetCustomCursor msg(imageHandle);
    GLUTEmu::Instance().MessageQueue(&msg);
    msg.WaitForResponse();
    return msg.responseValue;
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

double GLUTEmu_TimeGet() {
    return GLUTEmu::Instance().TimeGet();
}

void GLUTEmu_TimeSet(double time) {
    GLUTEmu::Instance().TimeSet(time);
}

void GLUTEmu_ProgramExit(int exitCode) {
    if (GLUTEmu::Instance().MessageIsMainThread()) {
        libqb_log_trace("Program exiting with code %d", exitCode);
        GLUTEmu::Instance().WindowSetCloseFunction(nullptr);
        GLUTEmu::Instance().WindowSetShouldClose(true);
        exit(exitCode);
    } else if (!GLUTEmu::Instance().MainLoopIsRunning()) {
        libqb_log_trace("Program exiting with code %d before main loop start", exitCode);
        exit(exitCode);
    } else {
        GLUTEmu::Instance().MessageQueue(new GLUTEmu::MessageProgramExit(exitCode));
    }
}
