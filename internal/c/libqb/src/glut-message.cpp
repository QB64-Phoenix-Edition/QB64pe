
#include "libqb-common.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// note: MacOSX uses Apple's GLUT not FreeGLUT
#ifdef QB64_MACOSX
#    include <GLUT/glut.h>
#else
#    define CORE_FREEGLUT
#    include <GL/freeglut.h>
#endif

#include "glut-message.h"
#include "mac-mouse-support.h"

void glut_message_set_cursor::execute() {
    if (style == GLUT_CURSOR_NONE) {
        glutSetCursor(style);
        macMouseAssociateMouseAndMouseCursorPosition(false);
    } else {
        macMouseAssociateMouseAndMouseCursorPosition(true);
        glutSetCursor(style);
    }
}

void glut_message_warp_pointer::execute() {
    glutWarpPointer(x, y);
}

void glut_message_get::execute() {
    response_value = glutGet(id);
}

void glut_message_iconify_window::execute() {
    glutIconifyWindow();
}

void glut_message_position_window::execute() {
    glutPositionWindow(x, y);
}

void glut_message_show_window::execute() {
    glutShowWindow();
}

void glut_message_hide_window::execute() {
    glutHideWindow();
}

void glut_message_set_window_title::execute() {
    glutSetWindowTitle(newTitle);
}

void glut_message_exit_program::execute() {
    macMouseDone();
    exit(exitCode);
}
