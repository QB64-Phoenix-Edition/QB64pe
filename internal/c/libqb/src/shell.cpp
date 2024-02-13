
#include "libqb-common.h"

#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef QB64_WINDOWS
# include <windows.h>
#endif

#include "command.h"
#include "datetime.h"
#include "error_handle.h"
#include "qbs.h"
#include "shell.h"

// FIXME
extern int32_t console;
extern int32_t console_active;

int32_t shell_call_in_progress = 0;

#ifdef QB64_WINDOWS
static int32_t cmd_available = -1;

static int32_t cmd_ok() {
    if (cmd_available == -1) {
        static STARTUPINFO si;
        static PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        if (CreateProcess(NULL,             // No module name (use command line)
                          "cmd.exe /c ver", // Command line
                          NULL,             // Process handle not inheritable
                          NULL,             // Thread handle not inheritable
                          FALSE,            // Set handle inheritance to FALSE
                          CREATE_NO_WINDOW, // No creation flags
                          NULL,             // Use parent's environment block
                          NULL,             // Use parent's starting directory
                          &si,              // Pointer to STARTUPINFO structure
                          &pi               // Pointer to PROCESS_INFORMATION structure
                          )) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            cmd_available = 1;
        } else {
            cmd_available = 0;
        }
    } //-1
    return cmd_available;
}
#endif

static int32_t cmd_command(qbs *str2) {
    static qbs *str = NULL;
    static int32_t s;
    if (!str)
        str = qbs_new(0, 0);
    qbs_set(str, qbs_ucase(str2));
    s = 0;
    if (qbs_equal(str, qbs_new_txt("ASSOC")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("BREAK")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("BCDBOOT")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("BCDEDIT")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("CALL")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("CD")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("CHDIR")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("CLS")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("COLOR")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("COPY")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("DATE")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("DEFRAG")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("DEL")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("DIR")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("ECHO")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("ENDLOCAL")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("ERASE")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("FOR")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("FTYPE")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("GOTO")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("GRAFTABL")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("IF")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("MD")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("MKDIR")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("MKLINK")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("MOVE")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("PATH")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("PAUSE")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("POPD")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("PROMPT")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("PUSHD")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("RD")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("REM")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("REN")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("RENAME")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("RMDIR")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("SET")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("SETLOCAL")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("SHIFT")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("START")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("TIME")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("TITLE")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("TYPE")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("VER")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("VERIFY")))
        s = 1;
    if (qbs_equal(str, qbs_new_txt("VOL")))
        s = 1;
    return s;
}

// FIXME: Move this elsewhere
extern int32_t full_screen;
extern int32_t full_screen_set;

int64_t func_shell(qbs *str) {
    if (is_error_pending())
        return 1;

    int64_t return_code;

    // exit full screen mode if necessary
    static int32_t full_screen_mode;
    full_screen_mode = full_screen;
    if (full_screen_mode) {
        full_screen_set = 0;
        do {
            Sleep(0);
        } while (full_screen);
    } // full_screen_mode
    static qbs *strz = NULL;
    static qbs *str1 = NULL;
    static qbs *str1z = NULL;
    static qbs *str2 = NULL;
    static qbs *str2z = NULL;
    static int32_t i;

    static int32_t use_console;
    use_console = 0;
    if (console) {
        if (console_active) {
            use_console = 1;
        }
    }

    if (!strz)
        strz = qbs_new(0, 0);
    if (!str1)
        str1 = qbs_new(0, 0);
    if (!str1z)
        str1z = qbs_new(0, 0);
    if (!str2)
        str2 = qbs_new(0, 0);
    if (!str2z)
        str2z = qbs_new(0, 0);

    if (str->len) {

#ifdef QB64_WINDOWS

        if (use_console) {
            qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
            shell_call_in_progress = 1;
            /*
                freopen("stdout.buf", "w", stdout);
                freopen("stderr.buf", "w", stderr);
            */
            return_code = system((char *)strz->chr);
            /*
                freopen("CON", "w", stdout);
                freopen("CON", "w", stderr);
                static char buf[1024];
                static int buflen;
                static int fd;
                fd = open("stdout.buf", O_RDONLY);
                while((buflen = read(fd, buf, 1024)) > 0)
                {
                write(1, buf, buflen);
                }
                close(fd);
                fd = open("stderr.buf", O_RDONLY);
                while((buflen = read(fd, buf, 1024)) > 0)
                {
                write(1, buf, buflen);
                }
                close(fd);
                remove("stdout.buf");
                remove("stderr.buf");
            */
            shell_call_in_progress = 0;
            goto shell_complete;
        }

        static STARTUPINFO si;
        static PROCESS_INFORMATION pi;

        if (cmd_ok()) {

            static SHELLEXECUTEINFO shi;
            static char cmd[10] = "cmd\0";

            // attempt to separate file name (if any) from parameters
            static int32_t x, quotes;

            qbs_set(str1, str);
            qbs_set(str2, qbs_new_txt(""));
            if (!str1->len)
                goto shell_complete; // failed!

            if (!cmd_command(str1)) {
                // try directly, as is
                qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
                ZeroMemory(&shi, sizeof(shi));
                shi.cbSize = sizeof(shi);
                shi.lpFile = (char *)&str1z->chr[0];
                shi.lpParameters = NULL;
                shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
                shi.nShow = SW_SHOW;
                if (ShellExecuteEx(&shi)) {
                    shell_call_in_progress = 1;
                    // Wait until child process exits.
                    WaitForSingleObject(shi.hProcess, INFINITE);
                    GetExitCodeProcess(shi.hProcess, (DWORD *)&return_code);
                    CloseHandle(shi.hProcess);
                    shell_call_in_progress = 0;
                    goto shell_complete;
                }
            }

            x = 0;
            quotes = 0;
            while (x < str1->len) {
                if (str1->chr[x] == 34) {
                    if (!quotes)
                        quotes = 1;
                    else
                        quotes = 0;
                }
                if (str1->chr[x] == 32) {
                    if (quotes == 0) {
                        qbs_set(str2, qbs_right(str1, str1->len - x - 1));
                        qbs_set(str1, qbs_left(str1, x));
                        goto split;
                    }
                }
                x++;
            }
        split:
            if (!str1->len)
                goto shell_complete; // failed!

            if (str2->len) {
                if (!cmd_command(str1)) {
                    qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
                    qbs_set(str2z, qbs_add(str2, qbs_new_txt_len("\0", 1)));
                    ZeroMemory(&shi, sizeof(shi));
                    shi.cbSize = sizeof(shi);
                    shi.lpFile = (char *)&str1z->chr[0];
                    shi.lpParameters = (char *)&str2z->chr[0];
                    // if (str2->len<=1) shi.lpParameters=NULL;
                    shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
                    shi.nShow = SW_SHOW;
                    if (ShellExecuteEx(&shi)) {
                        shell_call_in_progress = 1;
                        // Wait until child process exits.
                        WaitForSingleObject(shi.hProcess, INFINITE);
                        GetExitCodeProcess(shi.hProcess, (DWORD *)&return_code);
                        CloseHandle(shi.hProcess);
                        shell_call_in_progress = 0;
                        goto shell_complete;
                    }
                }
            }

            // failed, try cmd /c method...
            if (str2->len)
                qbs_set(str2, qbs_add(qbs_new_txt(" "), str2));
            qbs_set(strz, qbs_add(str1, str2));
            qbs_set(strz, qbs_add(qbs_new_txt(" /c "), strz));
            qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
            ZeroMemory(&shi, sizeof(shi));
            shi.cbSize = sizeof(shi);
            shi.lpFile = &cmd[0];
            shi.lpParameters = (char *)&strz->chr[0];
            shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
            shi.nShow = SW_SHOW;
            if (ShellExecuteEx(&shi)) {
                shell_call_in_progress = 1;
                // Wait until child process exits.
                WaitForSingleObject(shi.hProcess, INFINITE);
                GetExitCodeProcess(shi.hProcess, (DWORD *)&return_code);
                CloseHandle(shi.hProcess);
                shell_call_in_progress = 0;
                goto shell_complete;
            }

            /*
                qbs_set(strz,qbs_add(qbs_new_txt("cmd.exe /c "),str));
                qbs_set(strz,qbs_add(strz,qbs_new_txt_len("\0",1)));
                ZeroMemory( &si, sizeof(si) ); si.cb = sizeof(si); ZeroMemory( &pi, sizeof(pi) );
                if(CreateProcess(
                NULL,           // No module name (use command line)
                (char*)&strz->chr[0], // Command line
                NULL,           // Process handle not inheritable
                NULL,           // Thread handle not inheritable
                FALSE,          // Set handle inheritance to FALSE
                DETACHED_PROCESS, // No creation flags
                NULL,           // Use parent's environment block
                NULL,           // Use parent's starting directory
                &si,            // Pointer to STARTUPINFO structure
                &pi )           // Pointer to PROCESS_INFORMATION structure
                ){
                shell_call_in_progress=1;
                // Wait until child process exits.
                WaitForSingleObject( pi.hProcess, INFINITE );
                // Close process and thread handles.
                CloseHandle( pi.hProcess );
                CloseHandle( pi.hThread );
                shell_call_in_progress=0;
                goto shell_complete;
                }
            */

            return_code = 1;
            goto shell_complete; // failed

        } else {

            qbs_set(strz, qbs_add(qbs_new_txt("command.com /c "), str));
            qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));
            if (CreateProcess(NULL,                  // No module name (use command line)
                              (char *)&strz->chr[0], // Command line
                              NULL,                  // Process handle not inheritable
                              NULL,                  // Thread handle not inheritable
                              FALSE,                 // Set handle inheritance to FALSE
                              CREATE_NEW_CONSOLE,    // No creation flags
                              NULL,                  // Use parent's environment block
                              NULL,                  // Use parent's starting directory
                              &si,                   // Pointer to STARTUPINFO structure
                              &pi)                   // Pointer to PROCESS_INFORMATION structure
            ) {
                shell_call_in_progress = 1;
                // Wait until child process exits.
                WaitForSingleObject(pi.hProcess, INFINITE);
                // Close process and thread handles.
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                shell_call_in_progress = 0;
                goto shell_complete;
            }
            goto shell_complete; // failed

        } // cmd_ok()

#else

        qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
        shell_call_in_progress = 1;
        return_code = system((char *)strz->chr);
        shell_call_in_progress = 0;
        if (return_code == -1) { /* do nothing */
        } else {
            return_code = WEXITSTATUS(return_code);
        }

#endif

    } else {

// SHELL (with no parameters)
// note: opening a new shell is only available in windows atm via cmd
// note: assumes cmd available
#ifdef QB64_WINDOWS
        if (!use_console)
            AllocConsole();
        qbs_set(strz, qbs_new_txt_len("cmd\0", 4));
        shell_call_in_progress = 1;
        return_code = system((char *)strz->chr);
        shell_call_in_progress = 0;
        if (!use_console)
            FreeConsole();
        goto shell_complete;
#endif
    }

shell_complete:
    // reenter full screen mode if necessary
    if (full_screen_mode) {
        full_screen_set = full_screen_mode;
        do {
            Sleep(0);
        } while (!full_screen);
    } // full_screen_mode

    return return_code;
} // func SHELL(...

int64_t func__shellhide(qbs *str) { // func _SHELLHIDE(...
    if (is_error_pending())
        return 1;

    static int64_t return_code;
    return_code = 0;

    static qbs *strz = NULL;
    static int32_t i;
    if (!strz)
        strz = qbs_new(0, 0);
    if (!str->len) {
        error(5);
        return 1;
    } // cannot launch a hidden console

    static qbs *str1 = NULL;
    static qbs *str2 = NULL;
    static qbs *str1z = NULL;
    static qbs *str2z = NULL;
    if (!str1)
        str1 = qbs_new(0, 0);
    if (!str2)
        str2 = qbs_new(0, 0);
    if (!str1z)
        str1z = qbs_new(0, 0);
    if (!str2z)
        str2z = qbs_new(0, 0);

#ifdef QB64_WINDOWS

    static STARTUPINFO si;
    static PROCESS_INFORMATION pi;

    if (cmd_ok()) {

        static SHELLEXECUTEINFO shi;
        static char cmd[10] = "cmd\0";

        // attempt to separate file name (if any) from parameters
        static int32_t x, quotes;

        qbs_set(str1, str);
        qbs_set(str2, qbs_new_txt(""));

        if (!cmd_command(str1)) {
            // try directly, as is
            qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
            ZeroMemory(&shi, sizeof(shi));
            shi.cbSize = sizeof(shi);
            shi.lpFile = (char *)&str1z->chr[0];
            shi.lpParameters = NULL;
            shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
            shi.nShow = SW_HIDE;
            if (ShellExecuteEx(&shi)) {
                shell_call_in_progress = 1;
                // Wait until child process exits.
                WaitForSingleObject(shi.hProcess, INFINITE);
                GetExitCodeProcess(shi.hProcess, (DWORD *)&return_code);
                CloseHandle(shi.hProcess);
                shell_call_in_progress = 0;
                goto shell_complete;
            }
        }

        x = 0;
        quotes = 0;
        while (x < str1->len) {
            if (str1->chr[x] == 34) {
                if (!quotes)
                    quotes = 1;
                else
                    quotes = 0;
            }
            if (str1->chr[x] == 32) {
                if (quotes == 0) {
                    qbs_set(str2, qbs_right(str1, str1->len - x - 1));
                    qbs_set(str1, qbs_left(str1, x));
                    goto split;
                }
            }
            x++;
        }
    split:
        if (!str1->len) {
            return_code = 1;
            goto shell_complete;
        } // failed!

        if (str2->len) {
            if (!cmd_command(str1)) {
                qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
                qbs_set(str2z, qbs_add(str2, qbs_new_txt_len("\0", 1)));
                ZeroMemory(&shi, sizeof(shi));
                shi.cbSize = sizeof(shi);
                shi.lpFile = (char *)&str1z->chr[0];
                shi.lpParameters = (char *)&str2z->chr[0];
                // if (str2->len<=1) shi.lpParameters=NULL;
                shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
                shi.nShow = SW_HIDE;
                if (ShellExecuteEx(&shi)) {
                    shell_call_in_progress = 1;
                    // Wait until child process exits.
                    WaitForSingleObject(shi.hProcess, INFINITE);
                    GetExitCodeProcess(shi.hProcess, (DWORD *)&return_code);
                    CloseHandle(shi.hProcess);
                    shell_call_in_progress = 0;
                    goto shell_complete;
                }
            }
        }

        // failed, try cmd /c method...
        if (str2->len)
            qbs_set(str2, qbs_add(qbs_new_txt(" "), str2));
        qbs_set(strz, qbs_add(str1, str2));
        qbs_set(strz, qbs_add(qbs_new_txt(" /c "), strz));
        qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
        ZeroMemory(&shi, sizeof(shi));
        shi.cbSize = sizeof(shi);
        shi.lpFile = &cmd[0];
        shi.lpParameters = (char *)&strz->chr[0];
        shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        shi.nShow = SW_HIDE;
        if (ShellExecuteEx(&shi)) {
            shell_call_in_progress = 1;
            // Wait until child process exits.
            WaitForSingleObject(shi.hProcess, INFINITE);
            GetExitCodeProcess(shi.hProcess, (DWORD *)&return_code);
            CloseHandle(shi.hProcess);
            shell_call_in_progress = 0;
            goto shell_complete;
        }

        /*
            qbs_set(strz,qbs_add(qbs_new_txt("cmd.exe /c "),str));
            qbs_set(strz,qbs_add(strz,qbs_new_txt_len("\0",1)));
            ZeroMemory( &si, sizeof(si) ); si.cb = sizeof(si); ZeroMemory( &pi, sizeof(pi) );
            if(CreateProcess(
            NULL,           // No module name (use command line)
            (char*)&strz->chr[0], // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            CREATE_NO_WINDOW, // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory
            &si,            // Pointer to STARTUPINFO structure
            &pi )           // Pointer to PROCESS_INFORMATION structure
            ){
            shell_call_in_progress=1;
            // Wait until child process exits.
            WaitForSingleObject( pi.hProcess, INFINITE );
            // Close process and thread handles.
            CloseHandle( pi.hProcess );
            CloseHandle( pi.hThread );
            shell_call_in_progress=0;
            goto shell_complete;
            }
        */

        return_code = 1;
        goto shell_complete; // failed

    } else {

        qbs_set(strz, qbs_add(qbs_new_txt("command.com /c "), str));
        qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        if (CreateProcess(

                NULL,                  // No module name (use command line)
                (char *)&strz->chr[0], // Command line
                NULL,                  // Process handle not inheritable
                NULL,                  // Thread handle not inheritable
                FALSE,                 // Set handle inheritance to FALSE
                CREATE_NEW_CONSOLE,    // note: cannot hide new console, but can preserve existing one
                NULL,                  // Use parent's environment block
                NULL,                  // Use parent's starting directory
                &si,                   // Pointer to STARTUPINFO structure
                &pi)                   // Pointer to PROCESS_INFORMATION structure
        ) {
            shell_call_in_progress = 1;
            // Wait until child process exits.
            WaitForSingleObject(pi.hProcess, INFINITE);
            // Close process and thread handles.
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            shell_call_in_progress = 0;
            goto shell_complete;
        }
        goto shell_complete; // failed

    } // cmd_ok()

#else

    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
    shell_call_in_progress = 1;
    return_code = system((char *)strz->chr);
    shell_call_in_progress = 0;
    return return_code;

#endif

shell_complete:;

    return return_code;
} // func _SHELLHIDE(...

void sub_shell(qbs *str, int32_t passed) {
    if (is_error_pending())
        return;

    // exit full screen mode if necessary
    static int32_t full_screen_mode;
    full_screen_mode = full_screen;
    if (full_screen_mode) {
        full_screen_set = 0;
        do {
            Sleep(0);
        } while (full_screen);
    } // full_screen_mode
    static qbs *strz = NULL;
    static qbs *str1 = NULL;
    static qbs *str1z = NULL;
    static qbs *str2 = NULL;
    static qbs *str2z = NULL;
    static int32_t i;

    static int32_t use_console;
    use_console = 0;
    if (console) {
        if (console_active) {
            use_console = 1;
        }
    }

    if (!strz)
        strz = qbs_new(0, 0);
    if (!str1)
        str1 = qbs_new(0, 0);
    if (!str1z)
        str1z = qbs_new(0, 0);
    if (!str2)
        str2 = qbs_new(0, 0);
    if (!str2z)
        str2z = qbs_new(0, 0);

    if (passed)
        if (str->len == 0)
            passed = 0; //"" means launch a CONSOLE
    if (passed) {

#ifdef QB64_WINDOWS

        if (use_console) {
            qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
            shell_call_in_progress = 1;
            /*
                freopen("stdout.buf", "w", stdout);
                freopen("stderr.buf", "w", stderr);
            */
            system((char *)strz->chr);
            /*
                freopen("CON", "w", stdout);
                freopen("CON", "w", stderr);
                static char buf[1024];
                static int buflen;
                static int fd;
                fd = open("stdout.buf", O_RDONLY);
                while((buflen = read(fd, buf, 1024)) > 0)
                {
                write(1, buf, buflen);
                }
                close(fd);
                fd = open("stderr.buf", O_RDONLY);
                while((buflen = read(fd, buf, 1024)) > 0)
                {
                write(1, buf, buflen);
                }
                close(fd);
                remove("stdout.buf");
                remove("stderr.buf");
            */
            shell_call_in_progress = 0;
            goto shell_complete;
        }

        static STARTUPINFO si;
        static PROCESS_INFORMATION pi;

        if (cmd_ok()) {

            static SHELLEXECUTEINFO shi;
            static char cmd[10] = "cmd\0";

            // attempt to separate file name (if any) from parameters
            static int32_t x, quotes;

            qbs_set(str1, str);
            qbs_set(str2, qbs_new_txt(""));
            if (!str1->len)
                goto shell_complete; // failed!

            if (!cmd_command(str1)) {
                // try directly, as is
                qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
                ZeroMemory(&shi, sizeof(shi));
                shi.cbSize = sizeof(shi);
                shi.lpFile = (char *)&str1z->chr[0];
                shi.lpParameters = NULL;
                shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
                shi.nShow = SW_SHOW;
                if (ShellExecuteEx(&shi)) {
                    shell_call_in_progress = 1;
                    // Wait until child process exits.
                    WaitForSingleObject(shi.hProcess, INFINITE);
                    CloseHandle(shi.hProcess);
                    shell_call_in_progress = 0;
                    goto shell_complete;
                }
            }

            x = 0;
            quotes = 0;
            while (x < str1->len) {
                if (str1->chr[x] == 34) {
                    if (!quotes)
                        quotes = 1;
                    else
                        quotes = 0;
                }
                if (str1->chr[x] == 32) {
                    if (quotes == 0) {
                        qbs_set(str2, qbs_right(str1, str1->len - x - 1));
                        qbs_set(str1, qbs_left(str1, x));
                        goto split;
                    }
                }
                x++;
            }
        split:
            if (!str1->len)
                goto shell_complete; // failed!

            if (str2->len) {
                if (!cmd_command(str1)) {
                    qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
                    qbs_set(str2z, qbs_add(str2, qbs_new_txt_len("\0", 1)));
                    ZeroMemory(&shi, sizeof(shi));
                    shi.cbSize = sizeof(shi);
                    shi.lpFile = (char *)&str1z->chr[0];
                    shi.lpParameters = (char *)&str2z->chr[0];
                    // if (str2->len<=1) shi.lpParameters=NULL;
                    shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
                    shi.nShow = SW_SHOW;
                    if (ShellExecuteEx(&shi)) {
                        shell_call_in_progress = 1;
                        // Wait until child process exits.
                        WaitForSingleObject(shi.hProcess, INFINITE);
                        CloseHandle(shi.hProcess);
                        shell_call_in_progress = 0;
                        goto shell_complete;
                    }
                }
            }

            // failed, try cmd /c method...
            if (str2->len)
                qbs_set(str2, qbs_add(qbs_new_txt(" "), str2));
            qbs_set(strz, qbs_add(str1, str2));
            qbs_set(strz, qbs_add(qbs_new_txt(" /c "), strz));
            qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
            ZeroMemory(&shi, sizeof(shi));
            shi.cbSize = sizeof(shi);
            shi.lpFile = &cmd[0];
            shi.lpParameters = (char *)&strz->chr[0];
            shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
            shi.nShow = SW_SHOW;
            if (ShellExecuteEx(&shi)) {
                shell_call_in_progress = 1;
                // Wait until child process exits.
                WaitForSingleObject(shi.hProcess, INFINITE);
                CloseHandle(shi.hProcess);
                shell_call_in_progress = 0;
                goto shell_complete;
            }

            /*
                qbs_set(strz,qbs_add(qbs_new_txt("cmd.exe /c "),str));
                qbs_set(strz,qbs_add(strz,qbs_new_txt_len("\0",1)));
                ZeroMemory( &si, sizeof(si) ); si.cb = sizeof(si); ZeroMemory( &pi, sizeof(pi) );
                if(CreateProcess(
                NULL,           // No module name (use command line)
                (char*)&strz->chr[0], // Command line
                NULL,           // Process handle not inheritable
                NULL,           // Thread handle not inheritable
                FALSE,          // Set handle inheritance to FALSE
                DETACHED_PROCESS, // No creation flags
                NULL,           // Use parent's environment block
                NULL,           // Use parent's starting directory
                &si,            // Pointer to STARTUPINFO structure
                &pi )           // Pointer to PROCESS_INFORMATION structure
                ){
                shell_call_in_progress=1;
                // Wait until child process exits.
                WaitForSingleObject( pi.hProcess, INFINITE );
                // Close process and thread handles.
                CloseHandle( pi.hProcess );
                CloseHandle( pi.hThread );
                shell_call_in_progress=0;
                goto shell_complete;
                }
            */

            goto shell_complete; // failed

        } else {

            qbs_set(strz, qbs_add(qbs_new_txt("command.com /c "), str));
            qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));
            if (CreateProcess(NULL,                  // No module name (use command line)
                              (char *)&strz->chr[0], // Command line
                              NULL,                  // Process handle not inheritable
                              NULL,                  // Thread handle not inheritable
                              FALSE,                 // Set handle inheritance to FALSE
                              CREATE_NEW_CONSOLE,    // No creation flags
                              NULL,                  // Use parent's environment block
                              NULL,                  // Use parent's starting directory
                              &si,                   // Pointer to STARTUPINFO structure
                              &pi)                   // Pointer to PROCESS_INFORMATION structure
            ) {
                shell_call_in_progress = 1;
                // Wait until child process exits.
                WaitForSingleObject(pi.hProcess, INFINITE);
                // Close process and thread handles.
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                shell_call_in_progress = 0;
                goto shell_complete;
            }
            goto shell_complete; // failed

        } // cmd_ok()

#else

        qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
        shell_call_in_progress = 1;
        system((char *)strz->chr);
        shell_call_in_progress = 0;

#endif

    } else {

// SHELL (with no parameters)
// note: opening a new shell is only available in windows atm via cmd
// note: assumes cmd available
#ifdef QB64_WINDOWS
        if (!use_console)
            AllocConsole();
        qbs_set(strz, qbs_new_txt_len("cmd\0", 4));
        shell_call_in_progress = 1;
        system((char *)strz->chr);
        shell_call_in_progress = 0;
        if (!use_console)
            FreeConsole();
        goto shell_complete;
#endif
    }

shell_complete:
    // reenter full screen mode if necessary
    if (full_screen_mode) {
        full_screen_set = full_screen_mode;
        do {
            Sleep(0);
        } while (!full_screen);
    } // full_screen_mode
}

void sub_shell2(qbs *str, int32_t passed) { // HIDE
    if (is_error_pending())
        return;

    if (passed & 1) {
        sub_shell4(str, passed & 2);
        return;
    }
    if (!(passed & 2)) {
        error(5);
        return;
    } // should not hide a shell waiting for input

    static qbs *strz = NULL;
    static int32_t i;
    if (!strz)
        strz = qbs_new(0, 0);
    if (!str->len) {
        error(5);
        return;
    } // cannot launch a hidden console

    static qbs *str1 = NULL;
    static qbs *str2 = NULL;
    static qbs *str1z = NULL;
    static qbs *str2z = NULL;
    if (!str1)
        str1 = qbs_new(0, 0);
    if (!str2)
        str2 = qbs_new(0, 0);
    if (!str1z)
        str1z = qbs_new(0, 0);
    if (!str2z)
        str2z = qbs_new(0, 0);

#ifdef QB64_WINDOWS

    static STARTUPINFO si;
    static PROCESS_INFORMATION pi;

    if (cmd_ok()) {

        static SHELLEXECUTEINFO shi;
        static char cmd[10] = "cmd\0";

        // attempt to separate file name (if any) from parameters
        static int32_t x, quotes;

        qbs_set(str1, str);
        qbs_set(str2, qbs_new_txt(""));

        if (!cmd_command(str1)) {
            // try directly, as is
            qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
            ZeroMemory(&shi, sizeof(shi));
            shi.cbSize = sizeof(shi);
            shi.lpFile = (char *)&str1z->chr[0];
            shi.lpParameters = NULL;
            shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
            shi.nShow = SW_HIDE;
            if (ShellExecuteEx(&shi)) {
                shell_call_in_progress = 1;
                // Wait until child process exits.
                WaitForSingleObject(shi.hProcess, INFINITE);
                CloseHandle(shi.hProcess);
                shell_call_in_progress = 0;
                goto shell_complete;
            }
        }

        x = 0;
        quotes = 0;
        while (x < str1->len) {
            if (str1->chr[x] == 34) {
                if (!quotes)
                    quotes = 1;
                else
                    quotes = 0;
            }
            if (str1->chr[x] == 32) {
                if (quotes == 0) {
                    qbs_set(str2, qbs_right(str1, str1->len - x - 1));
                    qbs_set(str1, qbs_left(str1, x));
                    goto split;
                }
            }
            x++;
        }
    split:
        if (!str1->len)
            goto shell_complete; // failed!

        if (str2->len) {
            if (!cmd_command(str1)) {
                qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
                qbs_set(str2z, qbs_add(str2, qbs_new_txt_len("\0", 1)));
                ZeroMemory(&shi, sizeof(shi));
                shi.cbSize = sizeof(shi);
                shi.lpFile = (char *)&str1z->chr[0];
                shi.lpParameters = (char *)&str2z->chr[0];
                // if (str2->len<=1) shi.lpParameters=NULL;
                shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
                shi.nShow = SW_HIDE;
                if (ShellExecuteEx(&shi)) {
                    shell_call_in_progress = 1;
                    // Wait until child process exits.
                    WaitForSingleObject(shi.hProcess, INFINITE);
                    CloseHandle(shi.hProcess);
                    shell_call_in_progress = 0;
                    goto shell_complete;
                }
            }
        }

        // failed, try cmd /c method...
        if (str2->len)
            qbs_set(str2, qbs_add(qbs_new_txt(" "), str2));
        qbs_set(strz, qbs_add(str1, str2));
        qbs_set(strz, qbs_add(qbs_new_txt(" /c "), strz));
        qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
        ZeroMemory(&shi, sizeof(shi));
        shi.cbSize = sizeof(shi);
        shi.lpFile = &cmd[0];
        shi.lpParameters = (char *)&strz->chr[0];
        shi.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        shi.nShow = SW_HIDE;
        if (ShellExecuteEx(&shi)) {
            shell_call_in_progress = 1;
            // Wait until child process exits.
            WaitForSingleObject(shi.hProcess, INFINITE);
            CloseHandle(shi.hProcess);
            shell_call_in_progress = 0;
            goto shell_complete;
        }

        /*
            qbs_set(strz,qbs_add(qbs_new_txt("cmd.exe /c "),str));
            qbs_set(strz,qbs_add(strz,qbs_new_txt_len("\0",1)));
            ZeroMemory( &si, sizeof(si) ); si.cb = sizeof(si); ZeroMemory( &pi, sizeof(pi) );
            if(CreateProcess(
            NULL,           // No module name (use command line)
            (char*)&strz->chr[0], // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            CREATE_NO_WINDOW, // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory
            &si,            // Pointer to STARTUPINFO structure
            &pi )           // Pointer to PROCESS_INFORMATION structure
            ){
            shell_call_in_progress=1;
            // Wait until child process exits.
            WaitForSingleObject( pi.hProcess, INFINITE );
            // Close process and thread handles.
            CloseHandle( pi.hProcess );
            CloseHandle( pi.hThread );
            shell_call_in_progress=0;
            goto shell_complete;
            }
        */

        goto shell_complete; // failed

    } else {

        qbs_set(strz, qbs_add(qbs_new_txt("command.com /c "), str));
        qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        if (CreateProcess(

                NULL,                  // No module name (use command line)
                (char *)&strz->chr[0], // Command line
                NULL,                  // Process handle not inheritable
                NULL,                  // Thread handle not inheritable
                FALSE,                 // Set handle inheritance to FALSE
                CREATE_NEW_CONSOLE,    // note: cannot hide new console, but can preserve existing one
                NULL,                  // Use parent's environment block
                NULL,                  // Use parent's starting directory
                &si,                   // Pointer to STARTUPINFO structure
                &pi)                   // Pointer to PROCESS_INFORMATION structure
        ) {
            shell_call_in_progress = 1;
            // Wait until child process exits.
            WaitForSingleObject(pi.hProcess, INFINITE);
            // Close process and thread handles.
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            shell_call_in_progress = 0;
            goto shell_complete;
        }
        goto shell_complete; // failed

    } // cmd_ok()

#else

    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
    shell_call_in_progress = 1;
    system((char *)strz->chr);
    shell_call_in_progress = 0;
    return;

#endif

shell_complete:;
}

void sub_shell3(qbs *str, int32_t passed) { //_DONTWAIT
    // shell3 launches 'str' but does not wait for it to complete
    if (is_error_pending())
        return;

    if (passed & 1) {
        sub_shell4(str, passed & 2);
        return;
    }

    static qbs *strz = NULL;
    static int32_t i;

    static qbs *str1 = NULL;
    static qbs *str2 = NULL;
    static qbs *str1z = NULL;
    static qbs *str2z = NULL;
    if (!str1)
        str1 = qbs_new(0, 0);
    if (!str2)
        str2 = qbs_new(0, 0);
    if (!str1z)
        str1z = qbs_new(0, 0);
    if (!str2z)
        str2z = qbs_new(0, 0);

    if (!strz)
        strz = qbs_new(0, 0);

#ifdef QB64_WINDOWS

    static STARTUPINFO si;
    static PROCESS_INFORMATION pi;

    if (cmd_ok()) {

        static SHELLEXECUTEINFO shi;
        static char cmd[10] = "cmd\0";

        // attempt to separate file name (if any) from parameters
        static int32_t x, quotes;

        // allow for launching of a console
        if (!(passed & 2)) {
            qbs_set(str1, qbs_new_txt("cmd"));
        } else {
            qbs_set(str1, str);
            if (!str1->len)
                qbs_set(str1, qbs_new_txt("cmd"));
        }
        qbs_set(str2, qbs_new_txt(""));

        if (!cmd_command(str1)) {
            // try directly, as is
            qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
            ZeroMemory(&shi, sizeof(shi));
            shi.cbSize = sizeof(shi);
            shi.lpFile = (char *)&str1z->chr[0];
            shi.lpParameters = NULL;
            shi.fMask = SEE_MASK_FLAG_NO_UI;
            shi.nShow = SW_SHOW;
            if (ShellExecuteEx(&shi)) {
                goto shell_complete;
            }
        }

        x = 0;
        quotes = 0;
        while (x < str1->len) {
            if (str1->chr[x] == 34) {
                if (!quotes)
                    quotes = 1;
                else
                    quotes = 0;
            }
            if (str1->chr[x] == 32) {
                if (quotes == 0) {
                    qbs_set(str2, qbs_right(str1, str1->len - x - 1));
                    qbs_set(str1, qbs_left(str1, x));
                    goto split;
                }
            }
            x++;
        }
    split:
        if (!str1->len)
            goto shell_complete; // failed!

        if (str2->len) {
            if (!cmd_command(str1)) {
                qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
                qbs_set(str2z, qbs_add(str2, qbs_new_txt_len("\0", 1)));
                ZeroMemory(&shi, sizeof(shi));
                shi.cbSize = sizeof(shi);
                shi.lpFile = (char *)&str1z->chr[0];
                shi.lpParameters = (char *)&str2z->chr[0];
                // if (str2->len<=1) shi.lpParameters=NULL;
                shi.fMask = SEE_MASK_FLAG_NO_UI;
                shi.nShow = SW_SHOW;
                if (ShellExecuteEx(&shi)) {
                    goto shell_complete;
                }
            }
        }

        // failed, try cmd /c method...
        if (str2->len)
            qbs_set(str2, qbs_add(qbs_new_txt(" "), str2));
        qbs_set(strz, qbs_add(str1, str2));
        qbs_set(strz, qbs_add(qbs_new_txt(" /c "), strz));
        qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
        ZeroMemory(&shi, sizeof(shi));
        shi.cbSize = sizeof(shi);
        shi.lpFile = &cmd[0];
        shi.lpParameters = (char *)&strz->chr[0];
        shi.fMask = SEE_MASK_FLAG_NO_UI;
        shi.nShow = SW_SHOW;
        if (ShellExecuteEx(&shi)) {
            goto shell_complete;
        }

        /*
            qbs_set(strz,qbs_add(qbs_new_txt("cmd.exe /c "),str));
            qbs_set(strz,qbs_add(strz,qbs_new_txt_len("\0",1)));
            ZeroMemory( &si, sizeof(si) ); si.cb = sizeof(si); ZeroMemory( &pi, sizeof(pi) );
            if(CreateProcess(
            NULL,           // No module name (use command line)
            (char*)&strz->chr[0], // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            DETACHED_PROCESS, // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory
            &si,            // Pointer to STARTUPINFO structure
            &pi )           // Pointer to PROCESS_INFORMATION structure
            ){
            //ref: The created process remains in the system until all threads within the process have terminated and all handles to the process and any of its
           threads have been closed through calls to CloseHandle. The handles for both the process and the main thread must be closed through calls to
           CloseHandle. If these handles are not needed, it is best to close them immediately after the process is created. CloseHandle( pi.hProcess );
            CloseHandle( pi.hThread );
            goto shell_complete;
            }
        */

        goto shell_complete; // failed

    } else {

        qbs_set(strz, qbs_add(qbs_new_txt("command.com /c "), str));
        qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        if (CreateProcess(NULL,                  // No module name (use command line)
                          (char *)&strz->chr[0], // Command line
                          NULL,                  // Process handle not inheritable
                          NULL,                  // Thread handle not inheritable
                          FALSE,                 // Set handle inheritance to FALSE
                          CREATE_NEW_CONSOLE,    // note: cannot hide new console, but can preserve existing one
                          NULL,                  // Use parent's environment block
                          NULL,                  // Use parent's starting directory
                          &si,                   // Pointer to STARTUPINFO structure
                          &pi)                   // Pointer to PROCESS_INFORMATION structure
        ) {
            // ref: The created process remains in the system until all threads within the process have terminated and all handles to the process and any of its
            // threads have been closed through calls to CloseHandle. The handles for both the process and the main thread must be closed through calls to
            // CloseHandle. If these handles are not needed, it is best to close them immediately after the process is created.
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            goto shell_complete;
        }
        goto shell_complete; // failed

    } // cmd_ok()

#else

    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
    pid_t pid = fork();
    if (pid == 0)
        _exit(system((char *)strz->chr));
    return;

#endif

shell_complete:;
} // SHELL _DONTWAIT

void sub_shell4(qbs *str, int32_t passed) { //_DONTWAIT & _HIDE
    // if passed&2 set a string was given
    if (!(passed & 2)) {
        error(5);
        return;
    } // should not hide a shell waiting for input

    static qbs *strz = NULL;
    static int32_t i;

    static qbs *str1 = NULL;
    static qbs *str2 = NULL;
    static qbs *str1z = NULL;
    static qbs *str2z = NULL;
    if (!str1)
        str1 = qbs_new(0, 0);
    if (!str2)
        str2 = qbs_new(0, 0);
    if (!str1z)
        str1z = qbs_new(0, 0);
    if (!str2z)
        str2z = qbs_new(0, 0);

    if (!strz)
        strz = qbs_new(0, 0);

    if (!str->len) {
        error(5);
        return;
    } // console unsupported

#ifdef QB64_WINDOWS

    static STARTUPINFO si;
    static PROCESS_INFORMATION pi;

    if (cmd_ok()) {

        static SHELLEXECUTEINFO shi;
        static char cmd[10] = "cmd\0";

        // attempt to separate file name (if any) from parameters
        static int32_t x, quotes;

        qbs_set(str1, str);
        qbs_set(str2, qbs_new_txt(""));

        if (!cmd_command(str1)) {
            // try directly, as is
            qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
            ZeroMemory(&shi, sizeof(shi));
            shi.cbSize = sizeof(shi);
            shi.lpFile = (char *)&str1z->chr[0];
            shi.lpParameters = NULL;
            shi.fMask = SEE_MASK_FLAG_NO_UI;
            shi.nShow = SW_HIDE;
            if (ShellExecuteEx(&shi)) {
                goto shell_complete;
            }
        }

        x = 0;
        quotes = 0;
        while (x < str1->len) {
            if (str1->chr[x] == 34) {
                if (!quotes)
                    quotes = 1;
                else
                    quotes = 0;
            }
            if (str1->chr[x] == 32) {
                if (quotes == 0) {
                    qbs_set(str2, qbs_right(str1, str1->len - x - 1));
                    qbs_set(str1, qbs_left(str1, x));
                    goto split;
                }
            }
            x++;
        }
    split:
        if (!str1->len)
            goto shell_complete; // failed!

        if (str2->len) {
            if (!cmd_command(str1)) {
                qbs_set(str1z, qbs_add(str1, qbs_new_txt_len("\0", 1)));
                qbs_set(str2z, qbs_add(str2, qbs_new_txt_len("\0", 1)));
                ZeroMemory(&shi, sizeof(shi));
                shi.cbSize = sizeof(shi);
                shi.lpFile = (char *)&str1z->chr[0];
                shi.lpParameters = (char *)&str2z->chr[0];
                // if (str2->len<=1) shi.lpParameters=NULL;
                shi.fMask = SEE_MASK_FLAG_NO_UI;
                shi.nShow = SW_HIDE;
                if (ShellExecuteEx(&shi)) {
                    goto shell_complete;
                }
            }
        }

        // failed, try cmd /c method...
        if (str2->len)
            qbs_set(str2, qbs_add(qbs_new_txt(" "), str2));
        qbs_set(strz, qbs_add(str1, str2));
        qbs_set(strz, qbs_add(qbs_new_txt(" /c "), strz));
        qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
        ZeroMemory(&shi, sizeof(shi));
        shi.cbSize = sizeof(shi);
        shi.lpFile = &cmd[0];
        shi.lpParameters = (char *)&strz->chr[0];
        shi.fMask = SEE_MASK_FLAG_NO_UI;
        shi.nShow = SW_HIDE;
        if (ShellExecuteEx(&shi)) {
            goto shell_complete;
        }

        /*
            qbs_set(strz,qbs_add(qbs_new_txt("cmd.exe /c "),str));
            qbs_set(strz,qbs_add(strz,qbs_new_txt_len("\0",1)));
            ZeroMemory( &si, sizeof(si) ); si.cb = sizeof(si); ZeroMemory( &pi, sizeof(pi) );
            if(CreateProcess(
            NULL,           // No module name (use command line)
            (char*)&strz->chr[0], // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            DETACHED_PROCESS, // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory
            &si,            // Pointer to STARTUPINFO structure
            &pi )           // Pointer to PROCESS_INFORMATION structure
            ){
            //ref: The created process remains in the system until all threads within the process have terminated and all handles to the process and any of its
           threads have been closed through calls to CloseHandle. The handles for both the process and the main thread must be closed through calls to
           CloseHandle. If these handles are not needed, it is best to close them immediately after the process is created. CloseHandle( pi.hProcess );
            CloseHandle( pi.hThread );
            goto shell_complete;
            }
        */

        goto shell_complete; // failed

    } else {

        qbs_set(strz, qbs_add(qbs_new_txt("command.com /c "), str));
        qbs_set(strz, qbs_add(strz, qbs_new_txt_len("\0", 1)));
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        if (CreateProcess(NULL,                  // No module name (use command line)
                          (char *)&strz->chr[0], // Command line
                          NULL,                  // Process handle not inheritable
                          NULL,                  // Thread handle not inheritable
                          FALSE,                 // Set handle inheritance to FALSE
                          CREATE_NEW_CONSOLE,    // note: cannot hide new console, but can preserve existing one
                          NULL,                  // Use parent's environment block
                          NULL,                  // Use parent's starting directory
                          &si,                   // Pointer to STARTUPINFO structure
                          &pi)                   // Pointer to PROCESS_INFORMATION structure
        ) {
            // ref: The created process remains in the system until all threads within the process have terminated and all handles to the process and any of its
            // threads have been closed through calls to CloseHandle. The handles for both the process and the main thread must be closed through calls to
            // CloseHandle. If these handles are not needed, it is best to close them immediately after the process is created.
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            goto shell_complete;
        }
        goto shell_complete; // failed

    } // cmd_ok()

#else

    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
    pid_t pid = fork();
    if (pid == 0)
        _exit(system((char *)strz->chr));
    return;

#endif

shell_complete:;

} //_DONTWAIT & _HIDE
