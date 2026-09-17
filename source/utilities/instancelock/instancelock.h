// A helper to obtain an exclusive lock on a file, ensuring only one process can acquire it.
//
// QB64-PE's 'LOCK WRITE' does this, but only on Windows, so this header
// implements similar functionality on Linux and Mac OS (Note that it's not a
// prue replacement, it's unclear if the libqb runtime should do this).
//
// On Linux and Mac OS we use flock() which is not as strict as on Windows (the
// lock is not enforced automatically, only if other programs call flock()).
// Since the only other programs we're interacting with are other QB64-PE
// instances this still works fine.

#ifndef QB64PE_INSTANCELOCK_H
#define QB64PE_INSTANCELOCK_H

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/file.h>
#  include <unistd.h>
#endif

// Returns non-zero if this process now exclusively holds path, 0 if another
// process holds it.
//
// The descriptor is deliberately never closed: the claim is meant to last as
// long as the process, the OS will close the file and remove the lock when our
// process ends.
static inline int qb64pe_claim_lock(const char *path) {
#if defined(_WIN32)
    // A share mode of 0 is the claim - no other process (nor this one) can open
    // the file until the handle is closed. OPEN_ALWAYS creates the file when it
    // is missing and opens it as it stands when it is not.
    HANDLE handle = CreateFileA(path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    return handle != INVALID_HANDLE_VALUE;
#else
    int fd = open(path, O_CREAT | O_RDWR, 0644);

    if (fd < 0)
        return 0;

    // flock is advisory, which is enough here: the only other process that ever
    // asks for this lock is another QB64-PE. It is held by the open file
    // description, so it survives fork and is released by the last close - or
    // by the process ending.
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        close(fd);
        return 0;
    }

    return 1;
#endif
}

#endif
