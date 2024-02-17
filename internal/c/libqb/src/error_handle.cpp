
#include "libqb-common.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "command.h"
#include "event.h"
#include "gui.h"
#include "error_handle.h"

uint32_t new_error;
uint32_t error_occurred;
uint32_t error_retry;
uint32_t error_err; //=0;
uint32_t error_goto_line;
uint32_t error_handling;

static double error_erl; //=0;
static uint32_t ercl;
static uint32_t inclercl;

static const char *includedfilename;

static const char *human_error(int32_t errorcode) {
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

void clear_error()
{
    new_error = 0;
}

double get_error_erl()
{
    return error_erl;
}

uint32_t get_error_err()
{
    return error_err;
}

int32_t func__errorline()
{
    return ercl;
}

int32_t func__inclerrorline()
{
    return inclercl;
}

qbs *func__inclerrorfile()
{
    return qbs_new_txt(includedfilename);
}

void error_set_line(uint32_t errorline, uint32_t incerrorline, const char *incfilename)
{
    ercl = errorline;
    inclercl = incerrorline;
    includedfilename = incfilename;
}

qbs *func__errormessage(int32_t errorcode, int32_t passed) {
    if (!passed)
        errorcode = get_error_err();
    return qbs_new_txt(human_error(errorcode));
}

extern uint8_t close_program;
extern double last_line;
void end();

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

        cp = human_error(new_error);
#define FIXERRMSG_TITLE "%s%u - %s"
#define FIXERRMSG_BODY "Line: %u (in %s)\n%s%s"
#define FIXERRMSG_MAINFILE "main module"
#define FIXERRMSG_CONT "\nContinue?"
#define FIXERRMSG_UNHAND "Unhandled Error #"
#define FIXERRMSG_CRIT "Critical Error #"

        len = snprintf(errmess, 0, FIXERRMSG_BODY, (inclercl ? inclercl : ercl), (inclercl ? includedfilename : FIXERRMSG_MAINFILE), cp,
                       (!prevent_handling ? FIXERRMSG_CONT : ""));
        errmess = (char *)malloc(len + 1);
        if (!errmess)
            exit(0); // At this point we just give up
        snprintf(errmess, len + 1, FIXERRMSG_BODY, (inclercl ? inclercl : ercl), (inclercl ? includedfilename : FIXERRMSG_MAINFILE), cp,
                 (!prevent_handling ? FIXERRMSG_CONT : ""));

        len = snprintf(errtitle, 0, FIXERRMSG_TITLE, (!prevent_handling ? FIXERRMSG_UNHAND : FIXERRMSG_CRIT), new_error, binary_name->chr);
        errtitle = (char *)malloc(len + 1);
        if (!errtitle)
            exit(0); // At this point we just give up
        snprintf(errtitle, len + 1, FIXERRMSG_TITLE, (!prevent_handling ? FIXERRMSG_UNHAND : FIXERRMSG_CRIT), new_error, binary_name->chr);

        if (prevent_handling) {
            v = gui_alert(errmess, errtitle, "ok");
            exit(0);
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

    // critical errors:

    // out of memory errors
    if (error_number == 257) {
        gui_alert("Out of memory", "Critical Error #1", "ok");
        exit(0);
    } // generic "Out of memory" error
    // tracable "Out of memory" errors
    if (error_number == 502) {
        gui_alert("Out of memory", "Critical Error #2", "ok");
        exit(0);
    }
    if (error_number == 503) {
        gui_alert("Out of memory", "Critical Error #3", "ok");
        exit(0);
    }
    if (error_number == 504) {
        gui_alert("Out of memory", "Critical Error #4", "ok");
        exit(0);
    }
    if (error_number == 505) {
        gui_alert("Out of memory", "Critical Error #5", "ok");
        exit(0);
    }
    if (error_number == 506) {
        gui_alert("Out of memory", "Critical Error #6", "ok");
        exit(0);
    }
    if (error_number == 507) {
        gui_alert("Out of memory", "Critical Error #7", "ok");
        exit(0);
    }
    if (error_number == 508) {
        gui_alert("Out of memory", "Critical Error #8", "ok");
        exit(0);
    }
    if (error_number == 509) {
        gui_alert("Out of memory", "Critical Error #9", "ok");
        exit(0);
    }
    if (error_number == 510) {
        gui_alert("Out of memory", "Critical Error #10", "ok");
        exit(0);
    }
    if (error_number == 511) {
        gui_alert("Out of memory", "Critical Error #11", "ok");
        exit(0);
    }
    if (error_number == 512) {
        gui_alert("Out of memory", "Critical Error #12", "ok");
        exit(0);
    }
    if (error_number == 513) {
        gui_alert("Out of memory", "Critical Error #13", "ok");
        exit(0);
    }
    if (error_number == 514) {
        gui_alert("Out of memory", "Critical Error #14", "ok");
        exit(0);
    }
    if (error_number == 515) {
        gui_alert("Out of memory", "Critical Error #15", "ok");
        exit(0);
    }
    if (error_number == 516) {
        gui_alert("Out of memory", "Critical Error #16", "ok");
        exit(0);
    }
    if (error_number == 517) {
        gui_alert("Out of memory", "Critical Error #17", "ok");
        exit(0);
    }
    if (error_number == 518) {
        gui_alert("Out of memory", "Critical Error #18", "ok");
        exit(0);
    }

    // other critical errors
    if (error_number == 11) {
        gui_alert("Division by zero", "Critical Error", "ok");
        exit(0);
    }
    if (error_number == 256) {
        gui_alert("Out of stack space", "Critical Error", "ok");
        exit(0);
    }
    if (error_number == 259) {
        gui_alert("Cannot find dynamic library file", "Critical Error", "ok");
        exit(0);
    }
    if (error_number == 260) {
        gui_alert("Sub/Function does not exist in dynamic library", "Critical Error", "ok");
        exit(0);
    }
    if (error_number == 261) {
        gui_alert("Sub/Function does not exist in dynamic library", "Critical Error", "ok");
        exit(0);
    }

    if (error_number == 270) {
        gui_alert("_GL command called outside of SUB _GL's scope", "Critical Error", "ok");
        exit(0);
    }
    if (error_number == 271) {
        gui_alert("END/SYSTEM called within SUB _GL's scope", "Critical Error", "ok");
        exit(0);
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

