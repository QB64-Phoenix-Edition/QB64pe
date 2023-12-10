#include "libqb-common.h"

#include "filepath.h"
#include "filesystem.h"

#include "../../libqb.h"

// Get Current Working Directory
qbs *func__cwd() {
    qbs *final, *tqbs;
    int length;
    char *buf, *ret;

#if defined QB64_WINDOWS
    length = GetCurrentDirectoryA(0, NULL);
    buf = (char *)malloc(length);
    if (!buf) {
        error(7); //"Out of memory"
        return tqbs;
    }
    if (GetCurrentDirectoryA(length, buf) != --length) { // Sanity check
        free(buf);                                       // It's good practice
        tqbs = qbs_new(0, 1);
        error(51); //"Internal error"
        return tqbs;
    }
#elif defined QB64_UNIX
    length = 512;
    while (1) {
        buf = (char *)malloc(length);
        if (!buf) {
            tqbs = qbs_new(0, 1);
            error(7);
            return tqbs;
        }
        ret = getcwd(buf, length);
        if (ret)
            break;
        if (errno != ERANGE) {
            tqbs = qbs_new(0, 1);
            error(51);
            return tqbs;
        }
        free(buf);
        length += 512;
    }
    length = strlen(ret);
    ret = (char *)realloc(ret, length); // Chops off the null byte
    if (!ret) {
        tqbs = qbs_new(0, 1);
        error(7);
        return tqbs;
    }
    buf = ret;
#endif
    final = qbs_new(length, 1);
    memcpy(final->chr, buf, length);
    free(buf);
    return final;
}

qbs *func__dir(qbs *context_in) {

    static qbs *context = NULL;
    if (!context) {
        context = qbs_new(0, 0);
    }

    qbs_set(context, qbs_ucase(context_in));

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("TEXT")) || qbs_equal(qbs_ucase(context), qbs_new_txt("DOCUMENT")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("DOCUMENTS")) || qbs_equal(qbs_ucase(context), qbs_new_txt("MY DOCUMENTS"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 5, NULL, 0, osPath))) { // Documents
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("MUSIC")) || qbs_equal(qbs_ucase(context), qbs_new_txt("AUDIO")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("SOUND")) || qbs_equal(qbs_ucase(context), qbs_new_txt("SOUNDS")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("MY MUSIC"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 13, NULL, 0, osPath))) { // Music
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("PICTURE")) || qbs_equal(qbs_ucase(context), qbs_new_txt("PICTURES")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("IMAGE")) || qbs_equal(qbs_ucase(context), qbs_new_txt("IMAGES")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("MY PICTURES"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 39, NULL, 0, osPath))) { // Pictures
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("DCIM")) || qbs_equal(qbs_ucase(context), qbs_new_txt("CAMERA")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("CAMERA ROLL")) || qbs_equal(qbs_ucase(context), qbs_new_txt("PHOTO")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("PHOTOS"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 39, NULL, 0, osPath))) { // Pictures
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("MOVIE")) || qbs_equal(qbs_ucase(context), qbs_new_txt("MOVIES")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("VIDEO")) || qbs_equal(qbs_ucase(context), qbs_new_txt("VIDEOS")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("MY VIDEOS"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 14, NULL, 0, osPath))) { // Videos
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("DOWNLOAD")) || qbs_equal(qbs_ucase(context), qbs_new_txt("DOWNLOADS"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 0x0028, NULL, 0, osPath))) { // user folder
            // XP & SHGetFolderPathA do not support the concept of a Downloads folder, however it can be constructed
            mkdir((char *)((qbs_add(qbs_new_txt(osPath), qbs_new_txt_len("\\Downloads\0", 11)))->chr));
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\Downloads\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("DESKTOP"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 0, NULL, 0, osPath))) { // Desktop
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("APPDATA")) || qbs_equal(qbs_ucase(context), qbs_new_txt("APPLICATION DATA")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("PROGRAM DATA")) || qbs_equal(qbs_ucase(context), qbs_new_txt("DATA"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 0x001a, NULL, 0, osPath))) { // CSIDL_APPDATA (%APPDATA%)
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("LOCALAPPDATA")) || qbs_equal(qbs_ucase(context), qbs_new_txt("LOCAL APPLICATION DATA")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("LOCAL PROGRAM DATA")) || qbs_equal(qbs_ucase(context), qbs_new_txt("LOCAL DATA"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 0x001c, NULL, 0, osPath))) { // CSIDL_LOCAL_APPDATA (%LOCALAPPDATA%)
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("PROGRAMFILES")) || qbs_equal(qbs_ucase(context), qbs_new_txt("PROGRAM FILES"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 0x0026, NULL, 0, osPath))) { // CSIDL_PROGRAM_FILES (%PROGRAMFILES%)
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("PROGRAMFILESX86")) || qbs_equal(qbs_ucase(context), qbs_new_txt("PROGRAMFILES X86")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("PROGRAM FILES X86")) || qbs_equal(qbs_ucase(context), qbs_new_txt("PROGRAM FILES 86")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("PROGRAM FILES (X86)")) || qbs_equal(qbs_ucase(context), qbs_new_txt("PROGRAMFILES (X86)")) ||
        qbs_equal(qbs_ucase(context), qbs_new_txt("PROGRAM FILES(X86)"))) {
#ifdef QB64_WINDOWS &&_WIN64
        CHAR osPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, 0x002a, NULL, 0, osPath))) { // CSIDL_PROGRAM_FILES (%PROGRAMFILES(X86)%)
            return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
        }
#endif
    }

    if (qbs_equal(qbs_ucase(context), qbs_new_txt("TEMP")) || qbs_equal(qbs_ucase(context), qbs_new_txt("TEMP FILES"))) {
#ifdef QB64_WINDOWS
        CHAR osPath[MAX_PATH + 1];
        DWORD pathlen;
        pathlen = GetTempPathA(261, osPath); //%TEMP%
        char path[pathlen];
        memcpy(path, &osPath, pathlen);
        if (pathlen > 0) {
            return qbs_new_txt(path);
        }
#endif
    }

// general fallback location
#ifdef QB64_WINDOWS
    CHAR osPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, 0, NULL, 0, osPath))) { // desktop
        return qbs_add(qbs_new_txt(osPath), qbs_new_txt("\\"));
    }
    return qbs_new_txt(".\\"); // current location
#else
    return qbs_new_txt("./"); // current location
#endif
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
    x = GetFileAttributes(filepath_fix_directory(strz));
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
    x = GetFileAttributes(filepath_fix_directory(strz));
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
    static WIN32_FIND_DATA fd;
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
    i2 = GetCurrentDirectory(4096, (char *)curdir);
    if (i2) {
        i2 = GetShortPathName((char *)curdir, (char *)curdir2, 4096);
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

    hFind = FindFirstFile(filepath_fix_directory(strz), &fd);
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

    } while (FindNextFile(hFind, &fd));
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
    if (GetDiskFreeSpaceEx(cp, &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)) {
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
    static WIN32_FIND_DATA fd;
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
    hFind = FindFirstFile(filepath_fix_directory(strz), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        error(53);
        return;
    } // file not found
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            qbs_set(strfullz, qbs_add(strpath, qbs_new_txt_len(fd.cFileName, strlen(fd.cFileName) + 1)));
            if (!DeleteFile((char *)strfullz->chr)) {
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
    } while (FindNextFile(hFind, &fd));
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
