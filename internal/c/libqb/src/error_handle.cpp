
#include "libqb-common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef QB64_WINDOWS
#include <windows.h>
#elif defined(QB64_LINUX) || defined(QB64_MACOSX)
#include <pthread.h>
#endif

#include "command.h"
#include "error_handle.h"
#include "logging.h"
#include "event.h"
#include "gui.h"
#include "glut-thread.h"

uint32_t new_error;
uint32_t error_occurred;
uint32_t error_retry;
uint32_t error_err; //=0;
uint32_t error_goto_line;
qbs *error_handler_history;
uint32_t error_handling;

static double error_erl; //=0;
static uint32_t ercl;
static uint32_t inclercl;

static const char *includedfilename;

// Tracks the BASIC source line before each generated statement executes when
// the whole-program $ErrorLocation:ON diagnostic mode is enabled. This is
// intentionally separate from ercl/inclercl, because fatal errors can occur
// before the normal qbevent/evnt() path updates the standard error-line fields.
static uint32_t current_ercl;
static uint32_t current_inclercl;
static const char *current_includedfilename;

// False only for executables produced by an older bootstrap compiler that
// emits no explicit error_track_line(...) calls. A newly generated program
// clears the tracker once at startup, and $ErrorLocation:ON then updates it
// before statements. The legacy event path must not repopulate it afterwards.
static bool explicit_line_tracking_seen;

// Returns the BASIC source position most recently emitted by
// $ErrorLocation:ON. Included-file coordinates take precedence so the
// location matches the existing _INCLERRORLINE/_INCLERRORFILE behavior.
static bool get_current_error_location(uint32_t &srcLine, const char *&srcFile) {
    if (current_inclercl) {
        srcLine = current_inclercl;
        srcFile = current_includedfilename ? current_includedfilename : "included file";
        return true;
    }

    if (current_ercl) {
        srcLine = current_ercl;
        srcFile = "main module";
        return true;
    }

    srcLine = 0;
    srcFile = NULL;
    return false;
}

// Formats fatal runtime errors without dynamic allocation. Static storage
// keeps the out-of-stack path from allocating another object while reporting
// the original failure.
static const char *critical_error_message(const char *message) {
    static char errmess[1024];
    uint32_t srcLine;
    const char *srcFile;

    if (get_current_error_location(srcLine, srcFile))
        snprintf(errmess, sizeof(errmess), "Line: %u (in %s)\n%s", srcLine, srcFile, message);
    else
        snprintf(errmess, sizeof(errmess), "%s\nEnable $ErrorLocation:ON for source location details.", message);

    return errmess;
}

// Returns true after reporting error 256 when the current native thread is
// close enough to its stack limit that another recursive BASIC call would be
// unsafe. Stack-bound discovery is platform-specific, but the policy and
// reporting path are shared by Windows, Linux and macOS.
bool error_check_stack() {
    // Keep enough stack available for error formatting, the native dialog and
    // the platform-specific shutdown path. The check is emitted for every user
    // SUB/FUNCTION; $ErrorLocation affects only source-position reporting.
    static constexpr uintptr_t STACK_REPORT_RESERVE = 256u * 1024u;
    static thread_local bool stack_bounds_checked;
    static thread_local bool stack_bounds_available;
    static thread_local uintptr_t stack_lower_bound;
    static thread_local bool stack_error_reported;

    uint8_t stack_marker;

    if (!stack_bounds_checked) {
#ifdef QB64_WINDOWS
        MEMORY_BASIC_INFORMATION stack_info;
        if (VirtualQuery(&stack_marker, &stack_info, sizeof(stack_info)) == sizeof(stack_info)) {
            // AllocationBase is the low address of the thread's reserved stack
            // region. Supported Windows targets use downward-growing stacks.
            stack_lower_bound = reinterpret_cast<uintptr_t>(stack_info.AllocationBase);
            stack_bounds_available = stack_lower_bound != 0;
        }
#elif defined(QB64_LINUX)
        pthread_attr_t stack_attributes;
        if (pthread_getattr_np(pthread_self(), &stack_attributes) == 0) {
            void *stack_address = NULL;
            size_t stack_size = 0;

            if (pthread_attr_getstack(&stack_attributes, &stack_address, &stack_size) == 0 && stack_address && stack_size) {
                stack_lower_bound = reinterpret_cast<uintptr_t>(stack_address);
                stack_bounds_available = true;
            }

            pthread_attr_destroy(&stack_attributes);
        }
#elif defined(QB64_MACOSX)
        const uintptr_t stack_upper_bound = reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np(pthread_self()));
        const size_t stack_size = pthread_get_stacksize_np(pthread_self());

        if (stack_upper_bound && stack_size) {
            stack_lower_bound = stack_upper_bound - stack_size;
            stack_bounds_available = true;
        }
#endif
        stack_bounds_checked = true;
    }

    const uintptr_t current_stack = reinterpret_cast<uintptr_t>(&stack_marker);
    if (!stack_error_reported && stack_bounds_available && current_stack > stack_lower_bound &&
        current_stack - stack_lower_bound < STACK_REPORT_RESERVE) {
        stack_error_reported = true;
        error(256);
        return true;
    }

    return stack_error_reported;
}

static const char *error_code_to_text(int32_t errorcode) {
    // clang-format off
    switch (errorcode) {
        case 0: return "No error";
        case 1: return "NEXT without FOR";
        case 2: return "Syntax error";
        case 3: return "RETURN without GOSUB";
        case 4: return "Out of DATA";
        case 5: return "Illegal function call";
        case 6: return "Overflow";
        case 7: return "Out of memory";
        case 8: return "Label not defined";
        case 9: return "Subscript out of range";
        case 10: return "Duplicate definition";
        case 12: return "Illegal in direct mode";
        case 13: return "Type mismatch";
        case 14: return "Out of string space";
        //error 15 undefined
        case 16: return "String formula too complex";
        case 17: return "Cannot continue";
        case 18: return "Function not defined";
        case 19: return "No RESUME";
        case 20: return "RESUME without error";
        //error 21-23 undefined
        case 24: return "Device timeout";
        case 25: return "Device fault";
        case 26: return "FOR without NEXT";
        case 27: return "Out of paper";
        //error 28 undefined
        case 29: return "WHILE without WEND";
        case 30: return "WEND without WHILE";
        //error 31-32 undefined
        case 33: return "Duplicate label";
        //error 34 undefined
        case 35: return "Subprogram not defined";
        //error 36 undefined
        case 37: return "Argument-count mismatch";
        case 38: return "Array not defined";
        case 40: return "Variable required";
        case 50: return "FIELD overflow";
        case 51: return "Internal error";
        case 52: return "Bad file name or number";
        case 53: return "File not found";
        case 54: return "Bad file mode";
        case 55: return "File already open";
        case 56: return "FIELD statement active";
        case 57: return "Device I/O error";
        case 58: return "File already exists";
        case 59: return "Bad record length";
        case 61: return "Disk full";
        case 62: return "Input past end of file";
        case 63: return "Bad record number";
        case 64: return "Bad file name";
        case 67: return "Too many files";
        case 68: return "Device unavailable";
        case 69: return "Communication-buffer overflow";
        case 70: return "Permission denied";
        case 71: return "Disk not ready";
        case 72: return "Disk-media error";
        case 73: return "Feature unavailable";
        case 74: return "Rename across disks";
        case 75: return "Path/File access error";
        case 76: return "Path not found";
        case 258: return "Invalid handle";
        case 300: return "Memory region out of range";
        case 301: return "Invalid size";
        case 302: return "Source memory region out of range";
        case 303: return "Destination memory region out of range";
        case 304: return "Source and destination memory regions out of range";
        case 305: return "Source memory has been freed";
        case 306: return "Destination memory has been freed";
        case 307: return "Memory already freed";
        case 308: return "Memory has been freed";
        case 309: return "Memory not initialized";
        case 310: return "Source memory not initialized";
        case 311: return "Destination memory not initialized";
        case 312: return "Source and destination memory not initialized";
        case 313: return "Source and destination memory have been freed";
        case 314: return "_ASSERT failed";
        case 315: return "_ASSERT failed (check console for description)";
        default: return "Unprintable error";
    }
    // clang-format on
}

void clear_error() {
    new_error = 0;
}

double get_error_erl() {
    return error_erl;
}

uint32_t get_error_err() {
    return error_err;
}

int32_t func__errorline() {
    return ercl;
}

int32_t func__inclerrorline() {
    return inclercl;
}

qbs *func__inclerrorfile() {
    return qbs_new_txt(includedfilename);
}

void error_set_line(uint32_t errorline, uint32_t incerrorline, const char *incfilename) {
    ercl = errorline;
    inclercl = incerrorline;
    includedfilename = incfilename;

    // Bootstrap compatibility: an executable generated by an older compiler
    // has no explicit error_track_line(...) calls, so keep its critical-error
    // position warm through the legacy event path. A new compiler clears the
    // tracker at startup; after that, $ErrorLocation controls this tracker.
    if (!explicit_line_tracking_seen) {
        current_ercl = errorline;
        current_inclercl = incerrorline;
        current_includedfilename = incfilename;
    }
}

void error_track_line(uint32_t errorline, uint32_t incerrorline, const char *incfilename) {
    explicit_line_tracking_seen = true;
    current_ercl = errorline;
    current_inclercl = incerrorline;
    current_includedfilename = incfilename;
}

qbs *func__errormessage(int32_t errorcode, int32_t passed) {
    if (!passed)
        errorcode = get_error_err();
    return qbs_new_txt(error_code_to_text(errorcode));
}

extern uint8_t close_program;
extern double last_line;
void end();

// Fatal errors may be raised from the BASIC thread or from SUB _GL. Route the
// final shutdown through libqb_exit(), which transfers exit handling to the
// GLUT thread when a graphics window is active and exits directly otherwise.
// This is the same cross-platform path on Windows, Linux and macOS.
static void exit_after_fatal_error() {
    close_program = 1;
    libqb_exit(0);
}

extern void QBMAIN(void *);

void fix_error() {
    char *errtitle = NULL, *errmess = NULL;
    const char *cp;
    int prevent_handling = 0, len, v;
    if ((new_error >= 300) && (new_error <= 315))
        prevent_handling = 1;
    if (!error_goto_line || error_handling || prevent_handling) {
        // strip path from binary name
        static int32_t i;
        static qbs *binary_name = NULL;
        if (!binary_name)
            binary_name = qbs_new(0, 0);
        qbs_set(binary_name, qbs_add(func_command(0, 1), qbs_new_txt_len("\0", 1)));
        for (i = binary_name->len; i > 0; i--) {
            if ((binary_name->chr[i - 1] == 47) || (binary_name->chr[i - 1] == 92)) {
                qbs_set(binary_name, func_mid(binary_name, i + 1, 0, 0));
                break;
            }
        }

        cp = error_code_to_text(new_error);
#define FIXERRMSG_TITLE "%s%u - %s"
#define FIXERRMSG_BODY "Line: %u (in %s)\n%s%s"
#define FIXERRMSG_MAINFILE "main module"
#define FIXERRMSG_CONT "\nContinue?"
#define FIXERRMSG_UNHAND "Unhandled Error #"
#define FIXERRMSG_CRIT "Critical Error #"

        len = snprintf(errmess, 0, FIXERRMSG_BODY, (inclercl ? inclercl : ercl), (inclercl ? includedfilename : FIXERRMSG_MAINFILE), cp,
                       (!prevent_handling ? FIXERRMSG_CONT : ""));
        errmess = (char *)malloc(len + 1);
        if (!errmess) {
            exit_after_fatal_error(); // At this point we just give up
            return;
        }
        snprintf(errmess, len + 1, FIXERRMSG_BODY, (inclercl ? inclercl : ercl), (inclercl ? includedfilename : FIXERRMSG_MAINFILE), cp,
                 (!prevent_handling ? FIXERRMSG_CONT : ""));

        len = snprintf(errtitle, 0, FIXERRMSG_TITLE, (!prevent_handling ? FIXERRMSG_UNHAND : FIXERRMSG_CRIT), new_error, binary_name->chr);
        errtitle = (char *)malloc(len + 1);
        if (!errtitle) {
            exit_after_fatal_error(); // At this point we just give up
            return;
        }
        snprintf(errtitle, len + 1, FIXERRMSG_TITLE, (!prevent_handling ? FIXERRMSG_UNHAND : FIXERRMSG_CRIT), new_error, binary_name->chr);

        if (prevent_handling) {
            v = gui_alert(errmess, errtitle, "ok");
            exit_after_fatal_error();
            return;
        } else {
            v = gui_alert(errmess, errtitle, "yesno");
        }

        if ((v == 2) || (v == 0)) {
            close_program = 1;
            end();
        }
        new_error = 0;
        return;
    }
    error_err = new_error;
    new_error = 0;
    error_erl = last_line;
    error_occurred = 1;

    // FIXME: EWWWWW, there's no way this is correct
    QBMAIN(NULL);
    return;
}

void error(int32_t error_number) {
    libqb_log_error("QB64 Error %d reported: %s", error_number, error_code_to_text(error_number));

    // critical errors:

    // Out-of-memory errors include the BASIC source module and line that were executing.
    // Use fixed-size stack buffers here: allocating memory while reporting an
    // allocation failure could itself fail.
    int32_t critical_number = 0;
    if (error_number == 257)
        critical_number = 1;
    else if (error_number >= 502 && error_number <= 518)
        critical_number = error_number - 500;

    if (critical_number) {
        char errmess[512];
        char errtitle[32];
        uint32_t srcLine;
        const char *srcFile;

        if (get_current_error_location(srcLine, srcFile))
            snprintf(errmess, sizeof(errmess), "Out of memory (in %s) on line %u", srcFile, srcLine);
        else
            snprintf(errmess, sizeof(errmess), "Out of memory\nEnable $ErrorLocation:ON for source location details.");

        snprintf(errtitle, sizeof(errtitle), "Critical Error #%d", critical_number);
        gui_alert(errmess, errtitle, "ok");
        exit_after_fatal_error();
        return;
    }

    // Other fatal runtime errors used to bypass the standard error-event
    // path and therefore discarded the source position captured by
    // $ErrorLocation:ON. Keep their existing messages and titles, but route
    // all of them through the same allocation-free location formatter.
    const char *critical_message = NULL;
    switch (error_number) {
        case QB_ERROR_DIVISION_BY_ZERO:
            critical_message = "Division by zero";
            break;
        case 256:
            critical_message = "Out of stack space";
            break;
        case 259:
            critical_message = "Cannot find dynamic library file";
            break;
        case 260:
        case 261:
            critical_message = "Sub/Function does not exist in dynamic library";
            break;
        case 270:
            critical_message = "_GL command called outside of SUB _GL's scope";
            break;
        case 271:
            critical_message = "END/SYSTEM called within SUB _GL's scope";
            break;
    }

    if (critical_message) {
        gui_alert(critical_error_message(critical_message), "Critical Error", "ok");
        exit_after_fatal_error();
        return;
    }

    if (!new_error) {
        if ((new_error == 256) || (new_error == 257))
            fix_error(); // critical error!
        if (error_number <= 0)
            error_number = 5; // Illegal function call
        new_error = error_number;
        qbevent = 1;
    }
}
