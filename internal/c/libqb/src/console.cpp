#include "libqb-common.h"

#include "cmem.h"
#include "console.h"
#include "font.h"
#include "graphics.h"
#include "qbs.h"
#include "ring-buffer.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// GLFW_TODO: Get rid of OS specific code in this file and move it to term-emu.cpp

#ifdef QB64_WINDOWS
#    include <windows.h>
#endif

static constexpr size_t ConsoleInputQueueSize = 4096;

// This ia a temporary solutions for now until we can get the TermEmu library working properly on all platforms.
static RingBuffer<int32_t, ConsoleInputQueueSize, true> g_consoleKeyQueue; // A ring buffer that holds key scan codes.
static int32_t g_consoleMouseX = 0;                                        // The current X position of the mouse in the console window.
static int32_t g_consoleMouseY = 0;                                        // The current Y position of the mouse in the console window.
static uint32_t g_consoleMouseButtons = 0;                                 // The current state of the mouse buttons in the console window.

int32_t console_active = 1;
extern int32_t console_image;
extern int32_t console_child;

int32_t func__console() {
    if (is_error_pending())
        return -1;
    return console_image;
}

void sub__console(int32_t onoff) {
    if (!console)
        return;

    if (onoff == 1) {
        if (!console_active) {
#ifdef QB64_WINDOWS
            if (console_child) {
                ShowWindow(GetConsoleWindow(), SW_SHOWNOACTIVATE);
            }
#endif
            console_active = 1;
        }
    } else {
        if (console_active) {
#ifdef QB64_WINDOWS
            if (console_child) {
                ShowWindow(GetConsoleWindow(), SW_HIDE);
            }
#endif
            console_active = 0;
        }
    }
}

void sub__consoletitle(qbs *s) {
#ifdef QB64_WINDOWS
    std::string title(reinterpret_cast<const char *>(s->chr), s->len);
    if (console) {
        if (console_active) {
            SetConsoleTitleA(title.c_str());
            Sleep(40);
        }
    }
#endif
}

void sub__consolefont(qbs *FontName, int32_t FontSize) {
#ifdef QB64_WINDOWS
#    if WINVER >= 0x0600 // this block is not compatible with XP
    SECURITY_ATTRIBUTES SecAttribs = {sizeof(SECURITY_ATTRIBUTES), 0, 1};
    HANDLE cl_conout = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &SecAttribs, OPEN_EXISTING, 0, 0);
    static int OneTimePause;
    if (!OneTimePause) { // a slight delay so the console can be properly created and registered with Windows, before we try to change fonts with it.
        Sleep(500);
        OneTimePause = 1; // after the first pause, the console should be created, so we don't need any more delays in the future.
    }
    CONSOLE_FONT_INFOEX info = {0};
    info.cbSize = sizeof(info);
    info.dwFontSize.Y = FontSize; // leave X as zero
    info.FontWeight = FW_NORMAL;
    if (FontName->len > 0) { // if we don't pass a font name, don't change the existing one.
        // info is zero-initialized, so FaceName is already null-terminated.
        MultiByteToWideChar(CP_ACP, 0, (const char *)FontName->chr, FontName->len, info.FaceName, LF_FACESIZE - 1);
    }

    SetCurrentConsoleFontEx(cl_conout, NULL, &info);
#    endif
#endif
}

void sub__console_cursor(int32_t visible, int32_t cursorsize, int32_t passed) {
#ifdef QB64_WINDOWS
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;

    GetConsoleCursorInfo(consoleHandle, &info); // get the original info, so we reuse it, unless the user called for a change.

    if (visible == 1)
        info.bVisible = TRUE; // cursor is set to show
    if (visible == 2)
        info.bVisible = FALSE; // set to hide
    if (passed && cursorsize >= 0 && cursorsize <= 100)
        info.dwSize = cursorsize; // the user passed the cursor size, of a suitable size

    SetConsoleCursorInfo(consoleHandle, &info);
#endif
}

int32_t func__getconsoleinput() {
#ifdef QB64_WINDOWS
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD irInputRecord;
    DWORD dwEventsRead, fdwMode, dwMode;
    CONSOLE_SCREEN_BUFFER_INFO cl_bufinfo{};
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout != INVALID_HANDLE_VALUE)
        GetConsoleScreenBufferInfo(hStdout, &cl_bufinfo);

    GetConsoleMode(hStdin, (LPDWORD)&dwMode);
    fdwMode = ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(hStdin, fdwMode);
    fdwMode = dwMode | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, fdwMode);

    DWORD numEvents = 0;
    GetNumberOfConsoleInputEvents(hStdin, &numEvents);
    if (numEvents) {
        ReadConsoleInputA(hStdin, &irInputRecord, 1, &dwEventsRead);
        switch (irInputRecord.EventType) {
        case KEY_EVENT: // keyboard input
            g_consoleKeyQueue.Push(irInputRecord.Event.KeyEvent.wVirtualScanCode * (irInputRecord.Event.KeyEvent.bKeyDown ? 1 : -1));
            return 1;
        case MOUSE_EVENT: // mouse input
            g_consoleMouseX = irInputRecord.Event.MouseEvent.dwMousePosition.X + 1;
            g_consoleMouseY = irInputRecord.Event.MouseEvent.dwMousePosition.Y - cl_bufinfo.srWindow.Top + 1;
            g_consoleMouseButtons = irInputRecord.Event.MouseEvent.dwButtonState;
            // SetConsoleMode(hStdin, dwMode);
            return 2;
        }
    }
#endif
    return 0; // no or unhandled input
}

int32_t func__cinp(int32_t toggle, int32_t passed) {
    int32_t temp;
    if (g_consoleKeyQueue.Pop(temp)) {
        if (passed == 0)
            toggle = 1; // default: return positive/negative scan code
        if (toggle) {
            return temp;
        } else {
            if (temp >= 0)
                return temp;
            return -temp + 128;
        }
    }

    return 0;
}

int32_t func__console_mouse_x() {
    return g_consoleMouseX;
}

int32_t func__console_mouse_y() {
    return g_consoleMouseY;
}

int32_t func__console_mouse_buttons() {
    return g_consoleMouseButtons;
}

void sub__echo(qbs *message) {
    if (is_error_pending())
        return;

    int32_t prevDest = func__dest();
    sub__dest(func__console());

    makefit(message);
    qbs_print(message, 0);
    qbs_print(nothingstring, 1);

    sub__dest(prevDest);
}
