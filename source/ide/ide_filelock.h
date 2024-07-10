#include <stdint.h> // standard integer types

#ifdef QB64_WINDOWS

/// We wouldn't need to call these functions in Windows, but to avoid
/// the need for platform-conditional checks on the BASIC side we simply
/// define the functions as stubs in Windows.
///-------------------------------------------------------------------
int32_t LockGlobalFileAccess(const char *filename) {
    return 0; // always succeeds
}

void ReleaseGlobalFileAccess(void) {
    return;
}

#else // = QB64_LINUX or QB64_MACOSX

/// The real functions used in Linux and MacOSX will either open and
/// lock, or unlock and close the file respectively.
///-------------------------------------------------------------------
#include <fcntl.h>    // requred for open()
#include <sys/file.h> // requred for flock()
#include <unistd.h>   // requred for close()

// Saved file descriptor, as this is global we can only lock one file
// at a time, but that's sufficient for our use case.
//--------------------------------------------------------------------
int32_t filedesc = -1;

// Try to lock the given file to prevent other IDE instances from
// accessing the same file at the same time.
//  In: filename (STRING, add CHR$(0) to end of string)
// Out: error    (INTEGER, 0 = no error (file locked), -1 = error (file not locked))
//--------------------------------------------------------------------
int32_t LockGlobalFileAccess(const char *filename) {
    int32_t err = -1;     // so far assume error
    if (filedesc == -1) { // make sure only one file at a time
        filedesc = open(filename, O_RDWR);
        if (filedesc > -1) {
            err = flock(filedesc, LOCK_EX | LOCK_NB); // try to lock
            if (err == -1) {
                close(filedesc); // on error close and
                filedesc = -1;   // reset file descriptor
            }
        }
    }
    return err;
}

// Release an existing file lock, effectively allowing other IDE
// instances to access the file.
//  In: n/a
// Out: n/a
//--------------------------------------------------------------------
void ReleaseGlobalFileAccess(void) {
    if (filedesc > -1) { // only if a file is opened & locked
        flock(filedesc, LOCK_UN);
        close(filedesc);
        filedesc = -1;   // reset file descriptor
    }
}

#endif // WIN_LNX_MAC

