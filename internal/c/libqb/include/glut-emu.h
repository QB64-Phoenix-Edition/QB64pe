//----------------------------------------------------------------------------------------------------------------------
// QB64-PE GLUT-like emulation layer
// This abstracts the underlying windowing library and provides a GLUT-like API for QB64-PE
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <cstdint>
#include <cuchar>
#include <string_view>
#include <tuple>
#include <utility>

enum class GLUTEmu_WindowHint : int {
    WindowResizable = GLFW_RESIZABLE,
    WindowVisible = GLFW_VISIBLE,
    WindowDecorated = GLFW_DECORATED,
    WindowFocused = GLFW_FOCUSED,
    WindowAutoIconify = GLFW_AUTO_ICONIFY,
    WindowFloating = GLFW_FLOATING,
    WindowMaximized = GLFW_MAXIMIZED,
    WindowCenterCursor = GLFW_CENTER_CURSOR,
    WindowTransparentFramebuffer = GLFW_TRANSPARENT_FRAMEBUFFER,
    WindowFocusOnShow = GLFW_FOCUS_ON_SHOW,
    WindowScaleToMonitor = GLFW_SCALE_TO_MONITOR,
    WindowScaleFramebuffer = GLFW_SCALE_FRAMEBUFFER,
    WindowMousePassThrough = GLFW_MOUSE_PASSTHROUGH,
    WindowPositionX = GLFW_POSITION_X,
    WindowPositionY = GLFW_POSITION_Y,
    FramebufferSamples = GLFW_SAMPLES,
    FramebufferDoubleBuffer = GLFW_DOUBLEBUFFER,
    MonitorRefreshRate = GLFW_REFRESH_RATE,
    ContextVersionMajor = GLFW_CONTEXT_VERSION_MAJOR,
    ContextVersionMinor = GLFW_CONTEXT_VERSION_MINOR,
    ContextOpenGLProfile = GLFW_OPENGL_PROFILE,
    Win32KeyboardMenu = GLFW_WIN32_KEYBOARD_MENU,
    Win32ShowDefault = GLFW_WIN32_SHOWDEFAULT,
    macOSCocoaFrameName = GLFW_COCOA_FRAME_NAME,
    macOSCocoaGraphicsSwitching = GLFW_COCOA_GRAPHICS_SWITCHING,
    LinuxX11ClassName = GLFW_X11_CLASS_NAME,
    LinuxX11InstanceName = GLFW_X11_INSTANCE_NAME
};

// GLFW_TODO: We need QB64 compatible keycodes here which should be mapped to/from GLFW keycodes
enum class GLUTEmu_KeyboardKey : int {
    Unknown = GLFW_KEY_UNKNOWN,
    Space = GLFW_KEY_SPACE,
    Apostrophe = GLFW_KEY_APOSTROPHE,
    Comma = GLFW_KEY_COMMA,
    Minus = GLFW_KEY_MINUS,
    Period = GLFW_KEY_PERIOD,
    Slash = GLFW_KEY_SLASH,
    Zero = GLFW_KEY_0,
    One = GLFW_KEY_1,
    Two = GLFW_KEY_2,
    Three = GLFW_KEY_3,
    Four = GLFW_KEY_4,
    Five = GLFW_KEY_5,
    Six = GLFW_KEY_6,
    Seven = GLFW_KEY_7,
    Eight = GLFW_KEY_8,
    Nine = GLFW_KEY_9,
    Semicolon = GLFW_KEY_SEMICOLON,
    Equal = GLFW_KEY_EQUAL,
    A = GLFW_KEY_A,
    B = GLFW_KEY_B,
    C = GLFW_KEY_C,
    D = GLFW_KEY_D,
    E = GLFW_KEY_E,
    F = GLFW_KEY_F,
    G = GLFW_KEY_G,
    H = GLFW_KEY_H,
    I = GLFW_KEY_I,
    J = GLFW_KEY_J,
    K = GLFW_KEY_K,
    L = GLFW_KEY_L,
    M = GLFW_KEY_M,
    N = GLFW_KEY_N,
    O = GLFW_KEY_O,
    P = GLFW_KEY_P,
    Q = GLFW_KEY_Q,
    R = GLFW_KEY_R,
    S = GLFW_KEY_S,
    T = GLFW_KEY_T,
    U = GLFW_KEY_U,
    V = GLFW_KEY_V,
    W = GLFW_KEY_W,
    X = GLFW_KEY_X,
    Y = GLFW_KEY_Y,
    Z = GLFW_KEY_Z,
    LeftBracket = GLFW_KEY_LEFT_BRACKET,
    Backslash = GLFW_KEY_BACKSLASH,
    RightBracket = GLFW_KEY_RIGHT_BRACKET,
    GraveAccent = GLFW_KEY_GRAVE_ACCENT,
    World1 = GLFW_KEY_WORLD_1,
    World2 = GLFW_KEY_WORLD_2,
    Escape = GLFW_KEY_ESCAPE,
    Enter = GLFW_KEY_ENTER,
    Tab = GLFW_KEY_TAB,
    Backspace = GLFW_KEY_BACKSPACE,
    Insert = GLFW_KEY_INSERT,
    Delete = GLFW_KEY_DELETE,
    Right = GLFW_KEY_RIGHT,
    Left = GLFW_KEY_LEFT,
    Down = GLFW_KEY_DOWN,
    Up = GLFW_KEY_UP,
    PageUp = GLFW_KEY_PAGE_UP,
    PageDown = GLFW_KEY_PAGE_DOWN,
    Home = GLFW_KEY_HOME,
    End = GLFW_KEY_END,
    CapsLock = GLFW_KEY_CAPS_LOCK,
    ScrollLock = GLFW_KEY_SCROLL_LOCK,
    NumLock = GLFW_KEY_NUM_LOCK,
    PrintScreen = GLFW_KEY_PRINT_SCREEN,
    Pause = GLFW_KEY_PAUSE,
    F1 = GLFW_KEY_F1,
    F2 = GLFW_KEY_F2,
    F3 = GLFW_KEY_F3,
    F4 = GLFW_KEY_F4,
    F5 = GLFW_KEY_F5,
    F6 = GLFW_KEY_F6,
    F7 = GLFW_KEY_F7,
    F8 = GLFW_KEY_F8,
    F9 = GLFW_KEY_F9,
    F10 = GLFW_KEY_F10,
    F11 = GLFW_KEY_F11,
    F12 = GLFW_KEY_F12,
    F13 = GLFW_KEY_F13,
    F14 = GLFW_KEY_F14,
    F15 = GLFW_KEY_F15,
    F16 = GLFW_KEY_F16,
    F17 = GLFW_KEY_F17,
    F18 = GLFW_KEY_F18,
    F19 = GLFW_KEY_F19,
    F20 = GLFW_KEY_F20,
    F21 = GLFW_KEY_F21,
    F22 = GLFW_KEY_F22,
    F23 = GLFW_KEY_F23,
    F24 = GLFW_KEY_F24,
    F25 = GLFW_KEY_F25,
    KP0 = GLFW_KEY_KP_0,
    KP1 = GLFW_KEY_KP_1,
    KP2 = GLFW_KEY_KP_2,
    KP3 = GLFW_KEY_KP_3,
    KP4 = GLFW_KEY_KP_4,
    KP5 = GLFW_KEY_KP_5,
    KP6 = GLFW_KEY_KP_6,
    KP7 = GLFW_KEY_KP_7,
    KP8 = GLFW_KEY_KP_8,
    KP9 = GLFW_KEY_KP_9,
    KPDecimal = GLFW_KEY_KP_DECIMAL,
    KPDivide = GLFW_KEY_KP_DIVIDE,
    KPMultiply = GLFW_KEY_KP_MULTIPLY,
    KPSubtract = GLFW_KEY_KP_SUBTRACT,
    KPAdd = GLFW_KEY_KP_ADD,
    KPEnter = GLFW_KEY_KP_ENTER,
    KPEqual = GLFW_KEY_KP_EQUAL,
    LeftShift = GLFW_KEY_LEFT_SHIFT,
    LeftControl = GLFW_KEY_LEFT_CONTROL,
    LeftAlt = GLFW_KEY_LEFT_ALT,
    LeftSuper = GLFW_KEY_LEFT_SUPER,
    RightShift = GLFW_KEY_RIGHT_SHIFT,
    RightControl = GLFW_KEY_RIGHT_CONTROL,
    RightAlt = GLFW_KEY_RIGHT_ALT,
    RightSuper = GLFW_KEY_RIGHT_SUPER,
    Menu = GLFW_KEY_MENU,
    Last = GLFW_KEY_LAST
};

enum GLUTEmu_KeyboardKeyModifier : int {
    Shift = GLFW_MOD_SHIFT,
    Control = GLFW_MOD_CONTROL,
    Alt = GLFW_MOD_ALT,
    Super = GLFW_MOD_SUPER,
    CapsLock = GLFW_MOD_CAPS_LOCK,
    NumLock = GLFW_MOD_NUM_LOCK,
    ScrollLock = 1 << 16 // GLFW does not define a ScrollLock modifier, so this is a workaround
};

enum class GLUTEmu_MouseStandardCursor : int {
    Arrow = GLFW_ARROW_CURSOR,
    IBeam = GLFW_IBEAM_CURSOR,
    Crosshair = GLFW_CROSSHAIR_CURSOR,
    PointingHand = GLFW_POINTING_HAND_CURSOR,
    ResizeEW = GLFW_RESIZE_EW_CURSOR,
    ResizeNS = GLFW_RESIZE_NS_CURSOR,
    ResizeNWSE = GLFW_RESIZE_NWSE_CURSOR,
    ResizeNESW = GLFW_RESIZE_NESW_CURSOR,
    ResizeAll = GLFW_RESIZE_ALL_CURSOR,
    NotAllowed = GLFW_NOT_ALLOWED_CURSOR
};

enum class GLUTEnum_MouseCursorMode : int {
    Normal = GLFW_CURSOR_NORMAL,
    Hidden = GLFW_CURSOR_HIDDEN,
    Disabled = GLFW_CURSOR_DISABLED,
    Captured = GLFW_CURSOR_CAPTURED
};

enum class GLUTEmu_MouseButton : int {
    Left = GLFW_MOUSE_BUTTON_LEFT,
    Right = GLFW_MOUSE_BUTTON_RIGHT,
    Middle = GLFW_MOUSE_BUTTON_MIDDLE,
    Four = GLFW_MOUSE_BUTTON_4,
    Five = GLFW_MOUSE_BUTTON_5,
    Six = GLFW_MOUSE_BUTTON_6,
    Seven = GLFW_MOUSE_BUTTON_7,
    Eight = GLFW_MOUSE_BUTTON_8
};

enum class GLUTEmu_ButtonAction : int { Released = GLFW_RELEASE, Pressed = GLFW_PRESS, Repeated = GLFW_REPEAT };

typedef void (*GLUTEmu_CallbackWindowClose)();
typedef void (*GLUTEmu_CallbackWindowResized)(int width, int height);
typedef void (*GLUTEmu_CallbackWindowFramebufferResized)(int width, int height);
typedef void (*GLUTEmu_CallbackWindowMaximized)(int width, int height, bool maximized);
typedef void (*GLUTEmu_CallbackWindowMinimized)(int width, int height, bool minimized);
typedef void (*GLUTEmu_CallbackWindowFocused)(bool focused);
typedef void (*GLUTEmu_CallbackWindowRefresh)();
typedef void (*GLUTEmu_CallbackWindowIdle)();
typedef void (*GLUTEmu_CallbackKeyboardButton)(GLUTEmu_KeyboardKey key, int scancode, GLUTEmu_ButtonAction action, int modifiers);
typedef void (*GLUTEmu_CallbackKeyboardCharacter)(char32_t codepoint);
typedef void (*GLUTEmu_CallbackMousePosition)(double x, double y, GLUTEnum_MouseCursorMode mode);
typedef void (*GLUTEmu_CallbackMouseButton)(double x, double y, GLUTEmu_MouseButton button, GLUTEmu_ButtonAction action, GLUTEnum_MouseCursorMode mode,
                                            int modifiers);
typedef void (*GLUTEmu_CallbackMouseNotify)(double x, double y, bool entered, GLUTEnum_MouseCursorMode mode);
typedef void (*GLUTEmu_CallbackMouseScroll)(double x, double y, double xOffset, double yOffset, GLUTEnum_MouseCursorMode mode);
typedef void (*GLUTEmu_CallbackDropFiles)(int count, const char *paths[]);

std::tuple<int, int, int> GLUTEmu_ScreenGetMode();
template <typename T> void GLUTEmu_WindowSetHint(GLUTEmu_WindowHint hint, const T value);
bool GLUTEmu_WindowCreate(const char *title, int width, int height);
bool GLUTEmu_WindowIsCreated();
void GLUTEmu_WindowSetTitle(const char *title);
std::string_view GLUTEmu_WindowGetTitle();
void GLUTEmu_WindowSetIcon(int32_t imageHandle);
void GLUTEmu_WindowFullScreen(bool fullscreen);
bool GLUTEmu_WindowIsFullscreen();
void GLUTEmu_WindowMaximize();
bool GLUTEmu_WindowIsMaximized();
void GLUTEmu_WindowMinimize();
bool GLUTEmu_WindowIsMinimized();
void GLUTEmu_WindowRestore();
bool GLUTEmu_WindowIsRestored();
void GLUTEmu_WindowHide(bool hide);
bool GLUTEmu_WindowIsHidden();
void GLUTEmu_WindowFocus();
bool GLUTEmu_WindowIsFocused();
void GLUTEmu_WindowSetFloating(bool floating);
bool GLUTEmu_WindowIsFloating();
void GLUTEmu_WindowSetOpacity(float opacity);
float GLUTEmu_WindowGetOpacity();
void GLUTEmu_WindowSetBordered(bool bordered);
bool GLUTEmu_WindowIsBordered();
void GLUTEmu_WindowSetMousePassthrough(bool passthrough);
bool GLUTEmu_WindowAllowsMousePassthrough();
void GLUTEmu_WindowResize(int width, int height);
std::pair<int, int> GLUTEmu_WindowGetSize();
std::pair<int, int> GLUTEmu_WindowGetFramebufferSize();
void GLUTEmu_WindowMove(int x, int y);
std::pair<int, int> GLUTEmu_WindowGetPosition();
void GLUTEmu_WindowCenter();
void GLUTEmu_WindowSetAspectRatio(int width, int height);
void GLUTEmu_WindowSetSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight);
void GLUTEmu_WindowSetShouldClose(bool shouldClose);
void GLUTEmu_WindowSwapBuffers();
void GLUTEmu_WindowRefresh();
const void *GLUTEmu_WindowGetNativeHandle(int32_t type);
void GLUTEmu_WindowSetCloseFunction(GLUTEmu_CallbackWindowClose func);
void GLUTEmu_WindowSetResizedFunction(GLUTEmu_CallbackWindowResized func);
void GLUTEmu_WindowSetFramebufferResizedFunction(GLUTEmu_CallbackWindowFramebufferResized func);
void GLUTEmu_WindowSetMaximizedFunction(GLUTEmu_CallbackWindowMaximized func);
void GLUTEmu_WindowSetMinimizedFunction(GLUTEmu_CallbackWindowMinimized func);
void GLUTEmu_WindowSetFocusedFunction(GLUTEmu_CallbackWindowFocused func);
void GLUTEmu_WindowSetRefreshFunction(GLUTEmu_CallbackWindowRefresh func);
void GLUTEmu_WindowSetIdleFunction(GLUTEmu_CallbackWindowIdle func);
void GLUTEmu_KeyboardSetButtonFunction(GLUTEmu_CallbackKeyboardButton func);
void GLUTEmu_KeyboardSetCharacterFunction(GLUTEmu_CallbackKeyboardCharacter func);
bool GLUTEmu_KeyboardIsKeyModifierSet(GLUTEmu_KeyboardKeyModifier modifier);
bool GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor style);
bool GLUTEmu_MouseSetCustomCursor(int32_t imageHandle);
void GLUTEmu_MouseSetCursorMode(GLUTEnum_MouseCursorMode mode);
GLUTEnum_MouseCursorMode GLUTEmu_MouseGetCursorMode();
void GLUTEmu_MouseMove(double x, double y);
void GLUTEmu_MouseSetPositionFunction(GLUTEmu_CallbackMousePosition func);
void GLUTEmu_MouseSetButtonFunction(GLUTEmu_CallbackMouseButton func);
void GLUTEmu_MouseSetNotifyFunction(GLUTEmu_CallbackMouseNotify func);
void GLUTEmu_MouseSetScrollFunction(GLUTEmu_CallbackMouseScroll func);
void GLUTEmu_DropSetFilesFunction(GLUTEmu_CallbackDropFiles func);
void GLUTEmu_MainLoop();
void GLUTEmu_ProgramExit(int exitCode);
