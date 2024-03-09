// Mouse support functions for macOS
// These are required to overcome the limitations of GLUT

#include "libqb-common.h"

#include "mac-mouse-support.h"

#include <ApplicationServices/ApplicationServices.h>
#include <GLUT/glut.h>
#include <unistd.h>

#define QB64_EVENT_CLOSE 1
#define QB64_EVENT_KEY 2
#define QB64_EVENT_RELATIVE_MOUSE_MOVEMENT 3
#define QB64_EVENT_FILE_DROP 4

extern "C" int qb64_custom_event(int event, int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8, void *p1, void *p2);
void GLUT_MOUSEWHEEL_FUNC(int wheel, int direction, int x, int y);

extern int g_MouseX, g_MouseY;

static CFMachPortRef g_EventTap = nullptr;
static CFRunLoopSourceRef g_RunLoopSource = nullptr;

static CGEventRef macMouseCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *userInfo) {
    auto appPID = reinterpret_cast<int64_t>(userInfo);

    if (CGEventGetIntegerValueField(event, kCGEventTargetUnixProcessID) == appPID) {
        if (type == kCGEventScrollWheel) {
            auto deltaScroll = CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1);
            GLUT_MOUSEWHEEL_FUNC(0, deltaScroll, g_MouseX, g_MouseY);
        } else {
            auto deltaX = CGEventGetIntegerValueField(event, kCGMouseEventDeltaX);
            auto deltaY = CGEventGetIntegerValueField(event, kCGMouseEventDeltaY);
            if (deltaX || deltaY)
                qb64_custom_event(QB64_EVENT_RELATIVE_MOUSE_MOVEMENT, deltaX, deltaY, 0, 0, 0, 0, 0, 0, nullptr, nullptr);
        }
    }

    return event;
}

void macMouseInit() {
    if (!g_EventTap) {
        CGEventMask eventMask = CGEventMaskBit(kCGEventLeftMouseDown) | CGEventMaskBit(kCGEventLeftMouseUp) | CGEventMaskBit(kCGEventRightMouseDown) |
                                CGEventMaskBit(kCGEventRightMouseUp) | CGEventMaskBit(kCGEventMouseMoved) | CGEventMaskBit(kCGEventLeftMouseDragged) |
                                CGEventMaskBit(kCGEventRightMouseDragged) | CGEventMaskBit(kCGEventScrollWheel) | CGEventMaskBit(kCGEventOtherMouseDown) |
                                CGEventMaskBit(kCGEventOtherMouseUp) | CGEventMaskBit(kCGEventOtherMouseDragged);

        g_EventTap = CGEventTapCreate(kCGAnnotatedSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, eventMask, macMouseCallback,
                                      reinterpret_cast<void *>(getpid()));
        if (g_EventTap) {
            g_RunLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, g_EventTap, 0);

            if (g_RunLoopSource) {
                CFRunLoopAddSource(CFRunLoopGetCurrent(), g_RunLoopSource, kCFRunLoopCommonModes);
                CGEventTapEnable(g_EventTap, true);
            } else {
                CFRelease(g_EventTap);
            }
        }
    }
}

void macMouseDone() {
    if (g_EventTap) {
        CGEventTapEnable(g_EventTap, false);
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), g_RunLoopSource, kCFRunLoopCommonModes);
        CFRelease(g_RunLoopSource);
        CFRelease(g_EventTap);
        g_RunLoopSource = nullptr;
        g_EventTap = nullptr;
    }
}
