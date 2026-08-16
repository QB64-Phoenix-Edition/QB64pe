#include "libqb-common.h"

#include "console.h"
#include "error_handle.h"
#include "game_controller.h"
#include "glut-emu.h"
#include "graphics.h"
#include "main-thread.h"
#include "mouse.h"
#include "ring-buffer.h"
#include "rounding.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>

extern int32_t environment_2d__screen_width;
extern int32_t environment_2d__screen_height;
extern int32_t environment_2d__screen_x1;
extern int32_t environment_2d__screen_y1;
extern float environment_2d__screen_x_scale;
extern float environment_2d__screen_y_scale;

extern int32_t x_monitor;
extern int32_t y_monitor;

int32_t x_offset = 0;
int32_t y_offset = 0;
int32_t x_limit = 0;
int32_t y_limit = 0;

static constexpr auto MouseRingBufferCapacity = 65536u; // Must be a power of two and >= 2

// Mouse event produced by the GUI/GLFW callbacks and consumed by _MOUSEINPUT.
// x/y are raw window pixel coordinates.
struct mouse_event {
    double x;
    double y;
    double movementx; // relative movement delta (for raw / disabled mouse mode)
    double movementy; // relative movement delta (for raw / disabled mouse mode)
    uint32_t buttons; // button N uses bit (N - 1): 1=left, 2=right, 3=middle, 4+=extra
    double wheelX;    // horizontal wheel delta since latest _MOUSEINPUT event
    double wheelY;    // vertical wheel delta since latest _MOUSEINPUT event
};

static RingBuffer<mouse_event, MouseRingBufferCapacity, true> mouse_event_queue;
// Last event popped by _MOUSEINPUT.
static mouse_event current_gui_state{};
// Latest pushed GUI event — used by mouse_get_int33_status without requiring _MOUSEINPUT.
static mouse_event last_gui_pushed{};

// Dedicated tracker for computing relative mouse movement.
static double g_lastRawMouseX = 0.0;
static double g_lastRawMouseY = 0.0;
static bool g_lastRawMouseValid = false;
static GLUTEnum_MouseCursorMode g_lastRawMouseMode = GLUTEnum_MouseCursorMode::Normal;

// _MOUSEMOVE uses glfwSetCursorPos, which would otherwise be reported as a large mouse movement on the next position callback.
static bool g_mouseWarpPending = false;
static double g_mouseWarpX = 0.0;
static double g_mouseWarpY = 0.0;

static int32_t MouseCanonicalToDeviceButtonIndex(int32_t buttonNumber) {
    // _BUTTON mouse numbering: 1=left, 2=right, 3=middle.
    if (buttonNumber == 2)
        return 2; // right -> device index 2
    if (buttonNumber == 3)
        return 1; // middle -> device index 1
    return buttonNumber - 1;
}

void sub__mousehide(int32_t disable) {
#ifndef DEPENDENCY_CONSOLE_ONLY
    OPTIONAL_GLUT();
    if (disable == 2) {
        GLUTEmu_MouseSetCursorMode(GLUTEnum_MouseCursorMode::Disabled);
    } else {
        GLUTEmu_MouseSetCursorMode(GLUTEnum_MouseCursorMode::Hidden);
    }
#else
    (void)disable;
#endif
}

void sub__mouseshow(qbs *qbsStyle, int32_t passed) {
    if (is_error_pending())
        return;

#ifndef DEPENDENCY_CONSOLE_ONLY
    OPTIONAL_GLUT();

    // GLFW_TODO: We should extend sub__mouseshow to accept a parameter to show/hide the cursor instead of always showing it
    GLUTEmu_MouseSetCursorMode(GLUTEnum_MouseCursorMode::Normal);

    std::string style;
    if (passed && qbsStyle) {
        style.assign(reinterpret_cast<char *>(qbsStyle->chr), qbsStyle->len);
        std::transform(style.begin(), style.end(), style.begin(), ::toupper);
    } else {
        style = "DEFAULT";
    }

    if (style == "DEFAULT" || style == "ARROW") {
        GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::Arrow);
    } else if (style == "LINK" || style == "HELP" || style == "POINTINGHAND") {
        GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::PointingHand);
    } else if (style == "TEXT" || style == "IBEAM") {
        GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::IBeam);
    } else if (style == "CROSSHAIR") {
        GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::Crosshair);
    } else if (style == "VERTICAL" || style == "RESIZENS") {
        GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::ResizeNS);
    } else if (style == "HORIZONTAL" || style == "RESIZEEW") {
        GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::ResizeEW);
    } else if (style == "TOPLEFT_BOTTOMRIGHT" || style == "RESIZENESW") {
        GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::ResizeNESW);
    } else if (style == "TOPRIGHT_BOTTOMLEFT" || style == "RESIZENWSE") {
        GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::ResizeNWSE);
    } else if (style == "WAIT" || style == "NOTALLOWED") {
        GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::NotAllowed);
    } else if (style == "CYCLE" || style == "MOVE" || style == "RESIZEALL") {
        GLUTEmu_MouseSetStandardCursor(GLUTEmu_MouseStandardCursor::ResizeAll);
    } else {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
    }
#else
    (void)qbsStyle;
    (void)passed;
#endif
}

int32_t func__mousehidden() {
#ifndef DEPENDENCY_CONSOLE_ONLY
    OPTIONAL_GLUT(QB_FALSE);
    return QB_BOOL(GLUTEmu_MouseGetCursorMode() == GLUTEnum_MouseCursorMode::Hidden);
#else
    return QB_FALSE;
#endif
}

int32_t func__mousedisabled() {
#ifndef DEPENDENCY_CONSOLE_ONLY
    OPTIONAL_GLUT(QB_FALSE);
    return QB_BOOL(GLUTEmu_MouseGetCursorMode() == GLUTEnum_MouseCursorMode::Disabled);
#else
    return QB_FALSE;
#endif
}

double func__mousemovementx() {
    if (Image_IsSourceConsolePage())
        return (double)func__console_mouse_movementx();
    return (double)current_gui_state.movementx;
}

double func__mousemovementy() {
    if (Image_IsSourceConsolePage())
        return (double)func__console_mouse_movementy();
    return (double)current_gui_state.movementy;
}

void sub__mousemove(double x, double y) {
    OPTIONAL_GLUT();

    int32_t x2, y2, sx, sy;
    int32_t logicalX = 0;
    int32_t logicalY = 0;
    if (display_page->text) {
        sx = fontwidth[display_page->font] * display_page->width;
        sy = fontheight[display_page->font] * display_page->height;
        if (x < 0.5)
            goto error;
        if (y < 0.5)
            goto error;
        if (x > ((double)display_page->width) + 0.5)
            goto error;
        if (y > ((double)display_page->height) + 0.5)
            goto error;
        x -= 0.5;
        y -= 0.5;
        x = x * (double)fontwidth[display_page->font];
        y = y * (double)fontheight[display_page->font];
        x2 = qbr_double_to_long(x);
        y2 = qbr_double_to_long(y);
        if (x2 < 0)
            x2 = 0;
        if (y2 < 0)
            y2 = 0;
        if (x2 > sx - 1)
            x2 = sx - 1;
        if (y2 > sy - 1)
            y2 = sy - 1;
    } else {
        sx = display_page->width;
        sy = display_page->height;
        x2 = qbr_double_to_long(x);
        y2 = qbr_double_to_long(y);
        if (x2 < 0)
            goto error;
        if (y2 < 0)
            goto error;
        if (x2 > sx - 1)
            goto error;
        if (y2 > sy - 1)
            goto error;
    }

    // x2,y2 are logical pixel co-ordinates
    logicalX = x2;
    logicalY = y2;

    if (GLUTEmu_MouseGetCursorMode() == GLUTEnum_MouseCursorMode::Disabled) {
        // In disabled mode there is no visible cursor.
        GLUTEmu_MouseMove(logicalX, logicalY);

        g_mouseWarpPending = true;
        g_mouseWarpX = logicalX;
        g_mouseWarpY = logicalY;

        return;
    }

    // adjust for fullscreen position as necessary:
    x2 *= environment_2d__screen_x_scale;
    y2 *= environment_2d__screen_y_scale;
    x2 += environment_2d__screen_x1;
    y2 += environment_2d__screen_y1;

    GLUTEmu_MouseMove(x2, y2);

    // Remember the warp destination so the resulting position callback is not reported as mouse movement.
    g_mouseWarpPending = true;
    g_mouseWarpX = x2;
    g_mouseWarpY = y2;

    return;

error:
    error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
}

double func__mousex() {
    int32_t x, x2;
    double f;

    if (Image_IsSourceConsolePage()) {
        return func__console_mouse_x();
    }

    x = (int32_t)current_gui_state.x;

    // calculate pixel offset of mouse within SCREEN using environment variables
    x -= environment_2d__screen_x1;
    x = qbr_double_to_long((((double)x + 0.5) / environment_2d__screen_x_scale) - 0.5);
    if (x < 0)
        x = 0;
    if (x >= environment_2d__screen_width)
        x = environment_2d__screen_width - 1;

    // restrict range to the current display page's range to avoid causing errors
    x2 = display_page->width;
    if (display_page->text) {
        x2 *= fontwidth[display_page->font];
    }
    if (x >= x2)
        x = x2 - 1;

    if (display_page->text) {
        f = x;
        x2 = fontwidth[display_page->font];
        f = f / (double)x2 + 0.5;
        x2 = qbr_double_to_long(f);
        if (x2 > x)
            f -= 0.001;
        if (x2 < x)
            f += 0.001;
        return std::floor(f + 0.5);
    }

    return x;
}

double func__mousey() {
    int32_t y, y2;
    double f;

    if (Image_IsSourceConsolePage()) {
        return func__console_mouse_y();
    }

    y = (int32_t)current_gui_state.y;

    // calculate pixel offset of mouse within SCREEN using environment variables
    y -= environment_2d__screen_y1;
    y = qbr_double_to_long((((double)y + 0.5) / environment_2d__screen_y_scale) - 0.5);
    if (y < 0)
        y = 0;
    if (y >= environment_2d__screen_height)
        y = environment_2d__screen_height - 1;

    // restrict range to the current display page's range to avoid causing errors
    y2 = display_page->height;
    if (display_page->text) {
        y2 *= fontheight[display_page->font];
    }
    if (y >= y2)
        y = y2 - 1;

    if (display_page->text) {
        f = y;
        y2 = fontheight[display_page->font];
        f = f / (double)y2 + 0.5;
        y2 = qbr_double_to_long(f);
        if (y2 > y)
            f -= 0.001;
        if (y2 < y)
            f += 0.001;
        return std::floor(f + 0.5);
    }

    return y;
}

int32_t func__mouseinput() {
    mouse_event evt;
    if (mouse_event_queue.Pop(evt)) {
        current_gui_state = evt;
        return QB_TRUE;
    }

    return QB_FALSE;
}

int32_t func__mousebutton(int32_t i) {
    if (i < 1 || i > Mouse_MaxSupportedButtons) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return 0;
    }

    if (Image_IsSourceConsolePage()) {
        // Windows console button state is in the low bits of g_consoleMouseButtons.
        // It can report up to 5 buttons; buttons beyond that simply read as 0.
        return (func__console_mouse_buttons() & (1u << (i - 1))) ? -1 : 0;
    }

    // 1=left, 2=right, 3=middle, 4+=extra.
    // bit 0=left, bit 1=right, bit 2=middle, bit 3+=extra — no swap needed.
    return (current_gui_state.buttons & (1u << (i - 1))) ? -1 : 0;
}

double func__mousewheel(int32_t axis, int32_t passed) {
    if (Image_IsSourceConsolePage()) {
        if (func__console_mouse_buttons() < -0x100)
            return -1;
        if (func__console_mouse_buttons() > 0x100)
            return 1;
        return 0;
    }

    // With no argument (or axis==0): return Y delta. With any non-zero axis: return X delta.
    if (passed && axis != 0) {
        return current_gui_state.wheelX;
    }

    return current_gui_state.wheelY;
}

void mouse_get_int33_status(uint16_t *buttons, double *x, double *y) {
    if (Image_IsSourceConsolePage()) {
        // bit 0=left, bit 1=right, bit 2=middle.
        *buttons = (uint16_t)(func__console_mouse_buttons() & 7);
        *x = func__console_mouse_x();
        *y = func__console_mouse_y();
        return;
    }

    // Use the latest pushed GUI event so callers don't need _MOUSEINPUT first.
    // bit 0=left, bit 1=right, bit 2=middle.
    *buttons = (uint16_t)(last_gui_pushed.buttons & 7);

    // Temporarily install the latest push as the "current" event so that
    // func__mousex/y apply the correct coordinate transform.
    const mouse_event savedState = current_gui_state;
    current_gui_state = last_gui_pushed;
    *x = func__mousex();
    *y = func__mousey();
    current_gui_state = savedState;
}

void Mouse_QueueButtonUpEvent(int button, double x, double y) {
    mouse_event event{};
    event.x = x;
    event.y = y;
    event.buttons = last_gui_pushed.buttons;
    if (event.buttons & (1u << (button - 1)))
        event.buttons ^= (1u << (button - 1));
    mouse_event_queue.Push(event);
    last_gui_pushed = event;

    if (device_last && button >= 1 && button <= Mouse_MaxSupportedButtons) {
        device_struct *d = &devices[2];
        if (button <= d->lastbutton) {
            const int32_t deviceIdx = MouseCanonicalToDeviceButtonIndex(button);
            int32_t eventIndex = createDeviceEvent(d);
            setDeviceEventButtonValue(d, eventIndex, deviceIdx, 0);
            commitDeviceEvent(d);
        }
    }
}

void Mouse_QueueButtonDownEvent(int button, double x, double y) {
    mouse_event event{};
    event.x = x;
    event.y = y;
    event.buttons = last_gui_pushed.buttons;
    event.buttons |= (1u << (button - 1));
    mouse_event_queue.Push(event);
    last_gui_pushed = event;

    if (device_last && button >= 1 && button <= Mouse_MaxSupportedButtons) {
        device_struct *d = &devices[2];
        if (button <= d->lastbutton) {
            const int32_t deviceIdx = MouseCanonicalToDeviceButtonIndex(button);
            int32_t eventIndex = createDeviceEvent(d);
            setDeviceEventButtonValue(d, eventIndex, deviceIdx, 1);
            commitDeviceEvent(d);
        }
    }
}

void Mouse_QueuePositionEvent(double x, double y, GLUTEnum_MouseCursorMode mode) {
    const double currentPixelX = (mode == GLUTEnum_MouseCursorMode::Disabled) ? x * (double)environment_2d__screen_x_scale : x;
    const double currentPixelY = (mode == GLUTEnum_MouseCursorMode::Disabled) ? y * (double)environment_2d__screen_y_scale : y;

    if (g_mouseWarpPending) {
        if (mode == GLUTEnum_MouseCursorMode::Disabled) {
            g_lastRawMouseValid = false;
            g_mouseWarpPending = false;
        } else if (std::abs(currentPixelX - g_mouseWarpX) < 2.0 && std::abs(currentPixelY - g_mouseWarpY) < 2.0) {
            g_lastRawMouseX = currentPixelX;
            g_lastRawMouseY = currentPixelY;
            g_lastRawMouseValid = true;
            g_lastRawMouseMode = mode;
            g_mouseWarpPending = false;
        } else {
            g_mouseWarpPending = false;
        }
    }

    if (mode != g_lastRawMouseMode) {
        g_lastRawMouseValid = false;
        g_lastRawMouseMode = mode;
    }

    double movementx = 0.0;
    double movementy = 0.0;
    if (g_lastRawMouseValid) {
        movementx = currentPixelX - g_lastRawMouseX;
        movementy = currentPixelY - g_lastRawMouseY;
    } else {
        g_lastRawMouseValid = true;
    }
    g_lastRawMouseX = currentPixelX;
    g_lastRawMouseY = currentPixelY;

    mouse_event event{};
    if (mode == GLUTEnum_MouseCursorMode::Disabled) {
        event.x = 0.0;
        event.y = 0.0;
    } else {
        event.x = x;
        event.y = y;
    }
    event.movementx = movementx;
    event.movementy = movementy;
    event.buttons = last_gui_pushed.buttons;
    mouse_event_queue.Push(event);
    last_gui_pushed = event;

    // Push a second event to clear movement values, matching the old behavior.
    mouse_event reset{};
    reset.x = event.x;
    reset.y = event.y;
    reset.buttons = event.buttons;
    mouse_event_queue.Push(reset);
    last_gui_pushed = reset;

    if (device_last) {
        device_struct *d = &devices[2]; // mouse

        // Report position on the axes and movement on wheels 0/1.
        int32_t eventIndex = createDeviceEvent(d);
        if (mode != GLUTEnum_MouseCursorMode::Disabled) {
            double fx = x;
            fx -= x_offset;
            int32_t z = x_monitor - x_offset * 2;
            if (fx < 0)
                fx = 0;
            if (fx >= z)
                fx = z - 1;
            fx = fx / (double)(z - 1); // 0 to 1
            fx *= 2.0;                 // 0 to 2
            fx -= 1.0;                 // -1 to 1

            double fy = y;
            fy -= y_offset;
            z = y_monitor - y_offset * 2;
            if (fy < 0)
                fy = 0;
            if (fy >= z)
                fy = z - 1;
            fy = fy / (double)(z - 1); // 0 to 1
            fy *= 2.0;                 // 0 to 2
            fy -= 1.0;                 // -1 to 1

            setDeviceEventAxisValue(d, eventIndex, 0, fx);
            setDeviceEventAxisValue(d, eventIndex, 1, fy);
        }
        setDeviceEventWheelValue(d, eventIndex, 0, movementx);
        setDeviceEventWheelValue(d, eventIndex, 1, movementy);
        commitDeviceEvent(d);

        // Reset event: keep the position/axis values, but clear the movement wheels.
        eventIndex = createDeviceEvent(d);
        setDeviceEventWheelValue(d, eventIndex, 0, 0.0);
        setDeviceEventWheelValue(d, eventIndex, 1, 0.0);
        commitDeviceEvent(d);
    }
}

void Mouse_QueueScrollEvent(double x, double y, double xOffset, double yOffset) {
    // GLFW convention: positive yOffset = scroll up, positive xOffset = scroll right.
    // QB64 convention for _MOUSEWHEEL: negative = up, positive = down.
    // negate Y so that scrolling up gives a negative value as expected.
    mouse_event event{};
    event.x = x;
    event.y = y;
    event.buttons = last_gui_pushed.buttons;
    event.wheelX = xOffset;
    event.wheelY = -yOffset;
    mouse_event_queue.Push(event);
    last_gui_pushed = event;

    // Reset wheel so _MOUSEWHEEL returns 0 on the following _MOUSEINPUT call.
    mouse_event reset{};
    reset.x = x;
    reset.y = y;
    reset.buttons = last_gui_pushed.buttons;
    mouse_event_queue.Push(reset);
    last_gui_pushed = reset;

    if (device_last) {
        device_struct *d = &devices[2];
        int32_t eventIndex = createDeviceEvent(d);
        setDeviceEventWheelValue(d, eventIndex, 2, -yOffset); // _WHEEL(3) = Y
        setDeviceEventWheelValue(d, eventIndex, 3, xOffset);  // _WHEEL(4) = X
        commitDeviceEvent(d);
        eventIndex = createDeviceEvent(d);
        setDeviceEventWheelValue(d, eventIndex, 2, 0.0f);
        setDeviceEventWheelValue(d, eventIndex, 3, 0.0f);
        commitDeviceEvent(d);
    }
}

void GLUT_MOUSE_BUTTON_FUNC(double x, double y, GLUTEmu_MouseButton button, GLUTEmu_ButtonAction action, GLUTEnum_MouseCursorMode mode, int modifiers) {
    (void)mode;
    (void)modifiers;

    Mouse_Button mouseButton;
    switch (button) {
    case GLUTEmu_MouseButton::Left:
        mouseButton = Mouse_Button::Left;
        break;
    case GLUTEmu_MouseButton::Right:
        mouseButton = Mouse_Button::Right;
        break;
    case GLUTEmu_MouseButton::Middle:
        mouseButton = Mouse_Button::Middle;
        break;
    case GLUTEmu_MouseButton::Four:
        mouseButton = Mouse_Button::Four;
        break;
    case GLUTEmu_MouseButton::Five:
        mouseButton = Mouse_Button::Five;
        break;
    case GLUTEmu_MouseButton::Six:
        mouseButton = Mouse_Button::Six;
        break;
    case GLUTEmu_MouseButton::Seven:
        mouseButton = Mouse_Button::Seven;
        break;
    case GLUTEmu_MouseButton::Eight:
        mouseButton = Mouse_Button::Eight;
        break;
    default:
        mouseButton = Mouse_Button::None;
    }

    if (mouseButton == Mouse_Button::None)
        return;

    switch (action) {
    case GLUTEmu_ButtonAction::Pressed:
    case GLUTEmu_ButtonAction::Repeated:
        Mouse_QueueButtonDownEvent(static_cast<int>(mouseButton), x, y);
        break;

    case GLUTEmu_ButtonAction::Released:
        Mouse_QueueButtonUpEvent(static_cast<int>(mouseButton), x, y);
        break;
    }
}

void GLUT_MOUSE_SCROLL_FUNC(double x, double y, double xOffset, double yOffset, GLUTEnum_MouseCursorMode mode) {
    (void)mode;
    Mouse_QueueScrollEvent(x, y, xOffset, yOffset);
}

void GLUT_MOUSE_POSITION_FUNC(double x, double y, GLUTEnum_MouseCursorMode mode) {
    Mouse_QueuePositionEvent(x, y, mode);
}
