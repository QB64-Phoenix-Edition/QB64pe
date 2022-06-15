#ifndef INCLUDE_LIBQB_MUTEX_H
#define INCLUDE_LIBQB_MUTEX_H

struct libqb_mutex;

// Allocates and frees a Mutex. Mutex is created unlocked.
struct libqb_mutex *libqb_mutex_new();
void libqb_mutex_free(struct libqb_mutex *);

// Lock and unlock the Mutex
void libqb_mutex_lock(struct libqb_mutex *);
void libqb_mutex_unlock(struct libqb_mutex *);

// Locks a mutex when created, and unlocks when the guard goes out of scope
class libqb_mutex_guard {
  public:
    libqb_mutex_guard(struct libqb_mutex *mtx) : lock(mtx) {
        libqb_mutex_lock(lock);
    }

    ~libqb_mutex_guard() {
        libqb_mutex_unlock(lock);
    }

  private:
    struct libqb_mutex *lock;
};

#endif
