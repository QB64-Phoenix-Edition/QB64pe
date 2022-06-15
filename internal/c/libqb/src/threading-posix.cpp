
#include "libqb-common.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "mutex.h"

struct libqb_thread {
    pthread_t thread;
};

struct libqb_mutex {
    pthread_mutex_t mtx;
};

struct libqb_condvar {
    pthread_cond_t var;
};

struct libqb_mutex *libqb_mutex_new() {
    struct libqb_mutex *m = (struct libqb_mutex *)malloc(sizeof(*m));
    pthread_mutex_init(&m->mtx, NULL);
    return m;
}

void libqb_mutex_free(struct libqb_mutex *mutex) {
    pthread_mutex_destroy(&mutex->mtx);
    free(mutex);
}

void libqb_mutex_lock(struct libqb_mutex *m) {
    pthread_mutex_lock(&m->mtx);
}

void libqb_mutex_unlock(struct libqb_mutex *m) {
    pthread_mutex_unlock(&m->mtx);
}

struct libqb_condvar *libqb_condvar_new() {
    struct libqb_condvar *c = (struct libqb_condvar *)malloc(sizeof(*c));
    pthread_cond_init(&c->var, NULL);
    return c;
}

void libqb_condvar_free(struct libqb_condvar *c) {
    pthread_cond_destroy(&c->var);
    free(c);
}

void libqb_condvar_wait(struct libqb_condvar *condvar, struct libqb_mutex *mutex) {
    pthread_cond_wait(&condvar->var, &mutex->mtx);
}

void libqb_condvar_signal(struct libqb_condvar *condvar) {
    pthread_cond_signal(&condvar->var);
}

void libqb_condvar_broadcast(struct libqb_condvar *condvar) {
    pthread_cond_broadcast(&condvar->var);
}

struct libqb_thread *libqb_thread_new() {
    struct libqb_thread *t = (struct libqb_thread *)malloc(sizeof(*t));
    memset(t, 0, sizeof(*t));

    return t;
}

void libqb_thread_free(struct libqb_thread *t) {
    // The thread should have already have been joined.
    free(t);
}

struct thread_wrapper_args {
    void (*wrapper) (void *);
    void *arg;
};

static void *thread_wrapper(void *varg) {
    struct thread_wrapper_args *arg = (struct thread_wrapper_args *)varg;
    (arg->wrapper) (arg->arg);
    free(arg);

    return NULL;
}

void libqb_thread_start(struct libqb_thread *t, void (*start_func) (void *), void *start_func_arg) {
    struct thread_wrapper_args *arg = (struct thread_wrapper_args *)malloc(sizeof(*arg));
    arg->wrapper = start_func;
    arg->arg = start_func_arg;

    pthread_create(&t->thread, NULL, thread_wrapper, (void *)arg);
}

void libqb_thread_join(struct libqb_thread *t) {
    pthread_join(t->thread, NULL);
}
