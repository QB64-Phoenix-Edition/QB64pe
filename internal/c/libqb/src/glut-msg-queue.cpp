
#include "libqb-common.h"

#include <unistd.h>
#include <queue>

#include "mutex.h"
#include "glut-message.h"
#include "glut-thread.h"

static libqb_mutex *glut_msg_queue_lock = libqb_mutex_new();
static std::queue<glut_message *> glut_msg_queue;

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

int libqb_glut_get(int id) {
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
