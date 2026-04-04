// This file is for Console-Only programs. They never invoke GLUT so the setup here is much simpler.

#include "libqb-common.h"

#include "glut-thread.h"

#include <cstdlib>

// FIXME: Put this definition somewhere else
extern void MAIN_LOOP(void *);

void libqb_glut_presetup() {}

void libqb_start_main_thread() {
    // Because GLUT is not used, we can just run MAIN_LOOP without creating a
    // new thread for it.
    MAIN_LOOP(NULL);
}

void libqb_start_glut_thread() {}

bool libqb_is_glut_up() {
    return false;
}

// Since there's no GLUT thread to deal with we can just exit() like normal
void libqb_exit(int code) {
    exit(code);
}
