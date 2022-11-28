
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

void libqb_process_glut_queue() {
}

void libqb_glut_set_cursor(int style) {

}

void libqb_glut_warp_pointer(int x, int y)
{
}

int libqb_glut_get(int id) {
    return 0;
}

void libqb_glut_iconify_window() {
}

void libqb_glut_position_window(int x, int y) {
}

void libqb_glut_show_window() {
}

void libqb_glut_hide_window() {
}

void libqb_glut_set_window_title(const char *title) {
}

void libqb_glut_exit_program(int exitcode) {
    libqb_exit(exitcode);
}

// Since there's no GLUT thread to deal with we can just exit() like normal
void libqb_exit(int code) {
    exit(code);
}
