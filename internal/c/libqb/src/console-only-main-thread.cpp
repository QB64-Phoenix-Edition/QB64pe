
#include "libqb-common.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "glut-thread.h"

// This file is for Console-Only programs. They never invoke GLUT so the setup
// here is much simpler.

// FIXME: PUt this definition somewhere else
void MAIN_LOOP(void *);


void libqb_glut_presetup(int argc, char **argv) {

}

void libqb_start_main_thread(int argc, char **argv) {
    // Because GLUT is not used, we can just run MAIN_LOOP without creating a
    // new thread for it.
    MAIN_LOOP(NULL);
}

void libqb_start_glut_thread() {
}

bool libqb_is_glut_up() {
    return false;
}
