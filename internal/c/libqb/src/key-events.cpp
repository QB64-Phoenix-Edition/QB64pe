#include "key-events.h"
#include "error_handle.h"
#include "event.h"
#include "keyboard.h"
#include <cstdlib>

onkey_struct *onkey = (onkey_struct *)calloc(32, sizeof(onkey_struct));
int32_t onkey_inprogress = 0;

void onkey_setup(int32_t i, uint32_t id, int64_t pass) {
    // note: pass is ignored by ids not requiring a pass value
    if (is_error_pending())
        return;
    if ((i < 1) || (i > 31)) {
        error(5);
        return;
    }
    onkey[i].state = 0;
    onkey[i].pass = pass;
    onkey[i].id = id; // id must be set last because it is the trigger variable
}

void sub_key(int32_t i, int32_t option) {
    // ref: "(?){ON|OFF|STOP}"
    if (is_error_pending())
        return;
    if ((i < 0) || (i > 31)) {
        error(5);
        return;
    }
    int32_t i1 = i;
    int32_t i2 = i;
    if (!i) {
        i1 = i;
        i2 = 31;
    } // set all keys!
    for (i = i1; i <= i2; i++) {
        // ref: uint8 active;//0=OFF, 1=ON, 2=STOP
        if (option == 1) { // ON
            onkey[i].active = 1;
            if (onkey[i].state)
                qbevent = 1;
        }
        if (option == 2) { // OFF
            onkey[i].active = 0;
            onkey[i].state = 0;
        }
        if (option == 3) { // STOP
            onkey[i].active = 2;
            if (onkey[i].state)
                onkey[i].state = 1;
        }
    } // i
}

void onkey_reset_runtime_state() {
    for (int32_t i = 1; i <= 31; i++) {
        onkey[i].id = 0;
        onkey[i].active = 0;
        onkey[i].state = 0;
    }
    onkey_inprogress = 0;
}

void onkey_init_default_bindings() {
    onkey[1].keycode = 59 << 8; // F1-F10
    onkey[2].keycode = 60 << 8;
    onkey[3].keycode = 61 << 8;
    onkey[4].keycode = 62 << 8;
    onkey[5].keycode = 63 << 8;
    onkey[6].keycode = 64 << 8;
    onkey[7].keycode = 65 << 8;
    onkey[8].keycode = 66 << 8;
    onkey[9].keycode = 67 << 8;
    onkey[10].keycode = 68 << 8;
    onkey[11].keycode = 72 << 8; // up,left,right,down
    onkey[11].keycode_alternate = VK + QBVK_KP8;
    onkey[12].keycode = 75 << 8;
    onkey[12].keycode_alternate = VK + QBVK_KP4;
    onkey[13].keycode = 77 << 8;
    onkey[13].keycode_alternate = VK + QBVK_KP6;
    onkey[14].keycode = 80 << 8;
    onkey[14].keycode_alternate = VK + QBVK_KP2;
    onkey[30].keycode = 133 << 8; // F11,F12
    onkey[31].keycode = 134 << 8;
}
