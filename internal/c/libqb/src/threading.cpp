
#include "libqb-common.h"

#include "mutex.h"
#include "condvar.h"
#include "completion.h"

void completion_init(struct completion *comp) {
    comp->finished = 0;
    comp->mutex = libqb_mutex_new();
    comp->var = libqb_condvar_new();
}

void completion_clear(struct completion *comp) {
    libqb_mutex_free(comp->mutex);
    libqb_condvar_free(comp->var);
}

void completion_wait(struct completion *comp) {
    libqb_mutex_guard guard(comp->mutex);

    while (!comp->finished)
        libqb_condvar_wait(comp->var, comp->mutex);
}

void completion_finish(struct completion *comp) {
    libqb_mutex_guard guard(comp->mutex);

    comp->finished = 1;
    libqb_condvar_broadcast(comp->var);
}
