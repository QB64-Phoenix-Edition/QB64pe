//----------------------------------------------------------------------------------------------------
//  QB64-PE filesystem related functions
//-----------------------------------------------------------------------------------------------------

#include "libqb-common.h"

#include "filepath.h"
#include "filesystem.h"

#include "../../libqb.h"

#include <algorithm>
#include <sys/stat.h>
#ifdef QB64_WINDOWS
#    include <shlobj.h>
#endif

#ifdef QB64_BACKSLASH_FILESYSTEM
#    define __PATH_SEPARATOR '\\'
#else
#    define __PATH_SEPARATOR '/'
#endif

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
static bool __DirectoryExists(std::string &path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode));
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
void __GetKnownDirectory(KnownDirectory kD, std::string &path) {
    path.resize(FILENAME_MAX << 2, '\0'); // allocate something that is sufficiently large

    switch (kD) {
    case KnownDirectory::DESKTOP:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::DOCUMENTS:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::PICTURES:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::MUSIC:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_MYMUSIC | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::VIDEOS:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_MYVIDEO | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::DOWNLOAD:
#ifdef QB64_WINDOWS
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data()))) {
            // XP & SHGetFolderPathA do not support the concept of a Downloads folder, however it can be constructed
            path.resize(strlen(path.c_str()));
            path.append("\\Downloads");
            mkdir(path.c_str());
            if (!__DirectoryExists(path))
                path.clear();
        }
#else
#endif
        break;

    case KnownDirectory::APP_DATA:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::LOCAL_APP_DATA:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::PROGRAM_DATA:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::SYSTEM_FONTS:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_FONTS | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::USER_FONTS:
#ifdef QB64_WINDOWS
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data()))) {
            path.resize(strlen(path.c_str()));
            path.append("\\Microsoft\\Windows\\Fonts");
            if (!__DirectoryExists(path))
                path.clear();
        }
#else
#endif
        break;

    case KnownDirectory::TEMP:
#ifdef QB64_WINDOWS
        GetTempPathA(path.size(), (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::PROGRAM_FILES:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
        break;

    case KnownDirectory::PROGRAM_FILES_32:
#ifdef QB64_WINDOWS
#    ifdef _WIN64
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86 | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#    else
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#    endif
#else
#endif
        break;

    case KnownDirectory::HOME:
    default:
#ifdef QB64_WINDOWS
        SHGetFolderPathA(NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data());
#else
#endif
    }

    // Check if we got anything at all
    if (!strlen(path.c_str())) {
        path.resize(FILENAME_MAX << 2, '\0'); // just in case this was shrunk above
#ifdef QB64_WINDOWS
        if (FAILED(SHGetFolderPathA(NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, (char *)path.data())))
            path.assign(".");
#else
        path.assign(getenv("HOME") ? getenv("HOME") : ".");
#endif
    }

    // Add the trailing slash
    path.resize(strlen(path.c_str()));
    if (path.back() != __PATH_SEPARATOR)
        path.append(1, __PATH_SEPARATOR);
}

/// @brief Returns common paths such as My Documents, My Pictures, My Music, Desktop
/// @param context_in Is the directory type
/// @return A qbs containing the directory or an empty string on error
qbs *func__dir(qbs *context_in) {
    std::string context, path;

    context.assign((const char *)context_in->chr, context_in->len);
    std::transform(context.begin(), context.end(), context.begin(), [](unsigned char c) { return std::toupper(c); });

    if (context.compare("TEXT") == 0 || context.compare("DOCUMENT") == 0 || context.compare("DOCUMENTS") == 0 || context.compare("MY DOCUMENTS") == 0) {
        __GetKnownDirectory(KnownDirectory::DOCUMENTS, path);
    } else if (context.compare("MUSIC") == 0 || context.compare("AUDIO") == 0 || context.compare("SOUND") == 0 || context.compare("SOUNDS") == 0 ||
               context.compare("MY MUSIC") == 0) {
        __GetKnownDirectory(KnownDirectory::MUSIC, path);
    } else if (context.compare("PICTURE") == 0 || context.compare("PICTURES") == 0 || context.compare("IMAGE") == 0 || context.compare("IMAGES") == 0 ||
               context.compare("MY PICTURES") == 0 || context.compare("DCIM") == 0 || context.compare("CAMERA") == 0 || context.compare("CAMERA ROLL") == 0 ||
               context.compare("PHOTO") == 0 || context.compare("PHOTOS") == 0) {
        __GetKnownDirectory(KnownDirectory::PICTURES, path);
    } else if (context.compare("MOVIE") == 0 || context.compare("MOVIES") == 0 || context.compare("VIDEO") == 0 || context.compare("VIDEOS") == 0 ||
               context.compare("MY VIDEOS") == 0) {
        __GetKnownDirectory(KnownDirectory::VIDEOS, path);
    } else if (context.compare("DOWNLOAD") == 0 || context.compare("DOWNLOADS") == 0) {
        __GetKnownDirectory(KnownDirectory::DOWNLOAD, path);
    } else if (context.compare("DESKTOP") == 0) {
        __GetKnownDirectory(KnownDirectory::DESKTOP, path);
    } else if (context.compare("APPDATA") == 0 || context.compare("APPLICATION DATA") == 0 || context.compare("PROGRAM DATA") == 0 ||
               context.compare("DATA") == 0) {
        __GetKnownDirectory(KnownDirectory::APP_DATA, path);
    } else if (context.compare("LOCALAPPDATA") == 0 || context.compare("LOCAL APPLICATION DATA") == 0 || context.compare("LOCAL PROGRAM DATA") == 0 ||
               context.compare("LOCAL DATA") == 0) {
        __GetKnownDirectory(KnownDirectory::LOCAL_APP_DATA, path);
    } else if (context.compare("PROGRAMFILES") == 0 || context.compare("PROGRAM FILES") == 0) {
        __GetKnownDirectory(KnownDirectory::PROGRAM_FILES, path);
    } else if (context.compare("PROGRAMFILESX86") == 0 || context.compare("PROGRAMFILES X86") == 0 || context.compare("PROGRAM FILES X86") == 0 ||
               context.compare("PROGRAM FILES 86") == 0 || context.compare("PROGRAM FILES (X86)") == 0 || context.compare("PROGRAMFILES (X86)") == 0 ||
               context.compare("PROGRAM FILES(X86)") == 0) {
        __GetKnownDirectory(KnownDirectory::PROGRAM_FILES_32, path);
    } else if (context.compare("TMP") == 0 || context.compare("TEMP") == 0 || context.compare("TEMP FILES") == 0) {
        __GetKnownDirectory(KnownDirectory::TEMP, path);
    } else if (context.compare("HOME") == 0 || context.compare("USER") == 0 || context.compare("PROFILE") == 0 || context.compare("USERPROFILE") == 0 ||
               context.compare("USER PROFILE") == 0) {
        __GetKnownDirectory(KnownDirectory::HOME, path);
    } else if (context.compare("FONT") == 0 || context.compare("FONTS") == 0) {
        __GetKnownDirectory(KnownDirectory::SYSTEM_FONTS, path);
    } else if (context.compare("USERFONT") == 0 || context.compare("USER FONT") == 0 || context.compare("USERFONTS") == 0 ||
               context.compare("USER FONTS") == 0) {
        __GetKnownDirectory(KnownDirectory::USER_FONTS, path);
    } else if (context.compare("PROGRAMDATA") == 0 || context.compare("COMMON PROGRAM DATA") == 0) {
        __GetKnownDirectory(KnownDirectory::PROGRAM_DATA, path);
    } else {
        __GetKnownDirectory(KnownDirectory::DESKTOP, path); // anything else defaults to the desktop where the user can easily see stuff
    }

    auto size = strlen(path.c_str());
    qbs *final = qbs_new(size, 1);
    memcpy(final->chr, path.data(), size);

    return final;
}

int32 func__direxists(qbs *file) {
    if (new_error)
        return 0;
    static qbs *strz = NULL;
    if (!strz)
        strz = qbs_new(0, 0);
    qbs_set(strz, qbs_add(file, qbs_new_txt_len("\0", 1)));
#ifdef QB64_WINDOWS
    static int32 x;
    x = GetFileAttributesA(filepath_fix_directory(strz));
    if (x == INVALID_FILE_ATTRIBUTES)
        return 0;
    if (x & FILE_ATTRIBUTE_DIRECTORY)
        return -1;
    return 0;
#elif defined(QB64_UNIX)
    struct stat sb;
    if (stat(filepath_fix_directory(strz), &sb) == 0 && S_ISDIR(sb.st_mode))
        return -1;
    return 0;
#else
    return 0; // default response
#endif
}

int32 func__fileexists(qbs *file) {
    if (new_error)
        return 0;
    static qbs *strz = NULL;
    if (!strz)
        strz = qbs_new(0, 0);
    qbs_set(strz, qbs_add(file, qbs_new_txt_len("\0", 1)));
#ifdef QB64_WINDOWS
    static int32 x;
    x = GetFileAttributesA(filepath_fix_directory(strz));
    if (x == INVALID_FILE_ATTRIBUTES)
        return 0;
    if (x & FILE_ATTRIBUTE_DIRECTORY)
        return 0;
    return -1;
#elif defined(QB64_UNIX)
    struct stat sb;
    if (stat(filepath_fix_directory(strz), &sb) == 0 && S_ISREG(sb.st_mode))
        return -1;
    return 0;
#else
    // generic method (not currently used)
    static std::ifstream fh;
    fh.open(filepath_fix_directory(strz), std::ios::binary | std::ios::in);
    if (fh.is_open() == NULL) {
        fh.clear(std::ios::goodbit);
        return 0;
    }
    fh.clear(std::ios::goodbit);
    fh.close();
    return -1;
#endif
}

qbs *g_startDir = nullptr; // set on startup

qbs *func__startdir() {
    qbs *temp = qbs_new(0, 1);
    qbs_set(temp, g_startDir);
    return temp;
}

void sub_chdir(qbs *str) {

    if (new_error)
        return;
    static qbs *strz = NULL;
    if (!strz)
        strz = qbs_new(0, 0);
    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
    if (chdir(filepath_fix_directory(strz)) == -1) {
        // assume errno==ENOENT
        error(76); // path not found
    }

    static int32 tmp_long;
    static int32 got_ports = 0;
}

void sub_files(qbs *str, int32 passed) {
    if (new_error)
        return;

    static int32 i, i2, i3;
    static qbs *strz = NULL;
    if (!strz)
        strz = qbs_new(0, 0);

    if (passed) {
        qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
    } else {
        qbs_set(strz, qbs_new_txt_len("\0", 1));
    }

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

void sub_kill(qbs *str) {
    // note: file not found returned for non-existant paths too
    //      file already open returned if access unavailable
    if (new_error)
        return;
    static int32 i;
    static qbs *strz = NULL;
    if (!strz)
        strz = qbs_new(0, 0);
    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
#ifdef QB64_WINDOWS
    static WIN32_FIND_DATAA fd;
    static HANDLE hFind;
    static qbs *strpath = NULL;
    if (!strpath)
        strpath = qbs_new(0, 0);
    static qbs *strfullz = NULL;
    if (!strfullz)
        strfullz = qbs_new(0, 0);
    // find path
    qbs_set(strpath, strz);
    for (i = strpath->len; i > 0; i--) {
        if ((strpath->chr[i - 1] == 47) || (strpath->chr[i - 1] == 92)) {
            strpath->len = i;
            break;
        }
    } // i
    if (i == 0)
        strpath->len = 0; // no path specified
    static int32 count;
    count = 0;
    hFind = FindFirstFileA(filepath_fix_directory(strz), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        error(53);
        return;
    } // file not found
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            qbs_set(strfullz, qbs_add(strpath, qbs_new_txt_len(fd.cFileName, strlen(fd.cFileName) + 1)));
            if (!DeleteFileA((char *)strfullz->chr)) {
                i = GetLastError();
                if ((i == 5) || (i == 19) || (i == 33) || (i == 32)) {
                    FindClose(hFind);
                    error(55);
                    return;
                } // file already open
                FindClose(hFind);
                error(53);
                return; // file not found
            }
            count++;
        } // not a directory
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
    if (!count) {
        error(53);
        return;
    } // file not found
    return;
#else
    if (remove(filepath_fix_directory(strz))) {
        i = errno;
        if (i == ENOENT) {
            error(53);
            return;
        } // file not found
        if (i == EACCES) {
            error(75);
            return;
        }          // path/file access error
        error(64); // bad file name (assumed)
    }
#endif
}

void sub_mkdir(qbs *str) {
    if (new_error)
        return;
    static qbs *strz = NULL;
    if (!strz)
        strz = qbs_new(0, 0);
    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
#ifdef QB64_UNIX
    if (mkdir(filepath_fix_directory(strz), 0770) == -1) {
#else
    if (mkdir(filepath_fix_directory(strz)) == -1) {
#endif
        if (errno == EEXIST) {
            error(75);
            return;
        } // path/file access error
        // assume errno==ENOENT
        error(76); // path not found
    }
}

void sub_name(qbs *oldname, qbs *newname) {
    if (new_error)
        return;
    static qbs *strz = NULL;
    if (!strz)
        strz = qbs_new(0, 0);
    static qbs *strz2 = NULL;
    if (!strz2)
        strz2 = qbs_new(0, 0);
    static int32 i;
    qbs_set(strz, qbs_add(oldname, qbs_new_txt_len("\0", 1)));
    qbs_set(strz2, qbs_add(newname, qbs_new_txt_len("\0", 1)));
    if (rename(filepath_fix_directory(strz), filepath_fix_directory(strz2))) {
        i = errno;
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
        }         // path/file access error
        error(5); // Illegal function call (assumed)
    }
}

void sub_rmdir(qbs *str) {
    if (new_error)
        return;
    static qbs *strz = NULL;
    if (!strz)
        strz = qbs_new(0, 0);
    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
    if (rmdir(filepath_fix_directory(strz)) == -1) {
        if (errno == ENOTEMPTY) {
            error(75);
            return;
        } // path/file access error
        // assume errno==ENOENT
        error(76); // path not found
    }
}
