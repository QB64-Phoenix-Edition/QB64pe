#ifndef INCLUDE_LIBQB_CONDVAR_H
#define INCLUDE_LIBQB_CONDVAR_H

#include "mutex.h"

// Condition Variable
struct libqb_condvar;

// Allocates and frees a Condition Variable
struct libqb_condvar *libqb_condvar_new();
void libqb_condvar_free(struct libqb_condvar *);

// Waits until the Condition Variable is signalled, atomically dropping the
// Mutex while checking the Condition Variable
void libqb_condvar_wait(struct libqb_condvar *, struct libqb_mutex *);

// Signals a single thread waiting on the Condition Variable
void libqb_condvar_signal(struct libqb_condvar *);

// Signals all threads waiting on the Condition Variable
void libqb_condvar_broadcast(struct libqb_condvar *);

#endif
