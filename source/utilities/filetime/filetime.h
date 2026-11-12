#ifndef QB64PE_FILETIME_H
#define QB64PE_FILETIME_H

#include <stdint.h>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <sys/stat.h>
#  include <sys/types.h>
#endif

// Returns the modification time of path in nanoseconds since the unix epoch,
// or 0 when it cannot be read.
//
// The stamp is taken at the finest resolution the platform offers.
static inline int64_t qb64pe_file_mtime(const char *path) {
#if defined(_WIN32)
    // The CRT's struct stat has no sub-second field on Windows, so the stamp is
    // read from a handle. Opening the file means a symlink reports its target,
    // as stat() does elsewhere; GetFileAttributesEx would report the link itself.
    HANDLE handle = CreateFileA(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle == INVALID_HANDLE_VALUE)
        return 0;

    BY_HANDLE_FILE_INFORMATION info;
    BOOL ok = GetFileInformationByHandle(handle, &info);
    CloseHandle(handle);

    if (!ok)
        return 0;

    // Converts a FILETIME to unix epoch: this is the number of 100ns ticks
    // between the FILETIME epoch (1601) and the Unix epoch (1970).
    const uint64_t epoch = 116444736000000000ULL;
    uint64_t ticks = ((uint64_t)info.ftLastWriteTime.dwHighDateTime << 32) | info.ftLastWriteTime.dwLowDateTime;

    // A filesystem without write times reports zero. Anything we can't express
    // as positive nanoseconds is "unknown".
    if (ticks <= epoch || ticks - epoch > INT64_MAX / 100)
        return 0;

    return (int64_t)(ticks - epoch) * 100;
#else
    struct stat st;

    if (stat(path, &st) != 0)
        return 0;

#  if defined(__APPLE__)
    return (int64_t)st.st_mtimespec.tv_sec * 1000000000LL + (int64_t)st.st_mtimespec.tv_nsec;
#  else
    return (int64_t)st.st_mtim.tv_sec * 1000000000LL + (int64_t)st.st_mtim.tv_nsec;
#  endif
#endif
}

#endif
