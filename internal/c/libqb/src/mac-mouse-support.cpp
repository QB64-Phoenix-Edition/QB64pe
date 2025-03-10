// Mouse support functions for macOS
// These hacks are required to overcome the limitations of macOS GLUT

#include "libqb-common.h"

#include "logging.h"
#include "mac-mouse-support.h"

#include <ApplicationServices/ApplicationServices.h>
#include <unistd.h>

void qb64_custom_event_relative_mouse_movement(int deltaX, int deltaY);
void GLUT_MOUSEWHEEL_FUNC(int wheel, int direction, int x, int y);

class MacMouse {
  public:
    void UpdatePosition(int x, int y) {
        this->x = x;
        this->y = y;
    }

    static MacMouse &Instance() {
        static MacMouse instance;
        return instance;
    }

  private:
    MacMouse() : x(0), y(0) {
        appPID = getpid();

        eventSource = CGEventSourceCreate(CGEventSourceStateID::kCGEventSourceStateCombinedSessionState);
        if (eventSource) {
            CGEventSourceSetLocalEventsSuppressionInterval(eventSource, 0.0);
            libqb_log_trace("Disabled local events suppression interval");
        } else {
            libqb_log_warn("Failed to create event source");
        }

        CGEventMask eventMask = CGEventMaskBit(kCGEventLeftMouseDown) | CGEventMaskBit(kCGEventLeftMouseUp) | CGEventMaskBit(kCGEventRightMouseDown) |
                                CGEventMaskBit(kCGEventRightMouseUp) | CGEventMaskBit(kCGEventMouseMoved) | CGEventMaskBit(kCGEventLeftMouseDragged) |
                                CGEventMaskBit(kCGEventRightMouseDragged) | CGEventMaskBit(kCGEventScrollWheel) | CGEventMaskBit(kCGEventOtherMouseDown) |
                                CGEventMaskBit(kCGEventOtherMouseUp) | CGEventMaskBit(kCGEventOtherMouseDragged);

        eventTap = CGEventTapCreate(kCGAnnotatedSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, eventMask, Callback,
                                    reinterpret_cast<void *>(this));
        if (eventTap) {
            libqb_log_trace("Created event tap");

            runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);

            if (runLoopSource) {
                CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
                CGEventTapEnable(eventTap, true);

                libqb_log_trace("Created run loop source");
            } else {
                libqb_log_warn("Failed to create run loop source");

                CFRelease(eventTap);
                eventTap = nullptr;
            }
        } else {
            libqb_log_warn("Failed to create event tap");
        }

        libqb_log_info("Initialized Mac mouse hacks");
    }

    ~MacMouse() {
        if (eventTap) {
            CGEventTapEnable(eventTap, false);
            CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
            CFRelease(runLoopSource);
            CFRelease(eventTap);

            libqb_log_trace("Released event tap and run loop source");
        }

        if (eventSource) {
            CFRelease(eventSource);

            libqb_log_trace("Released event source");
        }

        libqb_log_info("Shutdown Mac mouse hacks");
    }

    MacMouse(const MacMouse &) = delete;
    MacMouse &operator=(const MacMouse &) = delete;
    MacMouse(MacMouse &&) = delete;
    MacMouse &operator=(MacMouse &&) = delete;

    static CGEventRef Callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *userInfo) {
        (void)(proxy);

        auto macMouse = reinterpret_cast<MacMouse *>(userInfo);

        if (CGEventGetIntegerValueField(event, kCGEventTargetUnixProcessID) == macMouse->appPID) {
            if (type == kCGEventScrollWheel) {
                GLUT_MOUSEWHEEL_FUNC(0, CGEventGetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1), macMouse->x, macMouse->y);
            } else {
                auto deltaX = CGEventGetIntegerValueField(event, kCGMouseEventDeltaX);
                auto deltaY = CGEventGetIntegerValueField(event, kCGMouseEventDeltaY);
                if (deltaX || deltaY) {
                    qb64_custom_event_relative_mouse_movement(deltaX, deltaY);
                }
            }
        }

        return event;
    }

    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;
    CGEventSourceRef eventSource;
    pid_t appPID;
    int x;
    int y;
};

void MacMouse_UpdatePosition(int x, int y) {
    MacMouse::Instance().UpdatePosition(x, y);
}
