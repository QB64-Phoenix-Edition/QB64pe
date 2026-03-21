#pragma once

#include <cstdint>

struct qbs;

struct onkey_struct {
    uint32_t id;                // the event ID to trigger (0=no event)
    int64_t pass;               // the value to pass to the triggered event (only applicable to ON ... CALL ...(x)
    uint8_t active;             // 0=OFF, 1=ON, 2=STOP
    uint8_t state;              // 0=untriggered,1=triggered,2=in progress(TIMER only),2+=multiple events queued(KEY only)
    uint32_t keycode;           // 32-bit code, same as what _KEYHIT returns
    uint32_t keycode_alternate; // an alternate keycode which may also trigger event
    uint8_t key_scancode;
    uint8_t key_flags;
    // flags:
    // 0 No keyboard flag, 1-3 Either Shift key, 4 Ctrl key, 8 Alt key,32 NumLock key,64 Caps Lock key, 128 Extended keys on a 101-key keyboard
    // To specify multiple shift states, add the values together. For example, a value of 12 specifies that the user-defined key is used in combination with the
    // Ctrl and Alt keys.
    qbs *text;
};

extern onkey_struct *onkey;
extern int32_t onkey_inprogress;

void onkey_setup(int32_t i, uint32_t id, int64_t pass);
void sub_key(int32_t i, int32_t option);
