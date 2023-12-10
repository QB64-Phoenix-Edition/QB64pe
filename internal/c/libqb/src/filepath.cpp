
// Implementation of these functions was pulled from miniaudio.h (MIT)

#include "libqb-common.h"

#include <algorithm>

#include "../../libqb.h"
#include "filepath.h"

const char *filepath_get_filename(const char *path) {
    const char *fileName;

    if (path == NULL) {
        return NULL;
    }

    fileName = path;

    /* We just loop through the path until we find the last slash. */
    while (path[0] != '\0') {
        if (path[0] == '/' || path[0] == '\\') {
            fileName = path;
        }

        path += 1;
    }

    /* At this point the file name is sitting on a slash, so just move forward. */
    while (fileName[0] != '\0' && (fileName[0] == '/' || fileName[0] == '\\')) {
        fileName += 1;
    }

    return fileName;
}

const char *filepath_get_extension(const char *path) {
    const char *extension;
    const char *lastOccurance;

    if (path == NULL) {
        path = "";
    }

    extension = filepath_get_filename(path);
    lastOccurance = NULL;

    /* Just find the last '.' and return. */
    while (extension[0] != '\0') {
        if (extension[0] == '.') {
            extension += 1;
            lastOccurance = extension;
        }

        extension += 1;
    }

    return (lastOccurance != NULL) ? lastOccurance : extension;
}

bool filepath_has_extension(const char *path, const char *extension) {
    const char *ext1;
    const char *ext2;

    if (path == NULL || extension == NULL) {
        return false;
    }

    ext1 = extension;
    ext2 = filepath_get_extension(path);

#if defined(_MSC_VER) || defined(__DMC__)
    return _stricmp(ext1, ext2) == 0;
#else
    return strcasecmp(ext1, ext2) == 0;
#endif
}

/// @brief Changes the slashes in a file name / path to make it compatible with the OS
/// @param path The path to fix (this contents may be changed)
/// @return Returns the C-string for convenience
const char *filepath_fix_directory(char *path) {
    auto len = strlen(path);

    for (size_t i = 0; i < len; i++) {
#ifdef QB64_WINDOWS
        if (path[i] == '/')
            path[i] = '\\';
#else
        if (path[i] == '\\')
            path[i] = '/';
#endif
    }

    return path;
}

/// @brief Changes the slashes in a file name / path to make it compatible with the OS
/// @param path The path to fix (this contents may be changed)
/// @return Returns the C-string for convenience
const char *filepath_fix_directory(qbs *path) {
    for (size_t i = 0; i < path->len; i++) {
#ifdef QB64_WINDOWS
        if (path->chr[i] == '/')
            path->chr[i] = '\\';
#else
        if (path->chr[i] == '\\')
            path->chr[i] = '/';
#endif
    }

    return (char *)path->chr;
}

/// @brief Changes the slashes in a file name / path to make it compatible with the OS
/// @param path The path to fix (this contents may be changed)
/// @return Returns the C-string for convenience
const char *filepath_fix_directory(std::string &path) {
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) {
#ifdef QB64_WINDOWS
        return c == '/' ? '\\' : c;
#else
        return c == '\\' ? '/' : c;
#endif
    });

    return path.c_str();
}
