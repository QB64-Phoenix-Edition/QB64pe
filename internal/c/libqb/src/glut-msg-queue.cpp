
#include "libqb-common.h"

#include <unistd.h>
#include <queue>

// note: MacOSX uses Apple's GLUT not FreeGLUT
#ifdef QB64_MACOSX
#    include <GLUT/glut.h>
#else
#    define CORE_FREEGLUT
#    include <GL/freeglut.h>
#endif

#include "mutex.h"
#include "glut-message.h"
#include "glut-thread.h"

static libqb_mutex *glut_msg_queue_lock = libqb_mutex_new();
static std::queue<glut_message *> glut_msg_queue;

// These values from GLUT are read on every process of the msg queue. Calls to
// libqb_glut_get() can then read from these values directly rather than wait
// for the GLUT thread to process the command.
static int glut_window_x, glut_window_y;

#ifdef CORE_FREEGLUT
static int glut_window_border_width, glut_window_header_height;
#endif

bool libqb_queue_glut_message(glut_message *msg) {
    if (!libqb_is_glut_up()) {
        msg->finish();
        return false;
    }

    libqb_mutex_guard guard(glut_msg_queue_lock);

    glut_msg_queue.push(msg);

    return true;
}

void libqb_process_glut_queue() {
    libqb_mutex_guard guard(glut_msg_queue_lock);

    glut_window_x = glutGet(GLUT_WINDOW_X);
    glut_window_y = glutGet(GLUT_WINDOW_Y);

#ifdef CORE_FREEGLUT
    glut_window_border_width = glutGet(GLUT_WINDOW_BORDER_WIDTH);
    glut_window_header_height = glutGet(GLUT_WINDOW_HEADER_HEIGHT);
#endif

    while (!glut_msg_queue.empty()) {
        glut_message *msg = glut_msg_queue.front();
        glut_msg_queue.pop();

        msg->execute();

        msg->finish();
    }
}

void libqb_glut_set_cursor(int style) {
    libqb_queue_glut_message(new glut_message_set_cursor(style));
}

void libqb_glut_warp_pointer(int x, int y) {
    libqb_queue_glut_message(new glut_message_warp_pointer(x, y));
}

static bool is_static_glut_value(int id) {
    return id == GLUT_WINDOW_Y
        || id == GLUT_WINDOW_X
#ifdef CORE_FREEGLUT
        || id == GLUT_WINDOW_BORDER_WIDTH
        || id == GLUT_WINDOW_HEADER_HEIGHT
#endif
    ;
}

static int __get_static_glut_value(int id) {
    switch (id) {
    case GLUT_WINDOW_Y:             return glut_window_y;
    case GLUT_WINDOW_X:             return glut_window_x;
#ifdef CORE_FREEGLUT
    case GLUT_WINDOW_BORDER_WIDTH:  return glut_window_border_width;
    case GLUT_WINDOW_HEADER_HEIGHT: return glut_window_header_height;
#endif
    default:                        return -1;
    }
}

int libqb_glut_get(int id) {
    if (is_static_glut_value(id)) {
        libqb_mutex_guard guard(glut_msg_queue_lock);
        return __get_static_glut_value(id);
    }

    glut_message_get msg(id);

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    return msg.response_value;
}

void libqb_glut_iconify_window() {
    libqb_queue_glut_message(new glut_message_iconify_window());
}

void libqb_glut_position_window(int x, int y) {
    libqb_queue_glut_message(new glut_message_position_window(x, y));
}

void libqb_glut_show_window() {
    libqb_queue_glut_message(new glut_message_show_window());
}

void libqb_glut_hide_window() {
    libqb_queue_glut_message(new glut_message_hide_window());
}

void libqb_glut_set_window_title(const char *title) {
    libqb_queue_glut_message(new glut_message_set_window_title(title));
}

void libqb_glut_exit_program(int exitcode) {
    glut_message_exit_program msg(exitcode);

    libqb_queue_glut_message(&msg);
    msg.wait_for_response();

    // Should never return
    exit(exitcode);
}
