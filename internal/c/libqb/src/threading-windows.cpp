
#include "libqb-common.h"

#include <windows.h>
#include <synchapi.h>
#include <process.h>

#include "thread.h"
#include "mutex.h"
#include "condvar.h"

struct libqb_thread {
    HANDLE thread_handle;
};

struct libqb_mutex {
    CRITICAL_SECTION crit_section;
};

struct libqb_condvar {
    CONDITION_VARIABLE var;
};

struct libqb_mutex *libqb_mutex_new() {
    struct libqb_mutex *m = (struct libqb_mutex *)malloc(sizeof(*m));

    InitializeCriticalSectionAndSpinCount(&m->crit_section, 200);
    return m;
}

void libqb_mutex_free(struct libqb_mutex *mutex) {
    DeleteCriticalSection(&mutex->crit_section);
    free(mutex);
}

void libqb_mutex_lock(struct libqb_mutex *m) {
    EnterCriticalSection(&m->crit_section);
}

void libqb_mutex_unlock(struct libqb_mutex *m) {
    LeaveCriticalSection(&m->crit_section);
}

struct libqb_condvar *libqb_condvar_new() {
    struct libqb_condvar *condvar = (struct libqb_condvar *)malloc(sizeof(*condvar));

    InitializeConditionVariable(&condvar->var);
    return condvar;
}

void libqb_condvar_free(struct libqb_condvar *condvar) {
    free(condvar);
}

void libqb_condvar_wait(struct libqb_condvar *condvar, struct libqb_mutex *mutex) {
    SleepConditionVariableCS(&condvar->var, &mutex->crit_section, INFINITE);
}

void libqb_condvar_signal(struct libqb_condvar *condvar) {
    WakeConditionVariable(&condvar->var);
}

void libqb_condvar_broadcast(struct libqb_condvar *condvar) {
    WakeAllConditionVariable(&condvar->var);
}

struct libqb_thread *libqb_thread_new() {
    struct libqb_thread *t = (struct libqb_thread *)malloc(sizeof(*t));
    t->thread_handle = 0;

    return t;
}

void libqb_thread_free(struct libqb_thread *t) {
    // The handle is closed automatically when using _beginthreadex
    free(t);
}

struct thread_wrapper_args {
    void (*wrapper) (void *);
    void *arg;
};

// This wrapper is so that the caller doesn't need to provide a __stdcall function, which is not portable
static unsigned int __stdcall stdcall_thread_wrapper(void *varg) {
    struct thread_wrapper_args *arg = (struct thread_wrapper_args *)varg;
    (arg->wrapper) (arg->arg);
    free(arg);

    return 0;
}

void libqb_thread_start(struct libqb_thread *t, void (*start_func) (void *), void *start_func_arg) {
    struct thread_wrapper_args *arg = (struct thread_wrapper_args *)malloc(sizeof(*arg));
    arg->wrapper = start_func;
    arg->arg = start_func_arg;

    t->thread_handle = (HANDLE)_beginthreadex(NULL, 0, stdcall_thread_wrapper, arg, 0, NULL);
}

void libqb_thread_join(struct libqb_thread *t) {
    WaitForSingleObject(t->thread_handle, INFINITE);
}
