//----------------------------------------------------------------------------------------------------
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
#endif

#ifdef QB64_BACKSLASH_FILESYSTEM
#    define PATH_SEPARATOR '\\'
#else
#    define PATH_SEPARATOR '/'
#endif

#if (FILENAME_MAX > 4096)
#    define PATHNAME_LENGTH_MAX FILENAME_MAX
#else
#    define PATHNAME_LENGTH_MAX 4096
#endif

/// @brief This is a global variable that is set on startup and holds the directory that was current when the program was loaded
qbs *g_startDir = nullptr;

/// @brief Gets the current working directory
/// @return A qbs containing the current working directory or an empty string on error
qbs *func__cwd() {
    std::string path;
    qbs *final;

    path.resize(FILENAME_MAX, '\0');

    for (;;) {
        if (getcwd((char *)path.data(), path.size())) {
            auto size = strlen(path.c_str());
            final = qbs_new(size, 1);
            memcpy(final->chr, path.data(), size);

            return final;
        } else {
            if (errno == ERANGE)
                path.resize(path.size() << 1); // buffer size was not sufficient; try again with a larger buffer
            else
                break; // some other error occurred
        }
    }

    final = qbs_new(0, 1);
    error(7);

    return final;
}

/// @brief Returns true if the specified directory exists
/// @param path The directory to check for
/// @return True if the directory exists
static inline bool DirectoryExists(const char *path) {
#ifdef QB64_WINDOWS
    auto x = GetFileAttributesA(path);

    return x != INVALID_FILE_ATTRIBUTES && (x & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat info;

    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
#endif
}

/// @brief Known directories (primarily Windows based, but we'll do our best to emulate on other platforms)
enum class KnownDirectory {
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

/// @brief This populates path with the full path for a known directory
/// @param kD Is a value from KnownDirectory (above)
/// @param path Is the string that will receive the directory path. The string may be changed
void GetKnownDirectory(KnownDirectory kD, std::string &path) {
#ifdef QB64_WINDOWS
    path.resize(PATHNAME_LENGTH_MAX, '\0'); // allocate something that is sufficiently large
#else
    auto envVar = getenv("HOME");
#endif

    switch (kD) {
    case KnownDirectory::DESKTOP: // %USERPROFILE%\OneDrive\Desktop
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar) {
            path.assign(envVar);
            path.append("/Desktop");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::DOCUMENTS: // %USERPROFILE%\OneDrive\Documents
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar) {
            path.assign(envVar);
            path.append("/Documents");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::PICTURES: // %USERPROFILE%\OneDrive\Pictures
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar) {
            path.assign(envVar);
            path.append("/Pictures");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::MUSIC: // %USERPROFILE%\Music
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_MYMUSIC | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar) {
            path.assign(envVar);
            path.append("/Music");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::VIDEOS: // %USERPROFILE%\Videos
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_MYVIDEO | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar) {
            path.assign(envVar);
#    ifdef QB64_MACOSX
            path.append("/Movies");
#    else
            path.append("/Videos");
#    endif
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::DOWNLOAD: // %USERPROFILE%\Downloads
#ifdef QB64_WINDOWS
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data()))) {
            // XP & SHGetFolderPathA do not support the concept of a Downloads folder, however it can be constructed
            path.resize(strlen(path.c_str()));
            path.append("\\Downloads");
            mkdir(path.c_str());
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#else
        if (envVar) {
            path.assign(envVar);
            path.append("/Downloads");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::APP_DATA: // %USERPROFILE%\AppData\Roaming
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar) {
            path.assign(envVar);
            path.append("/.config");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::LOCAL_APP_DATA: // %USERPROFILE%\AppData\Local
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar) {
            path.assign(envVar);
            path.append("/.local/share");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::PROGRAM_DATA: // %SystemDrive%\ProgramData
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar) {
            path.assign(envVar);
            path.append("/.local/share");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::SYSTEM_FONTS: // %SystemRoot%\Fonts
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_FONTS | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar) {
#    ifdef QB64_MACOSX
            path.assign("/System/Library/Fonts");
            if (!DirectoryExists(path.c_str())) {
                path.assign("/Library/Fonts");
                if (!DirectoryExists(path.c_str()))
                    path.clear();
            }
#    else
            path.assign("/usr/share/fonts");
            if (!DirectoryExists(path.c_str())) {
                path.assign("/usr/local/share/fonts");
                if (!DirectoryExists(path.c_str()))
                    path.clear();
            }
#    endif
        }
#endif
        break;

    case KnownDirectory::USER_FONTS: // %USERPROFILE%\AppData\Local\Microsoft\Windows\Fonts
#ifdef QB64_WINDOWS
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data()))) {
            path.resize(strlen(path.c_str()));
            path.append("\\Microsoft\\Windows\\Fonts");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#else
        if (envVar) {
            path.assign(envVar);
#    ifdef QB64_MACOSX
            path.append("/Library/Fonts");
            if (!DirectoryExists(path.c_str()))
                path.clear();
#    else
            path.append("/.local/share/fonts");
            if (!DirectoryExists(path.c_str())) {
                path.assign(envVar);
                path.append("/.fonts");
                if (!DirectoryExists(path.c_str()))
                    path.clear();
            }
#    endif
        }
#endif
        break;

    case KnownDirectory::TEMP: // %USERPROFILE%\AppData\Local\Temp
#ifdef QB64_WINDOWS
        GetTempPathA(path.size(), (char *)path.data());
#else
        path.assign("/var/tmp");
        if (!DirectoryExists(path.c_str())) {
            path.assign("/tmp");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::PROGRAM_FILES: // %SystemDrive%\Program Files
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar) {
            path.assign("/opt");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::PROGRAM_FILES_32: // %SystemDrive%\Program Files (x86)
#ifdef QB64_WINDOWS
#    ifdef _WIN64
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86 | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#    else
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#    endif
#else
        if (envVar) {
            path.assign("/opt");
            if (!DirectoryExists(path.c_str()))
                path.clear();
        }
#endif
        break;

    case KnownDirectory::HOME: // %USERPROFILE%
    default:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
        if (envVar)
            path.assign(envVar);
#endif
    }

    // Check if we got anything at all
    if (!strlen(path.c_str())) {
#ifdef QB64_WINDOWS
        path.resize(PATHNAME_LENGTH_MAX, '\0'); // just in case this was shrunk above

        if (FAILED(SHGetFolderPathA(NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data())))
            path.assign(".");
#else
        envVar = getenv("HOME"); // just in case this contains something other than home

        path.assign(envVar ? envVar : ".");
#endif
    }

    // Add the trailing slash
    path.resize(strlen(path.c_str()));
    if (path.back() != PATH_SEPARATOR)
        path.append(1, PATH_SEPARATOR);
}

/// @brief Returns common paths such as My Documents, My Pictures, My Music, Desktop
/// @param qbsContext Is the directory type
/// @return A qbs containing the directory or an empty string on error
qbs *func__dir(qbs *qbsContext) {
    std::string path, context(reinterpret_cast<char *>(qbsContext->chr), qbsContext->len);

    std::transform(context.begin(), context.end(), context.begin(), [](unsigned char c) { return std::toupper(c); });

    // The following is largely unchanged from what we previously had
    if (context.compare("TEXT") == 0 || context.compare("DOCUMENT") == 0 || context.compare("DOCUMENTS") == 0 || context.compare("MY DOCUMENTS") == 0) {
        GetKnownDirectory(KnownDirectory::DOCUMENTS, path);
    } else if (context.compare("MUSIC") == 0 || context.compare("AUDIO") == 0 || context.compare("SOUND") == 0 || context.compare("SOUNDS") == 0 ||
               context.compare("MY MUSIC") == 0) {
        GetKnownDirectory(KnownDirectory::MUSIC, path);
    } else if (context.compare("PICTURE") == 0 || context.compare("PICTURES") == 0 || context.compare("IMAGE") == 0 || context.compare("IMAGES") == 0 ||
               context.compare("MY PICTURES") == 0 || context.compare("DCIM") == 0 || context.compare("CAMERA") == 0 || context.compare("CAMERA ROLL") == 0 ||
               context.compare("PHOTO") == 0 || context.compare("PHOTOS") == 0) {
        GetKnownDirectory(KnownDirectory::PICTURES, path);
    } else if (context.compare("MOVIE") == 0 || context.compare("MOVIES") == 0 || context.compare("VIDEO") == 0 || context.compare("VIDEOS") == 0 ||
               context.compare("MY VIDEOS") == 0) {
        GetKnownDirectory(KnownDirectory::VIDEOS, path);
    } else if (context.compare("DOWNLOAD") == 0 || context.compare("DOWNLOADS") == 0) {
        GetKnownDirectory(KnownDirectory::DOWNLOAD, path);
    } else if (context.compare("DESKTOP") == 0) {
        GetKnownDirectory(KnownDirectory::DESKTOP, path);
    } else if (context.compare("APPDATA") == 0 || context.compare("APPLICATION DATA") == 0 || context.compare("PROGRAM DATA") == 0 ||
               context.compare("DATA") == 0) {
        GetKnownDirectory(KnownDirectory::APP_DATA, path);
    } else if (context.compare("LOCALAPPDATA") == 0 || context.compare("LOCAL APPLICATION DATA") == 0 || context.compare("LOCAL PROGRAM DATA") == 0 ||
               context.compare("LOCAL DATA") == 0) {
        GetKnownDirectory(KnownDirectory::LOCAL_APP_DATA, path);
    } else if (context.compare("PROGRAMFILES") == 0 || context.compare("PROGRAM FILES") == 0) {
        GetKnownDirectory(KnownDirectory::PROGRAM_FILES, path);
    } else if (context.compare("PROGRAMFILESX86") == 0 || context.compare("PROGRAMFILES X86") == 0 || context.compare("PROGRAM FILES X86") == 0 ||
               context.compare("PROGRAM FILES 86") == 0 || context.compare("PROGRAM FILES (X86)") == 0 || context.compare("PROGRAMFILES (X86)") == 0 ||
               context.compare("PROGRAM FILES(X86)") == 0) {
        GetKnownDirectory(KnownDirectory::PROGRAM_FILES_32, path);
    } else if (context.compare("TMP") == 0 || context.compare("TEMP") == 0 || context.compare("TEMP FILES") == 0) {
        GetKnownDirectory(KnownDirectory::TEMP, path);
    } else if (context.compare("HOME") == 0 || context.compare("USER") == 0 || context.compare("PROFILE") == 0 || context.compare("USERPROFILE") == 0 ||
               context.compare("USER PROFILE") == 0) {
        GetKnownDirectory(KnownDirectory::HOME, path);
    } else if (context.compare("FONT") == 0 || context.compare("FONTS") == 0) {
        GetKnownDirectory(KnownDirectory::SYSTEM_FONTS, path);
    } else if (context.compare("USERFONT") == 0 || context.compare("USER FONT") == 0 || context.compare("USERFONTS") == 0 ||
               context.compare("USER FONTS") == 0) {
        GetKnownDirectory(KnownDirectory::USER_FONTS, path);
    } else if (context.compare("PROGRAMDATA") == 0 || context.compare("COMMON PROGRAM DATA") == 0) {
        GetKnownDirectory(KnownDirectory::PROGRAM_DATA, path);
    } else {
        GetKnownDirectory(KnownDirectory::DESKTOP, path); // anything else defaults to the desktop where the user can easily see stuff
    }

    auto size = strlen(path.c_str());
    qbs *final = qbs_new(size, 1);
    memcpy(final->chr, path.data(), size);

    return final;
}

/// @brief Returns true if a directory specified exists
/// @param path The directory path
/// @return True if the directory exists
int32_t func__direxists(qbs *path) {
    if (new_error)
        return 0;

    static qbs *strz = nullptr;

    if (!strz)
        strz = qbs_new(0, 0);

    qbs_set(strz, qbs_add(path, qbs_new_txt_len("\0", 1)));

    return DirectoryExists(filepath_fix_directory(strz)) ? QB_TRUE : QB_FALSE;
}

/// @brief Returns true if a file specified exists
/// @param path The file path to check for
/// @return True if the file exists
static inline bool FileExists(const char *path) {
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
        return 0;

    static qbs *strz = nullptr;

    if (!strz)
        strz = qbs_new(0, 0);

    qbs_set(strz, qbs_add(path, qbs_new_txt_len("\0", 1)));

    return FileExists(filepath_fix_directory(strz)) ? QB_TRUE : QB_FALSE;
}

/// @brief Return the startup directory
/// @return A qbs containing the directory path
qbs *func__startdir() {
    qbs *temp = qbs_new(0, 1);

    qbs_set(temp, g_startDir);

    return temp;
}

/// @brief Changes the current directory
/// @param str The directory path to change to
void sub_chdir(qbs *str) {
    if (new_error)
        return;

    static qbs *strz = nullptr;

    if (!strz)
        strz = qbs_new(0, 0);

    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));

    if (chdir(filepath_fix_directory(strz)) == -1)
        error(76); // assume errno == ENOENT; path not found
}

/// @brief Checks if s is an empty string (either NULL or zero length)
/// @param s A null-terminated string or NULL
/// @return False is we have a valid string > length 0
static inline bool IsStringEmpty(const char *s) { return s == nullptr || s[0] == '\0'; }

/// @brief This is a basic pattern matching function used by GetDirectoryEntryName()
/// @param fileSpec The pattern to match
/// @param fileName The filename to match
/// @return True if it is a match, false otherwise
static inline bool IsPatternMatching(const char *fileSpec, const char *fileName) {
    auto spec = fileSpec;
    auto name = fileName;
    const char *any = nullptr;

    while (*spec || *name) {
        switch (*spec) {
        case '*': // handle wildcard '*' character
            any = spec;
            spec++;
            while (*name && *name != *spec)
                name++;
            break;

        case '?': // handle wildcard '?' character
            spec++;
            if (*name)
                name++;
            break;

        default: // compare non-wildcard characters
            if (*spec != *name) {
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
static inline bool HasPattern(const char *fileSpec) { return fileSpec != nullptr && (strchr(fileSpec, '*') || strchr(fileSpec, '?')); }

/// @brief An MS BASIC PDS DIR$ style function
/// @param fileSpec This can be a path with wildcard for the final level (i.e. C:/Windows/*.* or /usr/lib/* etc.)
/// @return Returns a file or directory name matching fileSpec or an empty string when there is nothing left
static const char *GetDirectoryEntryName(const char *fileSpec) {
    static DIR *pDir = nullptr;
    static char pattern[PATHNAME_LENGTH_MAX];
    static char entry[PATHNAME_LENGTH_MAX];

    entry[0] = '\0'; // set to an empty string

    if (!IsStringEmpty(fileSpec)) {
        // We got a filespec. Check if we have one already going and if so, close it
        if (pDir) {
            closedir(pDir);
            pDir = nullptr;
        }

        char dirName[PATHNAME_LENGTH_MAX]; // we only need this for opendir()

        if (HasPattern(fileSpec)) {
            // We have a pattern. Check if we have a path in it
            auto p = strrchr(fileSpec, '/'); // try *nix style separator
#ifdef QB64_WINDOWS
            if (!p)
                p = strrchr(fileSpec, '\\'); // try windows style separator
#endif

            if (p) {
                // Split the path and the filespec
                strncpy(pattern, p + 1, PATHNAME_LENGTH_MAX);
                pattern[PATHNAME_LENGTH_MAX - 1] = '\0';
                auto len = std::min<size_t>((p - fileSpec) + 1, PATHNAME_LENGTH_MAX - 1);
                memcpy(dirName, fileSpec, len);
                dirName[len] = '\0';
            } else {
                // No path. Use the current path
                strncpy(pattern, fileSpec, PATHNAME_LENGTH_MAX);
                pattern[PATHNAME_LENGTH_MAX - 1] = '\0';
                strcpy(dirName, "./");
            }
        } else {
            // No pattern. Check if this is a file and simply return the name if it exists
            if (FileExists(fileSpec)) {
                strncpy(entry, filepath_get_filename(fileSpec), PATHNAME_LENGTH_MAX);
                entry[PATHNAME_LENGTH_MAX - 1] = '\0';

                return entry;
            }

            // Else, We'll just assume it's a directory
            strncpy(dirName, fileSpec, PATHNAME_LENGTH_MAX);
            dirName[PATHNAME_LENGTH_MAX - 1] = '\0';
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

            if (IsPatternMatching(pattern, pDirent->d_name)) {
                strncpy(entry, pDirent->d_name, PATHNAME_LENGTH_MAX);
                entry[PATHNAME_LENGTH_MAX - 1] = '\0';

                break;
            }
        }
    }

    return entry;
}

/// @brief This mimics MS BASIC PDS 7.1 & VBDOS 1.0 DIR$() function
/// @param qbsFileSpec This can be a path with wildcard for the final level (i.e. C:/Windows/*.* or /usr/lib/* etc.)
/// @param passed Flags for optional parameters
/// @return Retuns a qbs with the directory entry name or an empty string if there are no more entries
qbs *func__files(qbs *qbsFileSpec, int32_t passed) {
    const char *entry;
    qbs *final;

    // Check if fresh arguments were passed and we need to begin a new session
    if (passed) {
        std::string fileSpec(reinterpret_cast<char *>(qbsFileSpec->chr), qbsFileSpec->len);

        entry = GetDirectoryEntryName(fileSpec.c_str());

        if (IsStringEmpty(entry)) {
            // This is per MS BASIC PDS 7.1 and VBDOS 1.0 behavior
            final = qbs_new(0, 1);
            error(53);
            return final;
        }
    } else {
        entry = GetDirectoryEntryName(nullptr);
    }

    auto size = strlen(entry);

    // TODO: Need to join the base directory here!
    if (DirectoryExists(entry)) {
        // Add a trailing slash if it is a directory
        final = qbs_new(size + 1, 1);
        memcpy(final->chr, entry, size);
        final->chr[size] = PATH_SEPARATOR;
    } else {
        final = qbs_new(size, 1);
        memcpy(final->chr, entry, size);
    }

    return final;
}

/// @brief Prints a list of files in the current directory using a file specification
/// @param str Is a string containing a path (it can include wildcards)
/// @param passed Optional parameters
void sub_files(qbs *str, int32_t passed) {
    if (new_error)
        return;

    static int32_t i, i2, i3;
    static qbs *strz = nullptr;

    if (!strz)
        strz = qbs_new(0, 0);

    if (passed) {
        qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
    } else {
        qbs_set(strz, qbs_new_txt_len("\0", 1));
    }

// TODO: Cleanup this mess. Eww.
#ifdef QB64_WINDOWS
    static WIN32_FIND_DATAA fd;
    static HANDLE hFind;
    static qbs *strpath = NULL;
    if (!strpath)
        strpath = qbs_new(0, 0);
    static qbs *strz2 = NULL;
    if (!strz2)
        strz2 = qbs_new(0, 0);

    i = 0;
    if (strz->len >= 2) {
        if (strz->chr[strz->len - 2] == 92)
            i = 1;
    } else
        i = 1;
    if (i) {                           // add * (and new NULL term.)
        strz->chr[strz->len - 1] = 42; //"*"
        qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
    }

    qbs_set(strpath, strz);

    for (i = strpath->len; i > 0; i--) {
        if ((strpath->chr[i - 1] == 47) || (strpath->chr[i - 1] == 92)) {
            strpath->len = i;
            break;
        }
    } // i
    if (i == 0)
        strpath->len = 0; // no path specified

    // print the current path
    // note: for QBASIC compatibility reasons it does not print the directory name of the files being displayed
    static uint8 curdir[4096];
    static uint8 curdir2[4096];
    i2 = GetCurrentDirectoryA(4096, (char *)curdir);
    if (i2) {
        i2 = GetShortPathNameA((char *)curdir, (char *)curdir2, 4096);
        if (i2) {
            qbs_set(strz2, qbs_ucase(qbs_new_txt_len((char *)curdir2, i2)));
            qbs_print(strz2, 1);
        } else {
            error(5);
            return;
        }
    } else {
        error(5);
        return;
    }

    hFind = FindFirstFileA(filepath_fix_directory(strz), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        error(53);
        return;
    } // file not found
    do {

        if (!fd.cAlternateFileName[0]) { // no alternate filename exists
            qbs_set(strz2, qbs_ucase(qbs_new_txt_len(fd.cFileName, strlen(fd.cFileName))));
        } else {
            qbs_set(strz2, qbs_ucase(qbs_new_txt_len(fd.cAlternateFileName, strlen(fd.cAlternateFileName))));
        }

        if (strz2->len < 12) { // padding required
            qbs_set(strz2, qbs_add(strz2, func_space(12 - strz2->len)));
            i2 = 0;
            for (i = 0; i < 12; i++) {
                if (strz2->chr[i] == 46) {
                    memmove(&strz2->chr[8], &strz2->chr[i], 4);
                    memset(&strz2->chr[i], 32, 8 - i);
                    break;
                }
            } // i
        }     // padding

        // add "      " or "<DIR> "
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            qbs_set(strz2, qbs_add(strz2, qbs_new_txt_len("<DIR> ", 6)));
        } else {
            qbs_set(strz2, qbs_add(strz2, func_space(6)));
        }

        makefit(strz2);
        qbs_print(strz2, 0);

    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    static ULARGE_INTEGER FreeBytesAvailableToCaller;
    static ULARGE_INTEGER TotalNumberOfBytes;
    static ULARGE_INTEGER TotalNumberOfFreeBytes;
    static int64 bytes;
    static char *cp;
    qbs_set(strpath, qbs_add(strpath, qbs_new_txt_len("\0", 1)));
    cp = (char *)strpath->chr;
    if (strpath->len == 1)
        cp = NULL;
    if (GetDiskFreeSpaceExA(cp, &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)) {
        bytes = *(int64 *)(void *)&FreeBytesAvailableToCaller;
    } else {
        bytes = 0;
    }
    if (func_pos(NULL) > 1) {
        strz2->len = 0;
        qbs_print(strz2, 1);
    } // new line if necessary
    qbs_set(strz2, qbs_add(qbs_str(bytes), qbs_new_txt_len(" Bytes free", 11)));
    qbs_print(strz2, 1);

#endif
}

/// @brief Deletes files from disk
/// @param str The file(s) to delete (may contain wildcard at the final level)
void sub_kill(qbs *str) {
    if (new_error)
        return;

    static qbs *strz = nullptr;

    if (!strz)
        strz = qbs_new(0, 0);

    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));

    std::string directory, fileName;

    filepath_split(filepath_fix_directory(strz), directory, fileName);       // split the file path
    auto entry = GetDirectoryEntryName(reinterpret_cast<char *>(strz->chr)); // get the first entry

    // Keep looking through the entries until we file a file
    while (!IsStringEmpty(entry)) {
        filepath_join(fileName, directory, entry);

        if (FileExists(fileName.c_str()))
            break;

        entry = GetDirectoryEntryName(nullptr); // get the next entry
    }

    // Check if we have exhausted the entries without ever finding a file
    if (IsStringEmpty(entry)) {
        // This behavior is per QBasic 1.1
        error(53);
        return;
    }

    // Process all matches
    do {
        // We'll delete only if it is a file
        if (FileExists(fileName.c_str())) {
            if (remove(fileName.c_str())) {
                auto i = errno;

                if (i == ENOENT) {
                    error(53);
                    return;
                } // file not found

                if (i == EACCES) {
                    error(75);
                    return;
                } // path / file access error

                error(64); // bad file name (assumed)
            }
        }

        entry = GetDirectoryEntryName(nullptr); // get the next entry
        filepath_join(fileName, directory, entry);
    } while (!IsStringEmpty(entry));
}

/// @brief Creates a new directory
/// @param str The directory path name to create
void sub_mkdir(qbs *str) {
    if (new_error)
        return;

    static qbs *strz = nullptr;

    if (!strz)
        strz = qbs_new(0, 0);

    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));

#ifdef QB64_WINDOWS
    if (mkdir(filepath_fix_directory(strz)) == -1) {
#else
    if (mkdir(filepath_fix_directory(strz), S_IRWXU | S_IRWXG) == -1) {
#endif
        if (errno == EEXIST) {
            error(75);
            return;
        } // path / file access error

        error(76); // assume errno == ENOENT; path not found
    }
}

/// @brief Renames a file or directory
/// @param oldname The old file / directory name
/// @param newname The new file / directory name
void sub_name(qbs *oldname, qbs *newname) {
    if (new_error)
        return;

    static qbs *strz = nullptr, *strz2 = nullptr;

    if (!strz)
        strz = qbs_new(0, 0);

    if (!strz2)
        strz2 = qbs_new(0, 0);

    qbs_set(strz, qbs_add(oldname, qbs_new_txt_len("\0", 1)));
    qbs_set(strz2, qbs_add(newname, qbs_new_txt_len("\0", 1)));

    if (rename(filepath_fix_directory(strz), filepath_fix_directory(strz2))) {
        auto i = errno;

        if (i == ENOENT) {
            error(53);
            return;
        } // file not found

        if (i == EINVAL) {
            error(64);
            return;
        } // bad file name

        if (i == EACCES) {
            error(75);
            return;
        } // path / file access error

        error(5); // illegal function call (assumed)
    }
}

/// @brief Deletes an empty directory
/// @param str The path name of the directory to delete
void sub_rmdir(qbs *str) {
    if (new_error)
        return;

    static qbs *strz = nullptr;

    if (!strz)
        strz = qbs_new(0, 0);

    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));

    if (rmdir(filepath_fix_directory(strz)) == -1) {
        if (errno == ENOTEMPTY) {
            error(75);
            return;
        } // path/file access error; not an empty directory

        error(76); // assume errno == ENOENT; path not found
    }
}
