//-----------------------------------------------------------------------------------------------------
//  QB64-PE filesystem related functions
//-----------------------------------------------------------------------------------------------------

#include "libqb-common.h"

#include "filepath.h"
#include "filesystem.h"

#include "../../libqb.h"

#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef QB64_WINDOWS
#    include <shlobj.h>
#else
#    include <sys/statvfs.h>
#endif

#ifdef QB64_BACKSLASH_FILESYSTEM
#    define FS_PATH_SEPARATOR '\\'
#else
#    define FS_PATH_SEPARATOR '/'
#endif

#if (FILENAME_MAX > 4096)
#    define FS_PATHNAME_LENGTH_MAX FILENAME_MAX
#else
#    define FS_PATHNAME_LENGTH_MAX 4096
#endif

/// @brief This is a global variable that is set on startup and holds the directory that was current when the program was loaded
qbs *g_startDir = nullptr;

/// @brief Gets the current working directory
/// @return A qbs containing the current working directory or an empty string on error
qbs *func__cwd() {
    std::string path;
    qbs *qbsFinal;

    path.resize(FILENAME_MAX, '\0');

    for (;;) {
        if (getcwd(&path[0], path.size())) {
            auto size = strlen(path.c_str());
            qbsFinal = qbs_new(size, 1);
            memcpy(qbsFinal->chr, &path[0], size);

            return qbsFinal;
        } else {
            if (errno == ERANGE)
                path.resize(path.size() << 1, '\0'); // buffer size was not sufficient; try again with a larger buffer
            else
                break; // some other error occurred
        }
    }

    qbsFinal = qbs_new(0, 1);
    error(QB_ERROR_INTERNAL_ERROR);

    return qbsFinal;
}

/// @brief Returns true if the specified directory exists
/// @param path The directory to check for
/// @return True if the directory exists
static inline bool FS_DirectoryExists(const char *path) {
#ifdef QB64_WINDOWS
    auto x = GetFileAttributesA(path);

    return x != INVALID_FILE_ATTRIBUTES && (x & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat info;

    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
#endif
}

/// @brief Known directories (primarily Windows based, but we'll do our best to emulate on other platforms)
enum class FS_KnownDirectory {
    HOME = 0,
    DESKTOP,
    DOCUMENTS,
    PICTURES,
    MUSIC,
    VIDEOS,
    DOWNLOAD,
    APP_DATA,
    LOCAL_APP_DATA,
    PROGRAM_DATA,
    SYSTEM_FONTS,
    USER_FONTS,
    TEMP,
    PROGRAM_FILES,
    PROGRAM_FILES_32,
};

#ifdef QB64_WINDOWS
/// @brief Returns the full path for a known directory
/// @param kD Is a value from FS_KnownDirectory (above)
/// @return The full path that ends with the system path separator
static std::string FS_GetKnownDirectory(FS_KnownDirectory kD) {
    std::string path(FS_PATHNAME_LENGTH_MAX, '\0'); // allocate something that is sufficiently large

    switch (kD) {
    case FS_KnownDirectory::DESKTOP: // %USERPROFILE%\OneDrive\Desktop
        SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
        break;

    case FS_KnownDirectory::DOCUMENTS: // %USERPROFILE%\OneDrive\Documents
        SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
        break;

    case FS_KnownDirectory::PICTURES: // %USERPROFILE%\OneDrive\Pictures
        SHGetFolderPathA(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
        break;

    case FS_KnownDirectory::MUSIC: // %USERPROFILE%\Music
        SHGetFolderPathA(NULL, CSIDL_MYMUSIC | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
        break;

    case FS_KnownDirectory::VIDEOS: // %USERPROFILE%\Videos
        SHGetFolderPathA(NULL, CSIDL_MYVIDEO | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
        break;

    case FS_KnownDirectory::DOWNLOAD: // %USERPROFILE%\Downloads
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]))) {
            // XP & SHGetFolderPathA do not support the concept of a Downloads folder, however it can be constructed
            path.resize(strlen(path.c_str()));
            path.append("\\Downloads");
            mkdir(path.c_str());
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::APP_DATA: // %USERPROFILE%\AppData\Roaming
        SHGetFolderPathA(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
        break;

    case FS_KnownDirectory::LOCAL_APP_DATA: // %USERPROFILE%\AppData\Local
        SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
        break;

    case FS_KnownDirectory::PROGRAM_DATA: // %SystemDrive%\ProgramData
        SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
        break;

    case FS_KnownDirectory::SYSTEM_FONTS: // %SystemRoot%\Fonts
        SHGetFolderPathA(NULL, CSIDL_FONTS | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
        break;

    case FS_KnownDirectory::USER_FONTS: // %USERPROFILE%\AppData\Local\Microsoft\Windows\Fonts
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]))) {
            path.resize(strlen(path.c_str()));
            path.append("\\Microsoft\\Windows\\Fonts");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::TEMP: // %USERPROFILE%\AppData\Local\Temp
        GetTempPathA(path.size(), &path[0]);
        break;

    case FS_KnownDirectory::PROGRAM_FILES: // %SystemDrive%\Program Files
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
        break;

    case FS_KnownDirectory::PROGRAM_FILES_32: // %SystemDrive%\Program Files (x86)
#    ifdef _WIN64
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86 | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
#    else
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
#    endif
        break;

    case FS_KnownDirectory::HOME: // %USERPROFILE%
    default:
        SHGetFolderPathA(NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0]);
    }

    // Check if we got anything at all
    if (!strlen(path.c_str())) {
        path.resize(FS_PATHNAME_LENGTH_MAX, '\0'); // just in case this was shrunk above

        if (FAILED(SHGetFolderPathA(NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &path[0])))
            path.assign(".\\"); // fallback to the current directory
    }

    // Add the trailing slash
    path.resize(strlen(path.c_str()));
    if (path.back() != FS_PATH_SEPARATOR)
        path.append(1, FS_PATH_SEPARATOR);

    return path;
}
#else
/// @brief Returns the full path for a known directory
/// @param kD Is a value from FS_KnownDirectory (above)
/// @return The full path that ends with the system path separator
static std::string FS_GetKnownDirectory(FS_KnownDirectory kD) {
    std::string path;
    auto envVar = getenv("HOME");

    switch (kD) {
    case FS_KnownDirectory::DESKTOP: // %USERPROFILE%\OneDrive\Desktop
        if (envVar) {
            path.assign(envVar);
            path.append("/Desktop");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::DOCUMENTS: // %USERPROFILE%\OneDrive\Documents
        if (envVar) {
            path.assign(envVar);
            path.append("/Documents");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::PICTURES: // %USERPROFILE%\OneDrive\Pictures
        if (envVar) {
            path.assign(envVar);
            path.append("/Pictures");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::MUSIC: // %USERPROFILE%\Music
        if (envVar) {
            path.assign(envVar);
            path.append("/Music");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::VIDEOS: // %USERPROFILE%\Videos
        if (envVar) {
            path.assign(envVar);
#    ifdef QB64_MACOSX
            path.append("/Movies");
#    else
            path.append("/Videos");
#    endif
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::DOWNLOAD: // %USERPROFILE%\Downloads
        if (envVar) {
            path.assign(envVar);
            path.append("/Downloads");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::APP_DATA: // %USERPROFILE%\AppData\Roaming
        if (envVar) {
            path.assign(envVar);
            path.append("/.config");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::LOCAL_APP_DATA: // %USERPROFILE%\AppData\Local
    case FS_KnownDirectory::PROGRAM_DATA:   // %SystemDrive%\ProgramData
        if (envVar) {
            path.assign(envVar);
            path.append("/.local/share");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::SYSTEM_FONTS: // %SystemRoot%\Fonts
#    ifdef QB64_MACOSX
        path.assign("/System/Library/Fonts");
        if (!FS_DirectoryExists(path.c_str())) {
            path.assign("/Library/Fonts");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
#    else
        path.assign("/usr/share/fonts");
        if (!FS_DirectoryExists(path.c_str())) {
            path.assign("/usr/local/share/fonts");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
#    endif
        break;

    case FS_KnownDirectory::USER_FONTS: // %USERPROFILE%\AppData\Local\Microsoft\Windows\Fonts
        if (envVar) {
            path.assign(envVar);
#    ifdef QB64_MACOSX
            path.append("/Library/Fonts");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
#    else
            path.append("/.local/share/fonts");
            if (!FS_DirectoryExists(path.c_str())) {
                path.assign(envVar);
                path.append("/.fonts");
                if (!FS_DirectoryExists(path.c_str()))
                    path.clear();
            }
#    endif
        }
        break;

    case FS_KnownDirectory::TEMP: // %USERPROFILE%\AppData\Local\Temp
        path.assign("/var/tmp");
        if (!FS_DirectoryExists(path.c_str())) {
            path.assign("/tmp");
            if (!FS_DirectoryExists(path.c_str()))
                path.clear();
        }
        break;

    case FS_KnownDirectory::PROGRAM_FILES:    // %SystemDrive%\Program Files
    case FS_KnownDirectory::PROGRAM_FILES_32: // %SystemDrive%\Program Files (x86)
#    ifdef QB64_MACOSX
        path.assign("/Applications");
#    else
        path.assign("/opt");
#    endif
        if (!FS_DirectoryExists(path.c_str()))
            path.clear();
        break;

    case FS_KnownDirectory::HOME: // %USERPROFILE%
    default:
        if (envVar)
            path.assign(envVar);
    }

    // Check if we got anything at all
    if (!strlen(path.c_str()))
        path.assign(envVar ? envVar : "./");

    // Add the trailing slash
    path.resize(strlen(path.c_str()));
    if (path.back() != FS_PATH_SEPARATOR)
        path.append(1, FS_PATH_SEPARATOR);

    return path;
}
#endif

/// @brief Returns common paths such as My Documents, My Pictures, My Music, Desktop
/// @param qbsContext Is the directory type
/// @return A qbs containing the directory or an empty string on error
qbs *func__dir(qbs *qbsContext) {
    std::string path, context(reinterpret_cast<char *>(qbsContext->chr), qbsContext->len);

    std::transform(context.begin(), context.end(), context.begin(), [](unsigned char c) { return std::toupper(c); });

    // The following is largely unchanged from what we previously had
    if (context.compare("TEXT") == 0 || context.compare("DOCUMENT") == 0 || context.compare("DOCUMENTS") == 0 || context.compare("MY DOCUMENTS") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::DOCUMENTS);
    } else if (context.compare("MUSIC") == 0 || context.compare("AUDIO") == 0 || context.compare("SOUND") == 0 || context.compare("SOUNDS") == 0 ||
               context.compare("MY MUSIC") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::MUSIC);
    } else if (context.compare("PICTURE") == 0 || context.compare("PICTURES") == 0 || context.compare("IMAGE") == 0 || context.compare("IMAGES") == 0 ||
               context.compare("MY PICTURES") == 0 || context.compare("DCIM") == 0 || context.compare("CAMERA") == 0 || context.compare("CAMERA ROLL") == 0 ||
               context.compare("PHOTO") == 0 || context.compare("PHOTOS") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::PICTURES);
    } else if (context.compare("MOVIE") == 0 || context.compare("MOVIES") == 0 || context.compare("VIDEO") == 0 || context.compare("VIDEOS") == 0 ||
               context.compare("MY VIDEOS") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::VIDEOS);
    } else if (context.compare("DOWNLOAD") == 0 || context.compare("DOWNLOADS") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::DOWNLOAD);
    } else if (context.compare("DESKTOP") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::DESKTOP);
    } else if (context.compare("APPDATA") == 0 || context.compare("APPLICATION DATA") == 0 || context.compare("PROGRAM DATA") == 0 ||
               context.compare("DATA") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::APP_DATA);
    } else if (context.compare("LOCALAPPDATA") == 0 || context.compare("LOCAL APPLICATION DATA") == 0 || context.compare("LOCAL PROGRAM DATA") == 0 ||
               context.compare("LOCAL DATA") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::LOCAL_APP_DATA);
    } else if (context.compare("PROGRAMFILES") == 0 || context.compare("PROGRAM FILES") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::PROGRAM_FILES);
    } else if (context.compare("PROGRAMFILESX86") == 0 || context.compare("PROGRAMFILES X86") == 0 || context.compare("PROGRAM FILES X86") == 0 ||
               context.compare("PROGRAM FILES 86") == 0 || context.compare("PROGRAM FILES (X86)") == 0 || context.compare("PROGRAMFILES (X86)") == 0 ||
               context.compare("PROGRAM FILES(X86)") == 0 || context.compare("PROGRAMFILES(X86)") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::PROGRAM_FILES_32);
    } else if (context.compare("TMP") == 0 || context.compare("TEMP") == 0 || context.compare("TEMP FILES") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::TEMP);
    } else if (context.compare("HOME") == 0 || context.compare("USER") == 0 || context.compare("PROFILE") == 0 || context.compare("USERPROFILE") == 0 ||
               context.compare("USER PROFILE") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::HOME);
    } else if (context.compare("FONT") == 0 || context.compare("FONTS") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::SYSTEM_FONTS);
    } else if (context.compare("USERFONT") == 0 || context.compare("USER FONT") == 0 || context.compare("USERFONTS") == 0 ||
               context.compare("USER FONTS") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::USER_FONTS);
    } else if (context.compare("PROGRAMDATA") == 0 || context.compare("COMMON PROGRAM DATA") == 0) {
        path = FS_GetKnownDirectory(FS_KnownDirectory::PROGRAM_DATA);
    } else {
        path = FS_GetKnownDirectory(FS_KnownDirectory::DESKTOP); // anything else defaults to the desktop where the user can easily see stuff
    }

    auto size = path.size();
    auto qbsFinal = qbs_new(size, 1);
    memcpy(qbsFinal->chr, &path[0], size);

    return qbsFinal;
}

/// @brief Returns true if a directory specified exists
/// @param path The directory path
/// @return True if the directory exists
int32_t func__direxists(qbs *path) {
    if (new_error)
        return QB_FALSE;

    std::string pathName(reinterpret_cast<char *>(path->chr), path->len);

    return FS_DirectoryExists(filepath_fix_directory(pathName)) ? QB_TRUE : QB_FALSE;
}

/// @brief Returns true if a file specified exists
/// @param path The file path to check for
/// @return True if the file exists
static inline bool FS_FileExists(const char *path) {
#ifdef QB64_WINDOWS
    auto x = GetFileAttributesA(path);

    return x != INVALID_FILE_ATTRIBUTES && !(x & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat info;

    return stat(path, &info) == 0 && S_ISREG(info.st_mode);
#endif
}

/// @brief Returns true if a file specified exists
/// @param path The file path to check for
/// @return True if the file exists
int32_t func__fileexists(qbs *path) {
    if (new_error)
        return QB_FALSE;

    std::string pathName(reinterpret_cast<char *>(path->chr), path->len);

    return FS_FileExists(filepath_fix_directory(pathName)) ? QB_TRUE : QB_FALSE;
}

/// @brief Return the startup directory
/// @return A qbs containing the directory path
qbs *func__startdir() {
    auto temp = qbs_new(0, 1);

    qbs_set(temp, g_startDir);

    return temp;
}

/// @brief Changes the current directory
/// @param str The directory path to change to
void sub_chdir(qbs *str) {
    if (new_error)
        return;

    std::string pathName(reinterpret_cast<char *>(str->chr), str->len);

    if (chdir(filepath_fix_directory(pathName)) == -1)
        error(QB_ERROR_PATH_NOT_FOUND); // assume errno == ENOENT; path not found
}

/// @brief Checks if s is an empty string (either NULL or zero length)
/// @param s A null-terminated string or NULL
/// @return False is we have a valid string > length 0
static inline bool FS_IsStringEmpty(const char *s) { return s == nullptr || s[0] == '\0'; }

/// @brief This is a basic pattern matching function used by FS_GetDirectoryEntryName()
/// @param fileSpec The pattern to match
/// @param fileName The filename to match
/// @return True if it is a match, false otherwise
static inline bool FS_IsPatternMatching(const char *fileSpec, const char *fileName) {
    auto spec = fileSpec;
    auto name = fileName;
    const char *any = nullptr;

    while (*spec || *name) {
        switch (*spec) {
        case '*': // handle wildcard '*' character
            any = spec;
            spec++;
#ifdef QB64_WINDOWS
            while (*name && std::toupper(*spec) != std::toupper(*name))
#else
            while (*name && *spec != *name)
#endif
                name++;
            break;

        case '?': // handle wildcard '?' character
            spec++;
            if (*name)
                name++;
            break;

        default: // compare non-wildcard characters
#ifdef QB64_WINDOWS
            if (std::toupper(*spec) != std::toupper(*name))
#else
            if (*spec != *name)
#endif
            {
                if (any && *name)
                    spec = any;
                else
                    return false;
            } else {
                spec++;
                name++;
            }
            break;
        }
    }

    return true;
}

/// @brief Returns true if fileSpec has any wildcards
/// @param fileSpec The string to check
/// @return True if * or ? are found
static inline bool FS_HasPattern(const char *fileSpec) { return fileSpec != nullptr && (strchr(fileSpec, '*') || strchr(fileSpec, '?')); }

/// @brief An MS BASIC PDS DIR$ style function
/// @param fileSpec This can be a path with wildcard for the final level (i.e. C:/Windows/*.* or /usr/lib/* etc.)
/// @return Returns a file or directory name matching fileSpec or an empty string when there is nothing left
static const char *FS_GetDirectoryEntryName(const char *fileSpec) {
    static DIR *pDir = nullptr;
    static char pattern[FS_PATHNAME_LENGTH_MAX];
    static char entry[FS_PATHNAME_LENGTH_MAX];

    entry[0] = '\0'; // set to an empty string

    if (!FS_IsStringEmpty(fileSpec)) {
        // We got a filespec. Check if we have one already going and if so, close it
        if (pDir) {
            closedir(pDir);
            pDir = nullptr;
        }

        char dirName[FS_PATHNAME_LENGTH_MAX]; // we only need this for opendir()

        if (FS_HasPattern(fileSpec)) {
            // We have a pattern. Check if we have a path in it
            auto p = strrchr(fileSpec, '/'); // try *nix style separator
#ifdef QB64_WINDOWS
            if (!p)
                p = strrchr(fileSpec, '\\'); // try windows style separator
#endif

            if (p) {
                // Split the path and the filespec
                strncpy(pattern, p + 1, FS_PATHNAME_LENGTH_MAX);
                pattern[FS_PATHNAME_LENGTH_MAX - 1] = '\0';
                auto len = std::min<size_t>((p - fileSpec) + 1, FS_PATHNAME_LENGTH_MAX - 1);
                memcpy(dirName, fileSpec, len);
                dirName[len] = '\0';
            } else {
                // No path. Use the current path
                strncpy(pattern, fileSpec, FS_PATHNAME_LENGTH_MAX);
                pattern[FS_PATHNAME_LENGTH_MAX - 1] = '\0';
                strcpy(dirName, "./");
            }
        } else {
            // No pattern. Check if this is a file and simply return the name if it exists
            if (FS_FileExists(fileSpec)) {
                strncpy(entry, filepath_get_filename(fileSpec), FS_PATHNAME_LENGTH_MAX);
                entry[FS_PATHNAME_LENGTH_MAX - 1] = '\0';

                return entry;
            }

            // Else, We'll just assume it's a directory
            strncpy(dirName, fileSpec, FS_PATHNAME_LENGTH_MAX);
            dirName[FS_PATHNAME_LENGTH_MAX - 1] = '\0';
            strcpy(pattern, "*");
        }

        pDir = opendir(dirName);
    }

    if (pDir) {
        for (;;) {
            auto pDirent = readdir(pDir);
            if (!pDirent) {
                closedir(pDir);
                pDir = nullptr;

                break;
            }

            if (FS_IsPatternMatching(pattern, pDirent->d_name)) {
                strncpy(entry, pDirent->d_name, FS_PATHNAME_LENGTH_MAX);
                entry[FS_PATHNAME_LENGTH_MAX - 1] = '\0';

                break;
            }
        }
    }

    return entry;
}

/// @brief This mimics MS BASIC PDS 7.1 & VBDOS 1.0 DIR$() function
/// @param qbsFileSpec This can be a path with wildcard for the final level (i.e. C:/Windows/*.* or /usr/lib/* etc.)
/// @param passed Flags for optional parameters
/// @return Returns a qbs with the directory entry name or an empty string if there are no more entries
qbs *func__files(qbs *qbsFileSpec, int32_t passed) {
    static std::string directory;
    std::string pathName;
    const char *entry;
    qbs *qbsFinal;

    // Check if fresh arguments were passed and we need to begin a new session
    if (passed) {
        std::string fileSpec(reinterpret_cast<char *>(qbsFileSpec->chr), qbsFileSpec->len);

        if (fileSpec.empty())
            fileSpec = "*.*";

        if (FS_DirectoryExists(filepath_fix_directory(fileSpec))) {
            directory = fileSpec;
        } else {
            filepath_split(fileSpec, directory, pathName); // split the file path
            if (directory.empty())
                directory = "./";
        }

        entry = FS_GetDirectoryEntryName(fileSpec.c_str());
    } else {
        // Check if we've been called the first time without a filespec
        if (directory.empty()) {
            // This is per MS BASIC PDS 7.1 and VBDOS 1.0 behavior
            qbsFinal = qbs_new(0, 1);
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return qbsFinal;
        }

        entry = FS_GetDirectoryEntryName(nullptr); // get the next entry
    }

    filepath_join(pathName, directory, entry);
    auto size = strlen(entry);

    if (size && FS_DirectoryExists(pathName.c_str())) {
        // Add a trailing slash if it is a directory
        qbsFinal = qbs_new(size + 1, 1);
        memcpy(qbsFinal->chr, entry, size);
        qbsFinal->chr[size] = FS_PATH_SEPARATOR;
    } else {
        qbsFinal = qbs_new(size, 1);
        memcpy(qbsFinal->chr, entry, size);
    }

    if (!size)
        directory.clear(); // clear the directory string since we are done with the session

    return qbsFinal;
}

/// @brief Returns the free volume space for a given directory
/// @param path A directory that resides on the volume we want the free space for
/// @return The free space in bytes
static uint64_t FS_GetFreeDiskSpace(const char *path) {
#ifdef QB64_WINDOWS
    ULARGE_INTEGER freeBytesAvailable;
    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;

    if (GetDiskFreeSpaceExA(path, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
        return totalNumberOfFreeBytes.QuadPart;
#else
    struct statvfs stat;

    if (statvfs(path, &stat) == 0)
        return static_cast<uint64_t>(stat.f_bsize) * stat.f_bfree;
#endif

    return 0; // zero if something failed
}

/// @brief Gets the fully qualified name (FQN)
/// @param path The path name to get the FQN for
/// @return The FQN
static std::string FS_GetFQN(const char *path) {
    std::string FQN = path; // fallback

#ifdef QB64_WINDOWS
    DWORD size = GetFullPathNameA(path, 0, nullptr, nullptr); // get the required buffer size

    if (size) {
        FQN.resize(size);
        if (GetFullPathNameA(path, size, &FQN[0], nullptr))
            FQN.resize(size - 1); // resize to exclude the null terminator
    }
#else
    char *result = realpath(path, nullptr);
    if (result) {
        FQN = result;
        free(result); // cleanup memory allocated by realpath
    }
#endif

    if (FS_DirectoryExists(FQN.c_str()) && FQN.back() != FS_PATH_SEPARATOR)
        FQN.append(1, FS_PATH_SEPARATOR);

    return FQN;
}

/// @brief Gets the fully qualified name (FQN)
/// @param qbsPathName The path name to get the FQN for
/// @return The FQN
qbs *func__fullpath(qbs *qbsPathName) {
    qbs *temp;

    if (!qbsPathName->len) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        temp = qbs_new(0, 1);
        return temp;
    }

    std::string pathName(reinterpret_cast<char *>(qbsPathName->chr), qbsPathName->len);
    filepath_fix_directory(pathName);

    if (!FS_DirectoryExists(pathName.c_str()) && !FS_FileExists(pathName.c_str())) {
        // Path not found
        error(QB_ERROR_PATH_NOT_FOUND);
        temp = qbs_new(0, 1);
        return temp;
    }

    pathName = FS_GetFQN(pathName.c_str());
    temp = qbs_new(pathName.size(), 1);
    memcpy(temp->chr, &pathName[0], pathName.size());

    return temp;
}

/// @brief Gets the short name for a file / directory (if possible)
/// @param path The file / directory to get the short name for
/// @return The short name
static std::string FS_GetShortName(const char *path) {
#ifdef QB64_WINDOWS
    DWORD size = GetShortPathNameA(path, nullptr, 0); // get the required buffer size

    if (size) {
        std::string shortPath;
        shortPath.resize(size);
        if (GetShortPathNameA(path, &shortPath[0], size)) {
            shortPath.resize(size - 1); // resize to exclude the null terminator
            return shortPath;
        }
    }
#endif

    return path; // return the path as-is for *nix or if GetShortPathNameA failed
}

/// @brief Prints a list of files in the current directory using a file specification
/// @param str Is a string containing a path (it can include wildcards)
/// @param passed Optional parameters
void sub_files(qbs *str, int32_t passed) {
    static qbs *strz = nullptr;

    if (new_error)
        return;

    if (!strz)
        strz = qbs_new(0, 0);

    std::string fileSpec, directory, pathName;

    if (passed && str->len) {
        fileSpec.assign(reinterpret_cast<char *>(str->chr), str->len);

        if (FS_DirectoryExists(filepath_fix_directory(fileSpec))) {
            directory = FS_GetFQN(fileSpec.c_str());
        } else {
            std::string d;
            filepath_split(fileSpec, d, pathName);
            if (d.empty())
                d = "./"; // default to the current directory if one is not specified

            directory = FS_GetFQN(d.c_str());
        }
    } else {
        fileSpec = "./";
        directory = FS_GetFQN(fileSpec.c_str());
    }

    if (directory.empty()) {
        // Invalid filespec
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }

    std::string shortName = FS_GetShortName(directory.c_str());

    // Print the path
    qbs_set(strz, qbs_new_txt_len(shortName.c_str(), shortName.size()));
    qbs_print(strz, 1);

    auto entry = FS_GetDirectoryEntryName(fileSpec.c_str()); // get the first entry
    filepath_join(pathName, directory, entry);

    if (FS_IsStringEmpty(entry)) {
        // File not found
        error(QB_ERROR_FILE_NOT_FOUND);
        return;
    }

    // Print directory entries
    do {
        fileSpec = FS_GetShortName(pathName.c_str()); // we do not need fileSpec anymore
        filepath_split(fileSpec, directory, shortName);
        qbs_set(strz, qbs_new_txt_len(shortName.c_str(), shortName.size()));

        if (strz->len < 12) {
            // Padding required
            qbs_set(strz, qbs_add(strz, func_space(12 - strz->len)));
            for (auto i = 0; i < 12; i++) {
                if (strz->chr[i] == 46) {
                    memmove(&strz->chr[8], &strz->chr[i], 4);
                    memset(&strz->chr[i], 32, 8 - i);
                    break;
                }
            }
        }

        if (FS_DirectoryExists(pathName.c_str()))
            qbs_set(strz, qbs_add(strz, qbs_new_txt_len("<DIR> ", 6)));
        else
            qbs_set(strz, qbs_add(strz, func_space(6)));

        makefit(strz);
        qbs_print(strz, 0);

        entry = FS_GetDirectoryEntryName(nullptr); // get the next entry
        filepath_join(pathName, directory, entry);
    } while (!FS_IsStringEmpty(entry));

    if (func_pos(0) > 1) {
        // Move to a new line if necessary
        strz->len = 0;
        qbs_print(strz, 1);
    }

    // Print the free volume space
    qbs_set(strz, qbs_add(qbs_str(FS_GetFreeDiskSpace(directory.c_str())), qbs_new_txt_len(" Bytes free", 11)));
    qbs_print(strz, 1);
}

/// @brief Deletes files from disk
/// @param str The file(s) to delete (may contain wildcard at the final level)
void sub_kill(qbs *str) {
    if (new_error)
        return;

    std::string directory, pathName, fileSpec(reinterpret_cast<char *>(str->chr), str->len);

    filepath_split(filepath_fix_directory(fileSpec), directory, pathName); // split the file path
    auto entry = FS_GetDirectoryEntryName(fileSpec.c_str());               // get the first entry

    // Keep looking through the entries until we file a file
    while (!FS_IsStringEmpty(entry)) {
        filepath_join(pathName, directory, entry);

        if (FS_FileExists(pathName.c_str()))
            break;

        entry = FS_GetDirectoryEntryName(nullptr); // get the next entry
    }

    // Check if we have exhausted the entries without ever finding a file
    if (FS_IsStringEmpty(entry)) {
        // This behavior is per QBasic 1.1
        error(QB_ERROR_FILE_NOT_FOUND);
        return;
    }

    // Process all matches
    do {
        // We'll delete only if it is a file
        if (FS_FileExists(pathName.c_str())) {
            if (remove(pathName.c_str())) {
                auto i = errno;

                if (i == ENOENT) {
                    error(QB_ERROR_FILE_NOT_FOUND);
                    return;
                } // file not found

                if (i == EACCES) {
                    error(QB_ERROR_PATH_FILE_ACCESS_ERROR);
                    return;
                } // path / file access error

                error(QB_ERROR_BAD_FILE_NAME); // bad file name (assumed)
            }
        }

        entry = FS_GetDirectoryEntryName(nullptr); // get the next entry
        filepath_join(pathName, directory, entry);
    } while (!FS_IsStringEmpty(entry));
}

/// @brief Creates a new directory
/// @param str The directory path name to create
void sub_mkdir(qbs *str) {
    if (new_error)
        return;

    std::string pathName(reinterpret_cast<char *>(str->chr), str->len);

#ifdef QB64_WINDOWS
    if (mkdir(filepath_fix_directory(pathName)) == -1)
#else
    if (mkdir(filepath_fix_directory(pathName), S_IRWXU | S_IRWXG) == -1)
#endif
    {
        if (errno == EEXIST) {
            error(QB_ERROR_PATH_FILE_ACCESS_ERROR);
            return;
        } // path / file access error

        error(QB_ERROR_PATH_NOT_FOUND); // assume errno == ENOENT; path not found
    }
}

/// @brief Renames a file or directory
/// @param oldname The old file / directory name
/// @param newname The new file / directory name
void sub_name(qbs *oldname, qbs *newname) {
    if (new_error)
        return;

    std::string pathNameOld(reinterpret_cast<char *>(oldname->chr), oldname->len), pathNameNew(reinterpret_cast<char *>(newname->chr), newname->len);

    if (rename(filepath_fix_directory(pathNameOld), filepath_fix_directory(pathNameNew))) {
        auto i = errno;

        if (i == ENOENT) {
            error(QB_ERROR_FILE_NOT_FOUND);
            return;
        } // file not found

        if (i == EINVAL) {
            error(QB_ERROR_BAD_FILE_NAME);
            return;
        } // bad file name

        if (i == EACCES) {
            error(QB_ERROR_PATH_FILE_ACCESS_ERROR);
            return;
        } // path / file access error

        error(QB_ERROR_ILLEGAL_FUNCTION_CALL); // illegal function call (assumed)
    }
}

/// @brief Deletes an empty directory
/// @param str The path name of the directory to delete
void sub_rmdir(qbs *str) {
    if (new_error)
        return;

    std::string pathName(reinterpret_cast<char *>(str->chr), str->len);

    if (rmdir(filepath_fix_directory(pathName)) == -1) {
        if (errno == ENOTEMPTY) {
            error(QB_ERROR_PATH_FILE_ACCESS_ERROR);
            return;
        } // path/file access error; not an empty directory

        error(QB_ERROR_PATH_NOT_FOUND); // assume errno == ENOENT; path not found
    }
}
