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
                std::vector<uint32_t> imgLargeConvertedData(imgLarge->width * imgLarge->height);
                std::vector<uint32_t> imgSmallConvertedData(imgSmall->width * imgSmall->height);

                // Ensure both icons are in 32bpp RGBA format, converting if necessary
                auto convertTo32bpp = [](img_struct *img, std::vector<uint32_t> &converted) -> uint8_t * {
                    libqb_log_trace("Converting icon to 32bpp RGBA format");

                    if (img->bits_per_pixel != 32) {
                        auto src = img->offset;
                        for (size_t i = 0; i < converted.size(); i++) {
                            converted[i] = image_swap_red_blue(img->pal[*src]);
                            ++src;
                        }
                    } else {
                        auto src = img->offset32;
                        for (size_t i = 0; i < converted.size(); i++) {
                            converted[i] = image_swap_red_blue(*src);
                            ++src;
                        }
                    }

                    return reinterpret_cast<uint8_t *>(converted.data());
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
            libqb_log_trace("Big resource icon loaded successfully");
        } else {
            libqb_log_warn("Failed to load big resource icon");
        }

        if (hSmallIcon != nullptr) {
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hSmallIcon));
            libqb_log_trace("Small resource icon loaded successfully");
        } else {
            libqb_log_warn("Failed to load small resource icon");
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
        0,   0,   0,  0,   0,   0,   0,   0,   1,   1,   1,   5,   56,  7,  7,  85,  86,  18, 18, 150, 49,  81,  89,  190, 37,  71,  79,  139, 3,   4,  5,  15,
        6,   8,   25, 48,  17,  28,  110, 153, 27,  36,  111, 189, 79,  18, 23, 154, 55,  7,  7,  84,  1,   1,   1,   3,   0,   0,   0,   0,   0,   0,  0,  0,
        0,   0,   0,  0,   2,   2,   2,   9,   163, 9,   9,   201, 195, 6,  6,  219, 34,  53, 58, 121, 39,  75,  82,  147, 38,  74,  82,  146, 27,  50, 56, 103,
        10,  14,  50, 82,  25,  43,  170, 226, 25,  43,  173, 226, 14,  16, 46, 93,  190, 7,  7,  215, 150, 10,  10,  187, 2,   2,   2,   9,   0,   0,  0,  0,
        26,  5,   5,  45,  108, 11,  11,  151, 224, 7,   7,   252, 85,  7,  7,  113, 15,  27, 30, 63,  56,  111, 122, 206, 59,  120, 133, 210, 18,  32, 35, 77,
        10,  14,  48, 82,  24,  42,  167, 223, 25,  40,  156, 218, 10,  14, 49, 82,  94,  7,  7,  122, 226, 7,   7,   253, 113, 10,  10,  156, 27,  5,  5,  47,
        138, 7,   7,  167, 213, 6,   6,   234, 200, 9,   9,   238, 96,  7,  7,  125, 0,   0,  0,  0,   34,  26,  8,   71,  88,  78,  35,  180, 7,   12, 13, 35,
        1,   1,   1,  7,   77,  35,  13,  133, 85,  38,  13,  139, 0,   0,  0,  0,   114, 7,  7,  143, 196, 10,  10,  237, 218, 5,   5,   238, 137, 7,  7,  166,
        107, 12,  12, 156, 255, 0,   0,   255, 247, 2,   2,   255, 155, 8,  8,  187, 12,  9,  3,  27,  181, 133, 15,  224, 151, 112, 16,  205, 23,  18, 4,  43,
        67,  29,  9,  101, 156, 68,  19,  230, 148, 61,  13,  190, 2,   1,  1,  7,   157, 7,  7,  187, 246, 2,   2,   255, 255, 0,   0,   255, 117, 12, 12, 166,
        150, 15,  15, 207, 156, 14,  14,  211, 254, 0,   0,   255, 109, 15, 15, 170, 27,  20, 7,  58,  163, 120, 16,  214, 158, 116, 16,  209, 40,  30, 6,  66,
        65,  29,  9,  99,  126, 56,  18,  193, 169, 69,  15,  216, 10,  6,  5,  33,  107, 15, 15, 168, 253, 0,   0,   255, 172, 14,  14,  229, 164, 15, 15, 222,
        170, 7,   7,  200, 242, 3,   3,   254, 236, 5,   5,   255, 155, 13, 13, 207, 143, 10, 10, 184, 9,   4,   4,   29,  2,   2,   2,   14,  88,  8,  8,  120,
        157, 8,   8,  191, 51,  9,   9,   89,  5,   3,   3,   16,  130, 11, 11, 175, 142, 12, 12, 191, 233, 6,   6,   255, 246, 2,   2,   255, 176, 7,  7,  206,
        24,  4,   4,  41,  146, 14,  14,  205, 202, 9,   9,   239, 241, 4,  4,  255, 182, 15, 15, 242, 195, 11,  11,  238, 59,  14,  14,  116, 154, 11, 11, 198,
        231, 5,   5,  252, 29,  7,   7,   56,  190, 11,  11,  234, 174, 15, 15, 234, 239, 4,  4,  255, 200, 9,   9,   235, 140, 15,  15,  200, 21,  4,  4,  38,
        0,   0,   0,  0,   48,  6,   6,   73,  192, 10,  10,  233, 222, 8,  8,  255, 230, 6,  6,  254, 187, 125, 16,  255, 155, 149, 18,  236, 185, 68, 10, 246,
        231, 10,  4,  255, 168, 167, 18,  240, 164, 146, 20,  255, 189, 9,  7,  255, 225, 8,  8,  255, 176, 11,  11,  222, 39,  6,   6,   64,  0,   0,  0,  0,
        0,   0,   0,  0,   0,   0,   0,   0,   4,   2,   2,   14,  83,  7,  7,  112, 94,  17, 17, 164, 182, 189, 18,  252, 169, 130, 13,  255, 163, 98, 11, 255,
        217, 55,  8,  255, 190, 191, 15,  255, 125, 102, 16,  242, 94,  15, 15, 171, 71,  8,  8,  101, 1,   1,   1,   7,   0,   0,   0,   0,   0,   0,  0,  0,
        0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,   19,  20, 6,  50,  149, 157, 14,  231, 75,  40,  11,  175, 218, 1,  1,  255,
        214, 104, 10, 255, 138, 145, 16,  234, 63,  66,  12,  176, 0,   0,  0,  5,   0,   0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,
        0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,   8,   8,  2,  27,  29,  23,  7,   106, 172, 12,  12,  223, 255, 0,  0,  255,
        211, 23,  5,  255, 135, 51,  15,  247, 28,  25,  7,   108, 0,   0,  0,  2,   0,   0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,
        0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,   0,   0,  0,  0,   12,  5,   5,   32,  139, 13,  13,  189, 217, 9,  9,  254,
        204, 11,  11, 247, 146, 13,  13,  199, 8,   4,   4,   25,  0,   0,  0,  0,   0,   0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,
        0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,   0,   0,  0,  0,   0,   0,   0,   0,   173, 9,   9,   208, 157, 14, 14, 212,
        139, 14,  14, 194, 181, 8,   8,   215, 0,   0,   0,   2,   0,   0,  0,  0,   0,   0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,
        0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,   0,   0,  0,  0,   0,   0,   0,   0,   18,  4,   4,   36,  77,  16, 16, 143,
        81,  16,  16, 144, 25,  5,   5,   45,  0,   0,   0,   0,   0,   0,  0,  0,   0,   0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,
        0,   0,   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,   0,   0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   4,  4,  21,
        14,  8,   8,  45,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,   0,   0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0};

    static constexpr uint8_t Icon32bppRGBA32x32[] = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   11,  9,   9,   47,  60,  15,  15,  123, 38,  28,  28,  154, 20,  20,  21,  111, 9,   10,  10,  58,  1,   1,   1,   9,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   4,   4,   4,   27,  8,   8,   8,   51,  8,   8,   8,   51,  16,  16,  16,  95,  34,  30,  30,  157, 66,  18,
        18,  137, 16,  11,  11,  58,  0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   4,
        4,   18,  62,  15,  15,  121, 161, 15,  15,  218, 202, 13,  13,  251, 70,  32,  34,  179, 63,  121, 133, 238, 75,  155, 172, 255, 75,  155, 172, 255,
        63,  121, 134, 235, 12,  18,  19,  61,  0,   0,   0,   0,   0,   0,   0,   0,   20,  29,  95,  164, 27,  49,  220, 255, 27,  49,  208, 255, 27,  49,
        211, 255, 27,  47,  187, 250, 49,  28,  51,  179, 186, 13,  13,  241, 162, 15,  15,  220, 55,  15,  15,  113, 2,   2,   2,   11,  0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   44,  12,  12,  92,  193, 11,  11,  240, 255, 0,   0,   255, 218, 8,   8,   249, 29,  11,  11,  74,  49,  93,  102, 194, 73,  148, 164, 255,
        14,  19,  20,  78,  14,  18,  19,  77,  73,  147, 164, 255, 50,  94,  103, 196, 0,   0,   0,   0,   0,   0,   0,   0,   20,  29,  99,  164, 26,  50,
        224, 255, 21,  21,  21,  140, 20,  21,  27,  139, 27,  49,  221, 255, 22,  31,  110, 178, 16,  8,   8,   50,  203, 9,   9,   242, 253, 0,   0,   255,
        178, 14,  14,  231, 28,  10,  10,  67,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   4,   4,   4,   20,  3,   3,   3,   17,  162, 14,  14,  217, 255, 0,   0,   255, 252, 1,   1,   255, 55,  15,  15,  116, 0,   0,   0,   0,
        56,  107, 118, 215, 67,  131, 145, 253, 0,   0,   0,   0,   0,   0,   0,   0,   67,  130, 144, 252, 56,  109, 120, 217, 0,   0,   0,   0,   0,   0,
        0,   0,   20,  29,  99,  164, 26,  50,  230, 255, 27,  49,  207, 255, 27,  49,  212, 255, 26,  50,  230, 255, 19,  24,  65,  145, 0,   0,   0,   0,
        48,  15,  15,  107, 252, 1,   1,   255, 255, 0,   0,   255, 138, 15,  15,  195, 2,   2,   2,   10,  5,   5,   5,   27,  0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   3,   3,   3,   15,  134, 19,  19,  209, 10,  10,  10,  52,  194, 14,  14,  248, 255, 0,   0,   255, 192, 12,  12,  243,
        2,   2,   2,   8,   0,   0,   0,   0,   49,  93,  102, 194, 73,  148, 164, 255, 14,  19,  20,  78,  14,  18,  19,  77,  73,  148, 164, 255, 50,  94,
        103, 196, 0,   0,   0,   0,   0,   0,   0,   0,   20,  29,  99,  164, 26,  50,  224, 255, 19,  19,  19,  129, 16,  16,  17,  109, 28,  48,  201, 255,
        25,  38,  141, 211, 0,   0,   0,   0,   2,   2,   2,   11,  202, 12,  12,  250, 255, 0,   0,   255, 194, 14,  14,  248, 10,  10,  10,  49,  147, 18,
        18,  220, 4,   4,   4,   20,  0,   0,   0,   0,   0,   0,   0,   0,   99,  16,  16,  164, 254, 0,   0,   255, 32,  14,  14,  87,  194, 14,  14,  249,
        255, 0,   0,   255, 147, 13,  13,  202, 0,   0,   0,   0,   0,   0,   0,   0,   12,  17,  19,  60,  62,  121, 133, 235, 75,  155, 172, 255, 75,  156,
        173, 255, 77,  160, 178, 255, 23,  35,  38,  113, 0,   0,   0,   0,   0,   0,   0,   0,   20,  29,  95,  164, 27,  49,  219, 255, 27,  49,  208, 255,
        27,  49,  210, 255, 28,  48,  197, 254, 15,  20,  55,  117, 0,   0,   0,   0,   0,   0,   0,   0,   173, 13,  13,  228, 255, 0,   0,   255, 200, 13,
        13,  253, 42,  15,  15,  100, 255, 0,   0,   255, 103, 16,  16,  167, 0,   0,   0,   0,   10,  8,   8,   42,  227, 7,   7,   254, 255, 0,   0,   255,
        111, 15,  15,  171, 163, 14,  14,  219, 255, 0,   0,   255, 156, 14,  14,  212, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,
        1,   9,   11,  12,  12,  73,  27,  27,  23,  142, 51,  83,  89,  217, 29,  49,  53,  139, 0,   0,   0,   0,   0,   0,   0,   0,   4,   4,   4,   26,
        8,   8,   8,   51,  33,  22,  17,  124, 50,  30,  19,  150, 11,  9,   8,   57,  0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   4,   195, 13,
        13,  247, 255, 0,   0,   255, 160, 14,  14,  216, 126, 15,  15,  186, 255, 0,   0,   255, 224, 7,   7,   254, 8,   7,   7,   38,  60,  14,  14,  116,
        255, 0,   0,   255, 255, 0,   0,   255, 230, 6,   6,   254, 126, 25,  25,  224, 255, 0,   0,   255, 221, 8,   8,   253, 8,   7,   7,   36,  0,   0,
        0,   0,   0,   0,   0,   0,   1,   1,   1,   8,   122, 92,  18,  195, 228, 166, 15,  255, 49,  38,  12,  108, 0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   45,  23,  12,  103, 222, 89,  15,  254, 242, 94,  13,  255, 37,  20,  11,  94,  0,   0,   0,   0,   0,   0,
        0,   0,   20,  12,  12,  67,  244, 3,   3,   255, 255, 0,   0,   255, 113, 27,  27,  221, 237, 5,   5,   255, 255, 0,   0,   255, 255, 0,   0,   255,
        62,  14,  14,  118, 38,  13,  13,  90,  246, 2,   2,   255, 255, 0,   0,   255, 255, 0,   0,   255, 224, 8,   8,   255, 255, 0,   0,   255, 255, 0,
        0,   255, 94,  16,  16,  159, 0,   0,   0,   0,   0,   0,   0,   1,   95,  72,  17,  170, 241, 174, 13,  255, 178, 132, 19,  243, 59,  45,  13,  122,
        1,   1,   1,   9,   0,   0,   0,   0,   0,   0,   0,   0,   37,  20,  11,  90,  216, 86,  16,  253, 165, 71,  21,  246, 242, 94,  13,  255, 37,  20,
        11,  94,  0,   0,   0,   0,   0,   0,   0,   0,   98,  15,  15,  159, 255, 0,   0,   255, 249, 2,   2,   255, 227, 7,   7,   255, 255, 0,   0,   255,
        255, 0,   0,   255, 251, 1,   1,   255, 39,  14,  14,  94,  21,  17,  17,  89,  122, 17,  17,  188, 255, 0,   0,   255, 255, 0,   0,   255, 255, 0,
        0,   255, 255, 0,   0,   255, 243, 3,   3,   255, 28,  12,  12,  77,  0,   0,   0,   0,   46,  36,  12,  105, 235, 170, 14,  255, 154, 114, 17,  217,
        133, 100, 17,  200, 235, 170, 14,  255, 92,  69,  16,  161, 0,   0,   0,   0,   23,  14,  8,   65,  210, 84,  16,  250, 158, 70,  21,  243, 87,  44,
        20,  179, 242, 94,  13,  255, 68,  35,  18,  154, 6,   5,   4,   27,  0,   0,   0,   0,   29,  13,  13,  79,  245, 2,   2,   255, 255, 0,   0,   255,
        255, 0,   0,   255, 255, 0,   0,   255, 255, 0,   0,   255, 152, 15,  15,  213, 26,  19,  19,  101, 159, 21,  21,  245, 36,  16,  16,  98,  153, 15,
        15,  213, 255, 0,   0,   255, 255, 0,   0,   255, 255, 0,   0,   255, 178, 14,  14,  233, 12,  12,  12,  59,  0,   0,   0,   0,   85,  65,  14,  147,
        242, 175, 13,  255, 53,  42,  16,  135, 29,  24,  12,  94,  217, 159, 16,  255, 136, 101, 17,  200, 0,   0,   0,   0,   54,  28,  14,  119, 202, 83,
        18,  255, 202, 83,  18,  255, 213, 86,  17,  255, 242, 94,  13,  255, 208, 85,  17,  255, 29,  17,  11,  87,  0,   0,   0,   0,   12,  12,  12,  58,
        183, 14,  14,  237, 255, 0,   0,   255, 255, 0,   0,   255, 255, 0,   0,   255, 182, 13,  13,  231, 60,  21,  21,  144, 171, 20,  20,  251, 178, 14,
        14,  233, 228, 6,   6,   253, 84,  20,  20,  164, 131, 21,  21,  213, 250, 1,   1,   255, 255, 0,   0,   255, 150, 14,  14,  205, 100, 21,  21,  184,
        17,  9,   9,   53,  7,   6,   5,   32,  139, 104, 18,  210, 217, 158, 16,  255, 222, 162, 15,  255, 166, 122, 18,  231, 23,  19,  8,   64,  0,   0,
        0,   0,   1,   1,   1,   4,   2,   2,   2,   16,  18,  12,  12,  69,  69,  42,  27,  191, 192, 80,  19,  255, 32,  19,  13,  98,  0,   0,   0,   2,
        12,  8,   8,   43,  82,  22,  22,  172, 150, 14,  14,  205, 255, 0,   0,   255, 249, 2,   2,   255, 143, 22,  22,  231, 111, 22,  22,  199, 240, 4,
        4,   255, 185, 14,  14,  239, 133, 14,  14,  189, 255, 0,   0,   255, 255, 0,   0,   255, 205, 12,  12,  251, 177, 19,  19,  255, 255, 0,   0,   255,
        194, 12,  12,  243, 85,  17,  17,  153, 177, 12,  12,  227, 9,   7,   7,   36,  0,   0,   0,   1,   5,   5,   5,   35,  6,   6,   6,   42,  1,   1,
        1,   4,   0,   0,   0,   0,   14,  10,  10,  53,  59,  14,  14,  114, 86,  15,  15,  143, 154, 20,  20,  235, 4,   4,   4,   22,  0,   0,   0,   2,
        0,   0,   0,   0,   5,   5,   5,   25,  163, 14,  14,  218, 64,  16,  16,  127, 186, 13,  13,  238, 250, 1,   1,   255, 172, 21,  21,  255, 216, 10,
        10,  254, 255, 0,   0,   255, 255, 0,   0,   255, 144, 14,  14,  200, 44,  14,  14,  99,  249, 2,   2,   255, 255, 0,   0,   255, 255, 0,   0,   255,
        255, 0,   0,   255, 255, 0,   0,   255, 254, 0,   0,   255, 87,  22,  22,  176, 194, 11,  11,  239, 190, 11,  11,  235, 32,  12,  12,  80,  0,   0,
        0,   0,   0,   0,   0,   0,   2,   2,   2,   12,  105, 17,  17,  171, 235, 5,   5,   255, 255, 0,   0,   255, 227, 6,   6,   252, 47,  13,  13,  98,
        0,   0,   0,   0,   0,   0,   0,   0,   21,  10,  10,  61,  173, 13,  13,  225, 181, 13,  13,  232, 66,  19,  19,  144, 251, 1,   1,   255, 255, 0,
        0,   255, 255, 0,   0,   255, 255, 0,   0,   255, 255, 0,   0,   255, 251, 1,   1,   255, 56,  14,  14,  114, 1,   1,   1,   3,   94,  17,  17,  161,
        222, 8,   8,   254, 255, 0,   0,   255, 255, 0,   0,   255, 255, 0,   0,   255, 255, 0,   0,   255, 241, 3,   3,   255, 104, 26,  26,  211, 202, 12,
        12,  249, 239, 4,   4,   255, 125, 15,  15,  187, 16,  8,   8,   46,  79,  18,  18,  151, 131, 16,  16,  196, 201, 13,  13,  253, 255, 0,   0,   255,
        190, 13,  13,  244, 0,   0,   0,   1,   11,  6,   6,   37,  113, 15,  15,  173, 230, 6,   6,   254, 201, 11,  11,  245, 87,  25,  25,  187, 233, 5,
        5,   254, 255, 0,   0,   255, 255, 0,   0,   255, 255, 0,   0,   255, 255, 0,   0,   255, 210, 10,  10,  250, 85,  16,  16,  149, 1,   1,   1,   4,
        0,   0,   0,   0,   0,   0,   0,   0,   35,  23,  23,  127, 74,  27,  27,  184, 124, 21,  21,  208, 175, 15,  15,  236, 218, 9,   9,   255, 249, 2,
        2,   255, 253, 1,   1,   255, 170, 21,  21,  254, 167, 22,  22,  255, 248, 2,   2,   255, 119, 15,  15,  178, 21,  17,  17,  89,  34,  13,  13,  88,
        249, 2,   2,   255, 255, 0,   0,   255, 224, 8,   8,   255, 6,   6,   6,   31,  97,  15,  15,  156, 250, 1,   1,   255, 169, 21,  21,  255, 162, 22,
        22,  250, 250, 1,   1,   255, 251, 1,   1,   255, 218, 9,   9,   255, 171, 14,  14,  231, 118, 20,  20,  199, 64,  27,  27,  172, 32,  22,  22,  122,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   13,  8,   8,   44,  173, 13,  13,  228, 228, 7,   7,   255, 205, 12,
        12,  254, 192, 16,  16,  255, 189, 16,  16,  255, 246, 2,   2,   255, 255, 0,   0,   255, 195, 38,  12,  255, 153, 130, 23,  255, 165, 126, 20,  253,
        108, 113, 16,  182, 119, 43,  18,  218, 247, 0,   0,   255, 255, 0,   0,   255, 224, 5,   5,   255, 115, 120, 20,  202, 159, 125, 19,  248, 155, 129,
        23,  255, 172, 125, 18,  255, 183, 14,  8,   255, 252, 0,   0,   255, 197, 14,  14,  255, 191, 16,  16,  254, 203, 13,  13,  254, 223, 8,   8,   255,
        146, 17,  17,  211, 10,  7,   7,   37,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   4,
        4,   22,  104, 16,  16,  168, 232, 6,   6,   255, 255, 0,   0,   255, 252, 1,   1,   255, 181, 18,  18,  251, 237, 4,   4,   255, 184, 108, 15,  255,
        214, 226, 15,  255, 159, 159, 18,  255, 192, 200, 17,  255, 208, 219, 16,  255, 165, 9,   7,   255, 253, 0,   0,   255, 192, 36,  11,  255, 229, 242,
        13,  255, 171, 177, 19,  255, 167, 169, 18,  255, 160, 161, 19,  255, 118, 9,   9,   255, 204, 12,  12,  255, 255, 0,   0,   255, 255, 0,   0,   255,
        207, 10,  10,  247, 69,  15,  15,  131, 2,   2,   2,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   17,  9,   9,   55,  113, 15,  15,  173, 213, 10,  10,  251, 187, 12,  12,  234,
        34,  20,  20,  115, 159, 159, 20,  243, 164, 169, 19,  255, 126, 1,   1,   255, 146, 90,  16,  255, 222, 235, 14,  255, 122, 12,  9,   255, 249, 0,
        0,   255, 187, 85,  14,  255, 223, 236, 14,  255, 89,  55,  17,  255, 121, 48,  12,  255, 63,  32,  16,  218, 32,  17,  17,  140, 205, 10,  10,  245,
        195, 11,  11,  243, 86,  15,  15,  146, 6,   5,   5,   28,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        5,   5,   5,   25,  54,  14,  14,  111, 99,  21,  21,  194, 193, 204, 18,  255, 209, 221, 16,  255, 186, 196, 19,  255, 220, 232, 14,  255, 147, 143,
        19,  255, 161, 1,   1,   255, 252, 0,   0,   255, 182, 135, 17,  255, 227, 240, 13,  255, 222, 235, 14,  255, 222, 235, 14,  255, 93,  92,  20,  240,
        93,  18,  18,  196, 47,  14,  14,  104, 3,   3,   3,   15,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   17,  18,  10,  75,  226, 238, 13,  255, 119, 125, 18,  240, 82,  86,
        14,  210, 71,  61,  17,  227, 135, 3,   3,   255, 233, 0,   0,   255, 248, 1,   1,   255, 183, 183, 19,  255, 137, 144, 18,  247, 11,  11,  11,  179,
        14,  14,  14,  175, 4,   4,   4,   116, 0,   0,   0,   12,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   61,  63,  13,  127, 227, 240,
        13,  255, 22,  23,  10,  174, 1,   1,   1,   41,  148, 13,  13,  222, 248, 0,   0,   255, 255, 0,   0,   255, 211, 7,   7,   255, 214, 226, 15,  255,
        210, 222, 16,  255, 194, 204, 18,  255, 194, 204, 18,  255, 38,  40,  13,  158, 0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   33,  34,  10,  103, 74,  77,  14,  201, 2,   2,   2,   104, 69,  16,  16,  137, 247, 2,   2,   255, 255, 0,   0,   255, 255, 0,   0,   255,
        214, 9,   6,   255, 141, 84,  14,  255, 126, 85,  14,  255, 101, 88,  17,  240, 79,  83,  14,  206, 7,   7,   5,   124, 0,   0,   0,   7,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   6,   0,   0,   0,   23,  38,  12,  12,  98,  225, 7,   7,   254, 145, 25,  25,  245,
        255, 0,   0,   255, 255, 0,   0,   255, 253, 0,   0,   255, 235, 0,   0,   255, 114, 25,  25,  239, 198, 7,   7,   252, 25,  10,  10,  89,  0,   0,
        0,   14,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   41,  14,  14,  98,
        111, 18,  18,  181, 166, 14,  14,  224, 221, 9,   9,   255, 243, 3,   3,   255, 216, 10,  10,  255, 233, 6,   6,   255, 166, 15,  15,  224, 119, 18,
        18,  189, 29,  13,  13,  83,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   6,   6,   6,   29,  33,  16,  16,  97,  245, 3,   3,   255, 192, 15,  15,  250, 209, 11,  11,  254, 184, 14,  14,  239, 186, 13,
        13,  241, 249, 2,   2,   255, 49,  19,  19,  128, 3,   3,   3,   17,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   149, 16,  16,  212, 255, 0,   0,   255, 173, 13,  13,  227, 171, 14,
        14,  226, 156, 14,  14,  211, 149, 14,  14,  204, 255, 0,   0,   255, 170, 15,  15,  231, 1,   1,   1,   7,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   52,  15,  15,  110, 234, 5,
        5,   254, 154, 14,  14,  209, 130, 14,  14,  185, 129, 14,  14,  184, 124, 13,  13,  179, 242, 3,   3,   255, 59,  15,  15,  120, 0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   72,  16,  16,  137, 133, 14,  14,  190, 88,  14,  14,  143, 102, 13,  13,  156, 108, 13,  13,  163, 96,  17,  17,  163,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   6,   41,  23,  23,  136, 47,  14,  14,  102, 74,  14,  14,  129,
        40,  22,  22,  129, 3,   3,   3,   16,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   4,
        14,  12,  12,  61,  40,  16,  16,  102, 1,   1,   1,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   4,   4,   4,   19,  15,  15,  15,  73,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};

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
