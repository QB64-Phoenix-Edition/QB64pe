#ifndef INCLUDE_LIBQB_COMPLETION_H
#define INCLUDE_LIBQB_COMPLETION_H

#include "mutex.h"
#include "condvar.h"

// A completion is a oneshot signal - it waits until finish is called and
// then never blocks again.
//
// Due to the oneshot nature the order wait() and finish() are called does not matter.
struct completion {
    int finished;
    struct libqb_mutex *mutex;
    struct libqb_condvar *var;
};

void completion_init(struct completion *);
void completion_clear(struct completion *);

// Blocks until the completion is finished
void completion_wait(struct completion *);

// Finishes the completion, unblocks all waiters
void completion_finish(struct completion *);

#endif
