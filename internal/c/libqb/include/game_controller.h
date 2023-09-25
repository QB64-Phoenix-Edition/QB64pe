//----------------------------------------------------------------------------------------------------------------------
// QB64-PE Game Controller Library
// Powered by libstem Gamepad (https://github.com/ThemsAllTook/libstem_gamepad)
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

struct device_struct;
struct onstrig_struct;

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
