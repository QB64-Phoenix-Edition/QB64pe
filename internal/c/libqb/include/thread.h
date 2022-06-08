#ifndef INCLUDE_LIBQB_THREAD_H
#define INCLUDE_LIBQB_THREAD_H

// Thread
struct libqb_thread;

// Allocates a new thread, note that it is not running when this returns
// Thread should already be stopped/joined when free is called
struct libqb_thread *libqb_thread_new();
void libqb_thread_free(struct libqb_thread *);

// Configures and starts a thread to begin it's execution.
void libqb_thread_start(struct libqb_thread *, void (*start_func) (void *), void *arg);

// Joins a thread to end its execution
void libqb_thread_join(struct libqb_thread *);

#endif
