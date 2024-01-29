#ifndef INCLUDE_LIBQB_GLUT_MESSAGE_H
#define INCLUDE_LIBQB_GLUT_MESSAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "completion.h"

class glut_message {
  private:
    completion *finished = NULL;

    void initCompletion() {
        finished = new completion();
        completion_init(finished);
    }

  protected:
    glut_message(bool withCompletion) {
        if (withCompletion)
            initCompletion();
    }

  public:
    // Calling this indicates to the creator of the message that it has been
    // completed, and any response data is available to be read.
    //
    // If `finished` is NULL that means nobody is waiting for the response. In
    // that situation we're free to simply delete the object.
    void finish() {
        if (finished)
            completion_finish(finished);
        else
            delete this;
    }

    void wait_for_response() {
        completion_wait(finished);
    }

    virtual ~glut_message() {
        if (finished) {
            completion_wait(finished); // Should be a NOP, but better to check anyway
            completion_clear(finished);

            delete finished;
        }
    }

    virtual void execute() = 0;
};

class glut_message_set_cursor : public glut_message {
  public:
    int style;
    void execute();

    glut_message_set_cursor(int _style) : glut_message(false), style(_style) { }
};

class glut_message_warp_pointer : public glut_message {
  public:
    int x, y;
    void execute();

    glut_message_warp_pointer(int _x, int _y) : glut_message(false), x(_x), y(_y) { }
};

class glut_message_get : public glut_message {
  public:
    int id;
    int response_value;
    void execute();

    glut_message_get(int _id) : glut_message(true), id(_id), response_value(0) { }
};

class glut_message_iconify_window : public glut_message {
  public:
    void execute();

    glut_message_iconify_window() : glut_message(false) { }
};

class glut_message_position_window : public glut_message {
  public:
    int x, y;
    void execute();

    glut_message_position_window(int _x, int _y) : glut_message(false), x(_x), y(_y) { }
};

class glut_message_show_window : public glut_message {
  public:
    void execute();

    glut_message_show_window() : glut_message(false) { }
};

class glut_message_hide_window : public glut_message {
  public:
    void execute();

    glut_message_hide_window() : glut_message(false) { }
};

class glut_message_set_window_title : public glut_message {
  public:
    char *newTitle;
    void execute();

    glut_message_set_window_title(const char *title) : glut_message(false) {
        newTitle = strdup(title);
    }

    virtual ~glut_message_set_window_title() {
        free(newTitle);
    }
};

class glut_message_exit_program : public glut_message {
  public:
    int exitCode;
    void execute();

    glut_message_exit_program(int _exitCode) : glut_message(true), exitCode(_exitCode) { }
};

// Queues a glut_message to be processed. Returns false if the message was not
// queued.
bool libqb_queue_glut_message(glut_message *msg);

#endif
