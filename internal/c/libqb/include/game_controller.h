//----------------------------------------------------------------------------------------------------------------------
// QB64-PE Game Controller Library
// Powered by libstem Gamepad (https://github.com/ThemsAllTook/libstem_gamepad)
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

// device_struct constants
#define QUEUED_EVENTS_LIMIT 1024
#define DEVICETYPE_CONTROLLER 1
#define DEVICETYPE_KEYBOARD 2
#define DEVICETYPE_MOUSE 3

struct device_struct {
    int32_t used;
    int32_t type;
    // 0=Unallocated
    // 1=Joystick/Gamepad
    // 2=Keyboard
    // 3=Mouse
    char *name; // FIXME: this is modified by game_controller.cpp
    int32_t connected;
    int32_t lastbutton;
    int32_t lastaxis;
    int32_t lastwheel;
    //--------------
    int32_t max_events;
    int32_t queued_events;
    uint8_t *events; // the structure and size of the events depends greatly on the device and its capabilities
    int32_t event_size;
    //--------------
    uint8_t STRIG_button_pressed[256]; // checked and cleared by the STRIG function
    //--------------
    void *handle_pointer;    // handle as pointer
    int64_t handle_int;      // handle as integer
    const char *description; // description provided by manufacturer
    int64_t product_id;
    int64_t vendor_id;
    int32_t buttons;
    int32_t axes;
    int32_t balls;
    int32_t hats;
};

struct onstrig_struct {
    uint32_t id;    // the event ID to trigger (0=no event)
    int64_t pass;   // the value to pass to the triggered event (only applicable to ON ... CALL ...(x)
    uint8_t active; // 0=OFF, 1=ON, 2=STOP
    uint8_t state;  // 0=untriggered,1=triggered,2=in progress(TIMER only),2+=multiple events queued(KEY only)
};

extern int32_t device_last;
extern int32_t device_max;
extern device_struct *devices;
extern onstrig_struct *onstrig;
extern int32_t onstrig_inprogress;

uint8_t getDeviceEventButtonValue(device_struct *device, int32_t eventIndex, int32_t objectIndex);
void setDeviceEventButtonValue(device_struct *device, int32_t eventIndex, int32_t objectIndex, uint8_t value);
float getDeviceEventAxisValue(device_struct *device, int32_t eventIndex, int32_t objectIndex);
void setDeviceEventAxisValue(device_struct *device, int32_t eventIndex, int32_t objectIndex, float value);
float getDeviceEventWheelValue(device_struct *device, int32_t eventIndex, int32_t objectIndex);
void setDeviceEventWheelValue(device_struct *device, int32_t eventIndex, int32_t objectIndex, float value);
void setupDevice(device_struct *device);
int32_t createDeviceEvent(device_struct *device);
void commitDeviceEvent(device_struct *device);

void QB64_GAMEPAD_INIT();
void QB64_GAMEPAD_POLL();
void QB64_GAMEPAD_SHUTDOWN();
