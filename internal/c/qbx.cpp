#include "qbx.h"

#ifdef QB64_WINDOWS
int _CRT_glob = -1; // enable globbing on llvm-mingw by default
#endif

void requestKeyboardOverlayImage(int32 handle) {
    requestedKeyboardOverlayImage = handle;
}

int32 timer_event_occurred = 0; // inc/dec as each GOSUB to QBMAIN ()
                                // begins/ends
int32 timer_event_id = 0;
int32 key_event_occurred = 0; // inc/dec as each GOSUB to QBMAIN () begins/ends
int32 key_event_id = 0;
int32 strig_event_occurred = 0; // inc/dec as each GOSUB to QBMAIN ()
                                // begins/ends
int32 strig_event_id = 0;
uint16 call_absolute_offsets[256];
uint32 dbgline;
uint32 qbs_cmem_sp = 256;
uint32 cmem_sp = 65536;
intptr_t dblock; // 32bit offset of dblock
uint8 close_program = 0;
int32 tab_spc_cr_size = 1; // 1=PRINT(default), 2=FILE
int32 tab_fileno = 0;      // only valid if tab_spc_cr_size=2
int32 tab_LPRINT = 0;      // 1=dest is LPRINT image

uint64 *nothingvalue; // a pointer to 8 empty bytes in dblock
uint32 bkp_new_error = 0;
qbs *nothingstring;
uint32 qbevent = 0;
uint8 suspend_program = 0;
uint8 stop_program = 0;
uint8_t cmem[1114099]; // 16*65535+65535+3 (enough for highest referenceable dword in conv memory)
uint8 *cmem_static_pointer = &cmem[0] + 1280 + 65536;
uint8 *cmem_dynamic_base = &cmem[0] + 655360;
uint8 *mem_static;
uint8 *mem_static_pointer;
uint8 *mem_static_limit;
double last_line = 0;

uint32 next_return_point = 0;
uint32 *return_point = (uint32 *)malloc(4 * 16384);
uint32 return_points = 16384;
void *qbs_input_variableoffsets[257];
int32 qbs_input_variabletypes[257];

// qbmain specific global variables
char g_tmp_char;
uint8 g_tmp_uchar;
int16 g_tmp_short;
uint16 g_tmp_ushort;
int32 g_tmp_long;
uint32 g_tmp_ulong;

int8 g_tmp_int8;
uint8 g_tmp_uint8;
int16 g_tmp_int16;
uint16 g_tmp_uint16;
int32 g_tmp_int32;
uint32 g_tmp_uint32;
int64 g_tmp_int64;
uint64 g_tmp_uint64;
float g_tmp_float;
double g_tmp_double;
long double g_tmp_longdouble;

qbs *g_tmp_str;
qbs *g_swap_str;
qbs *pass_str;
ptrszint data_offset = 0;


void swap_string(qbs *a, qbs *b) {
    static qbs *c;
    c = qbs_new(a->len, 0);
    memcpy(c->chr, a->chr, a->len);
    qbs_set(a, b);
    qbs_set(b, c);
    qbs_free(c);
}

void swap_block(void *a, void *b, uint32 bytes) {
    static uint32 quads;
    quads = bytes >> 2;
    static uint32 *a32, *b32;
    a32 = (uint32 *)a;
    b32 = (uint32 *)b;
    while (quads--) {
        static uint32 c;
        c = *a32;
        *a32++ = *b32;
        *b32++ = c;
    }
    bytes &= 3;
    static uint8 *a8, *b8;
    a8 = (uint8 *)a32;
    b8 = (uint8 *)b32;
    while (bytes--) {
        static uint8 c;
        c = *a8;
        *a8++ = *b8;
        *b8++ = c;
    }
}

static std::atomic<bool> disableEvents = false;

ptrszint check_lbound(ptrszint *array, int32 index, int32 num_indexes) {
    static ptrszint ret;
    disableEvents = true;
    ret = func_lbound((ptrszint *)(*array), index, num_indexes);
    clear_error();
    disableEvents = false;
    return ret;
}

ptrszint check_ubound(ptrszint *array, int32 index, int32 num_indexes) {
    static ptrszint ret;
    disableEvents = true;
    ret = func_ubound((ptrszint *)(*array), index, num_indexes);
    clear_error();
    disableEvents = false;
    return ret;
}

uint64 call_getubits(uint32 bsize, ptrszint *array, ptrszint i) {
    return getubits(bsize, (uint8 *)(*array), i);
}

int64 call_getbits(uint32 bsize, ptrszint *array, ptrszint i) {
    return getbits(bsize, (uint8 *)(*array), i);
}

void call_setbits(uint32 bsize, ptrszint *array, ptrszint i, int64 val) {
    setbits(bsize, (uint8 *)(*array), i, val);
}

int32 logical_drives() {
#ifdef QB64_WINDOWS
    return GetLogicalDrives();
#else
    return 0;
#endif
}


#include "global.txt"

// DECLARE LIBRARY header includes, kept out of regsf.txt so that regsf.txt stays
// safe to include from every translation unit.
//
// Guarded because ./internal/source doesn't yet have the file.
#if __has_include("libheaders.txt")
#    include "libheaders.txt"
#endif
#include "regsf.txt"

// the DECLARE LIBRARY variables themselves; regsf.txt carries an extern for each
#if __has_include("libdefs.txt")
#    include "libdefs.txt"
#endif

// set_dynamic_info is called immediately when
// main() begins, to set global, static variables
// controlling app init
void set_dynamic_info() {
#include "dyninfo.txt"
}

void sub_clear(int32 ignore, int32 ignore2, int32 stack, int32 passed) {
    static ptrszint tmp_long;
// note: stack can be ignored
#include "clear.txt"
    // reset DATA read offset
    data_offset = 0;
    // close open files
    sub_close(NULL, NULL); // closes all open files
    // free images
    freeallimages();
    // stop & free sounds (note: QB also stops any sound from the PLAY command)
    // invalidate RETURN location(s)
    next_return_point = 0;
    // reset error goto location to 'unhandled'
    error_goto_line = 0;
    uint32 qbs_tmp_base = qbs_tmp_list_nexti;
    qbs_set(error_handler_history, qbs_new_txt_len("", 0));
    qbs_cleanup(qbs_tmp_base, 0);
    // invalidate RESUME
    error_handling = 0;
    return;
}

int32 run_from_line = 0;
// run_from_line's value is an index in a list of possible "run from" locations
// when 0, the program runs from the beginning

void sub__icon(int32 i, int32 i2, int32 passed);

void sub__display();
void sub__autodisplay();
int32 func__autodisplay();

void chain_input() {
    // note: common data or not, every program must check for chained data,
    //      it could be sharing files or screen state

    // check if command$ contains a tmp chain directive
    int32 FF;

    if ((func_command(0, 0))->len >= 32) {
        if (qbs_equal(qbs_right(func_command(0, 0), 4), qbs_new_txt_len(".tmp", 4))) {
            if (qbs_equal(func_mid(func_command(0, 0), (func_command(0, 0))->len - 31, 25, 1), qbs_new_txt_len("(unique-tag:=/@*$+-)chain", 25))) {
                FF = func_freefile();
                sub_open(func_mid(func_command(0, 0), (func_command(0, 0))->len - 11, 12, 1), 2, NULL, NULL, FF, NULL, 0);

                static int32 int32val, int32val2;
                static int64 int64val, int64val2;
                static int64 bytes, bytei;
                static qbs *tqbs;
                static ptrszint tmp_long;

                // CHDIR directive
                static uint8 chdir_data[4096];
                sub_get(FF, NULL, byte_element((uint64)&int32val, 4),
                        0); // assume CHDIR directive 512
                sub_get(FF, NULL, byte_element((uint64)&int32val, 4),
                        0); // assume len
                sub_get(FF, NULL, byte_element((uint64)chdir_data, int32val),
                        0); // data
                chdir_data[int32val] = 0;

                chain_restorescreenstate(FF);

                // get first command
                sub_get(FF, NULL, byte_element((uint64)&int32val, 4), 0);

// read COMMON data
#include "inpchain.txt"

                sub_close(FF, 1);

                sub_kill(func_mid(func_command(0, 0), (func_command(0, 0))->len - 11, 12, 1));

                chdir((char *)chdir_data);

                // remove chain tag from COMMAND$
                func_command_str->len -= 32;
                // remove trailing space (if any)
                if (func_command_str->len)
                    func_command_str->len--;
            }
        }
    }
}

void sub_chain(qbs *f) {
    if (is_error_pending())
        return;

#ifdef QB64_WINDOWS

    // run program
    static qbs *str = NULL;
    if (str == NULL)
        str = qbs_new(0, 0);
    static qbs *str2 = NULL;
    if (str2 == NULL)
        str2 = qbs_new(0, 0);

    static int32 i, i2, x;
    static qbs *strz = NULL;
    if (!strz)
        strz = qbs_new(0, 0);
    static char *cp;

    if (!f->len) {
        error(53);
        return;
    } // file not found (as in QB)
    qbs_set(str, f);
    qbs_set(str2, qbs_ucase(str));

    static qbs *f_exe = NULL;
    if (!f_exe)
        f_exe = qbs_new(0, 0);
    static qbs *f_bas = NULL;
    if (!f_bas)
        f_bas = qbs_new(0, 0); // no parameters
    static qbs *f_path = NULL;
    if (!f_path)
        f_path = qbs_new(0, 0);
    static int32 path_len;
    static qbs *current_path = NULL;
    if (!current_path)
        current_path = qbs_new(0, 0);
    static qbs *thisexe_path = NULL;
    if (!thisexe_path)
        thisexe_path = qbs_new(0, 0);

    // note: supports arguments after filename

    f_bas->len = 0;
    for (i = 0; i < str->len; i++) {
        if (str->chr[i] == 46) {
            //.bas?
            if ((i + 3) < str->len) {
                if ((str2->chr[i + 1] == 66) && (str2->chr[i + 2] == 65) && (str2->chr[i + 3] == 83)) { //"BAS"
                    qbs_set(f_bas, str);
                    f_bas->len = i + 4;  // arguments truncated
                    qbs_set(f_exe, str); // change .bas to .exe
                    f_exe->chr[i + 1] = 101;
                    f_exe->chr[i + 2] = 120;
                    f_exe->chr[i + 3] = 101; //"exe"
                    goto extensions_ready;
                } //"BAS"
            } // bas
            //.exe?
            if ((i + 3) < str->len) {
                if ((str2->chr[i + 1] == 69) && (str2->chr[i + 2] == 88) && (str2->chr[i + 3] == 69)) { //"EXE"
                    qbs_set(f_bas, str);
                    f_bas->len = i + 4; // arguments truncated, change .exe to .bas
                    f_bas->chr[i + 1] = 98;
                    f_bas->chr[i + 2] = 97;
                    f_exe->chr[i + 3] = 115; //"bas"
                    qbs_set(f_exe, str);     // note: exe kept as is
                    goto extensions_ready;
                } //"EXE"
            } // exe
            break; // no meaningful extension found
        } //"."
    }

    // no extension given!
    // note: It is more 'likely' that the user will want to pass arguments than
    // chain a
    //      filename containing spaces. Therefore, only everything left of
    //      left-most space will be considered the path+filename.
    i2 = str->len; // last character index of filename
    for (i = str->len - 1; i; i--) {
        if (str->chr[i] == 32)
            i2 = i;
    }
    qbs_set(str2, qbs_right(str, str->len - i2)); //[+extension]
    str->len = i2;                                //[path+]file
    qbs_set(f_exe, qbs_add(qbs_add(str, qbs_new_txt(".exe ")), str2));
    qbs_set(f_bas, qbs_add(str, qbs_new_txt(".bas")));

extensions_ready:

    // normalize dir slashes
    filepath_fix_directory(f_exe);
    filepath_fix_directory(f_bas);

    // get path (strip paths from f_exe & f_bas)
    f_path->len = 0;
    for (i = f_bas->len - 1; i >= 0; i--) {
        if ((f_bas->chr[i] == 92) || (f_bas->chr[i] == 47) || (f_bas->chr[i] == 58)) {
            qbs_set(f_path, f_bas);
            f_path->len = i + 1;
            if (f_bas->chr[i] == 58) {
                f_path->chr[i + 1] = 92;
                f_path->len++;
            } // add "\" to ":"
            // strip paths
            memmove(f_exe->chr, &f_exe->chr[i + 1], f_exe->len - (i + 1));
            f_exe->len -= (i + 1);
            memmove(f_bas->chr, &f_bas->chr[i + 1], f_bas->len - (i + 1));
            f_bas->len -= (i + 1);
            break;
        }
    }

    static uint8 path_data[4096];
    static int32 defaultpath;

    defaultpath = 0;
    if (!f_path->len) { // use current path if no path specified
        defaultpath = 1;
        // get current path (add \ if necessary)
        i = GetCurrentDirectoryA(4096, (char *)path_data);
        qbs_set(f_path, func_space(i + 1));
        memcpy(f_path->chr, path_data, i);
        if ((f_path->chr[i - 1] != 92) && (f_path->chr[i - 1] != 47)) {
            f_path->chr[i] = 92;
        } else {
            f_path->len--;
        }
    }

    // get current program's exe's path (including "\")
    GetModuleFileNameA(NULL, (char *)path_data, 4096);
    i = strlen((char *)path_data);
    for (i2 = i - 1; i2 >= 0; i2--) {
        x = path_data[i2];
        if ((x == 92) || (x == 47) || (x == 58)) {
            if (x == 58)
                i2++;
            i2++;
            break;
        }
    }
    qbs_set(thisexe_path, func_space(i2));
    memcpy(thisexe_path->chr, path_data, i2);
    thisexe_path->chr[i2] = 92; //"\"

    // 1. create & open a temporary file to pass information to the chained
    // program
    double TD;
    int32 TL, FF;
    qbs *TS = NULL;
    if (TS == NULL)
        TS = qbs_new(0, 0);
    qbs *TFS = NULL;
    if (TFS == NULL)
        TFS = qbs_new(0, 0);
    TD = func_timer(0.001E+0, 1);
    TL = qbr(std::floor(TD));
    TL = qbr((TD - TL) * 999);
    if (TL < 100)
        TL = 100; // ensure value is a 3 digit number
    qbs_set(TS, qbs_ltrim(qbs_str((int32)(TL))));
    qbs_set(TFS, qbs_add(qbs_add(qbs_new_txt_len("chain", 5), TS), qbs_new_txt_len(".tmp", 4)));
    FF = func_freefile();
    sub_open(TFS, 2, NULL, NULL, FF, NULL, 0); // opened in BINARY mode

    // add common data
    static int32 int32val, int32val2;
    static int64 int64val, int64val2;
    static qbs *tqbs;
    static int64 bytes, bytei;
    static ptrszint tmp_long;

    // CHDIR directive
    int32val = 512;
    sub_put(FF, NULL, byte_element((uint64)&int32val, 4), 0);
    int32val = f_path->len - 1;
    sub_put(FF, NULL, byte_element((uint64)&int32val, 4), 0);
    sub_put(FF, NULL, byte_element((uint64)f_path->chr, f_path->len - 1),
            0); //-1 removes trailing "\"

    chain_savescreenstate(FF);

#    include "chain.txt"
    // add "end of commands" value
    int32val = 0;
    sub_put(FF, NULL, byte_element((uint64)&int32val, 4), 0);

    sub_close(FF, 1);

    // move chain???.tmp file to path
    if (!defaultpath) {
        qbs_set(str, qbs_new_txt("move /Y "));
        qbs_set(str, qbs_add(str, qbs_new_txt_len("\x022", 1)));
        qbs_set(str, qbs_add(str, TFS));
        qbs_set(str, qbs_add(str, qbs_new_txt_len("\x022", 1)));
        qbs_set(str, qbs_add(str, qbs_new_txt(" ")));
        qbs_set(str, qbs_add(str, qbs_new_txt_len("\x022", 1)));
        qbs_set(str, qbs_add(str, f_path));
        str->len--; // remove trailing "\" of dest path
        qbs_set(str, qbs_add(str, qbs_new_txt_len("\x022", 1)));
        qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
        WinExec((char *)strz->chr, SW_HIDE);
    }

    static int32 method;
    method = 1;

chain_retry:

    if (method == 1) {
        qbs_set(str, qbs_add(f_path, f_exe));
    }

    if (method == 2) {
        // move chain???.tmp file to 'thisexe_path' path
        qbs_set(str, qbs_new_txt("move /Y "));
        qbs_set(str, qbs_add(str, qbs_new_txt_len("\x022", 1)));
        qbs_set(str, qbs_add(str, f_path));
        qbs_set(str, qbs_add(str, TFS));
        qbs_set(str, qbs_add(str, qbs_new_txt_len("\x022", 1)));
        qbs_set(str, qbs_add(str, qbs_new_txt(" ")));
        qbs_set(str, qbs_add(str, qbs_new_txt_len("\x022", 1)));
        qbs_set(str, qbs_add(str, thisexe_path));
        str->len--; // remove trailing "\" of dest path
        qbs_set(str, qbs_add(str, qbs_new_txt_len("\x022", 1)));
        qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));
        sub_shell(str, 1);
        qbs_set(str, qbs_add(thisexe_path, f_exe));
    }

    if (method == 3) {
        // attempt .bas compilation
        qbs_set(str, qbs_new_txt_len("\x022", 1));
        qbs_set(str, qbs_add(str, thisexe_path));
        qbs_set(str, qbs_add(str, qbs_new_txt("qb64pe.exe")));
        qbs_set(str, qbs_add(str, qbs_new_txt_len("\x022", 1)));
        qbs_set(str, qbs_add(str, qbs_new_txt(" -c ")));
        qbs_set(str, qbs_add(str, f_path));
        qbs_set(str, qbs_add(str, f_bas));
        sub_shell(str, 1);
        qbs_set(str, qbs_add(thisexe_path, f_exe));
    }

    // add a space
    qbs_set(str, qbs_add(str, qbs_new_txt(" ")));
    // add chain tag
    qbs_set(str, qbs_add(str, qbs_new_txt_len("(unique-tag:=/@*$+-)", 20)));
    // add chain file name
    qbs_set(str, qbs_add(str, TFS));
    // add NULL terminator
    qbs_set(strz, qbs_add(str, qbs_new_txt_len("\0", 1)));

#    ifdef QB64_WINDOWS

    if (WinExec((char *)strz->chr, SW_SHOWDEFAULT) > 31) {
        goto run_exit;
    } else {
        goto run_failed;
    }

#    else

    system((char *)strz->chr);
    // success?
    goto run_exit;

#    endif

// exit this program
run_exit:
    close_program = 1;
    end();
    exit(99); //<--this line should never actually be executed

// failed
run_failed:

    if (method == 1) {
        method = 2;
        goto chain_retry;
    }
    if (method == 2) {
        method = 3;
        goto chain_retry;
    }

    qbs_set(str, qbs_add(thisexe_path, TFS));
    sub_kill(str); // remove tmp file (chain specific)
    error(53);
    return; // file not found

#endif
}

// note: index 0 is unused
int32 device_last = 0;   // last used device
int32 device_max = 1000; // number of allocated indexes
device_struct *devices = (device_struct *)calloc(1000 + 1, sizeof(device_struct));

static constexpr size_t kDeviceAxisWheelSize = sizeof(double);

// device_struct helper functions
uint8 getDeviceEventButtonValue(device_struct *device, int32 eventIndex, int32 objectIndex) {
    return *(device->events + eventIndex * device->event_size + device->lastaxis * kDeviceAxisWheelSize + device->lastwheel * kDeviceAxisWheelSize +
             objectIndex);
}

void setDeviceEventButtonValue(device_struct *device, int32 eventIndex, int32 objectIndex, uint8 value) {
    *(device->events + eventIndex * device->event_size + device->lastaxis * kDeviceAxisWheelSize + device->lastwheel * kDeviceAxisWheelSize + objectIndex) =
        value;
}

double getDeviceEventAxisValue(device_struct *device, int32 eventIndex, int32 objectIndex) {
    return *(double *)(device->events + eventIndex * device->event_size + objectIndex * kDeviceAxisWheelSize);
}

void setDeviceEventAxisValue(device_struct *device, int32 eventIndex, int32 objectIndex, double value) {
    *(double *)(device->events + eventIndex * device->event_size + objectIndex * kDeviceAxisWheelSize) = value;
}

double getDeviceEventWheelValue(device_struct *device, int32 eventIndex, int32 objectIndex) {
    return *(double *)(device->events + eventIndex * device->event_size + device->lastaxis * kDeviceAxisWheelSize + objectIndex * kDeviceAxisWheelSize);
}

void setDeviceEventWheelValue(device_struct *device, int32 eventIndex, int32 objectIndex, double value) {
    *(double *)(device->events + eventIndex * device->event_size + device->lastaxis * kDeviceAxisWheelSize + objectIndex * kDeviceAxisWheelSize) = value;
}

void setupDevice(device_struct *device) {
    int32 size = (int32)(device->lastaxis * kDeviceAxisWheelSize + device->lastwheel * kDeviceAxisWheelSize) + device->lastbutton;
    size += (int32)sizeof(int64_t); // for appended ordering index
    size += (int32)sizeof(int64_t) - 1;
    size = size - (size & ((int32)sizeof(int64_t) - 1)); // align to int64_t boundary
    device->event_size = size;
    device->events = (uint8 *)calloc(2, device->event_size); // create initial 'current' and 'previous' events
    device->max_events = 2;
    device->queued_events = 2;
    device->connected = 1;
    device->used = 1;
}

int32 createDeviceEvent(device_struct *device) {
    uint8 *cp, *cp2;
    if (device->queued_events == device->max_events) { // expand/shift event buffer
        if (device->max_events >= QUEUED_EVENTS_LIMIT) {
            // discard base message
            memmove(device->events, device->events + device->event_size, (device->queued_events - 1) * device->event_size);
            device->queued_events--;
        } else {
            cp = (uint8 *)calloc(device->max_events * 2, device->event_size);       // create new buffer
            memcpy(cp, device->events, device->queued_events * device->event_size); // copy events from old buffer into
                                                                                    // new buffer
            cp2 = device->events;
            device->events = cp;
            device->max_events *= 2;
            free(cp2);
        }
    }
    // copy previous event data into new event
    memmove(device->events + device->queued_events * device->event_size, device->events + (device->queued_events - 1) * device->event_size, device->event_size);
    *(int64 *)(device->events + (device->queued_events * device->event_size) + (device->event_size - (int32)sizeof(int64_t))) =
        device_event_index++; // set global event index
    int32 eventIndex = device->queued_events;
    return eventIndex;
}

void commitDeviceEvent(device_struct *device) {
    device->queued_events++;
}

int32 func__devices() {
    return device_last;
}

int32 device_selected = 0;

qbs *func__device(int32 i, int32 passed) {
    if (!passed)
        i = device_selected;
    if (i < 1 || i > device_last) {
        error(5);
        return qbs_new(0, 1);
    }
    return qbs_new_txt(devices[i].name);
}

int32 func__deviceinput(int32 i, int32 passed) {
    static device_struct *d;
    static int32 retval;
    retval = -1;
    device_selected = -1;

    if (!passed) {
        // find oldest event across all devices
        static int32 i2;
        static int64 index, lowest_index;
        i2 = -1;
        for (i = 1; i <= device_last; i++) {
            d = &devices[i];
            if (d->queued_events > 2) {
                index = *(int64 *)((d->events + d->event_size * 2) + (d->event_size - (int32)sizeof(int64_t)));
                if ((i2 == -1) || (index < lowest_index)) {
                    i2 = i;
                    lowest_index = index;
                    retval = i2;
                } // first/lower
            } // queued_events>2
        } // i
        if (i2 != -1)
            i = i2;
        else
            return 0;
    }

    if (i < 1 || i > device_last)
        error(5);
    d = &devices[i];

    device_selected = i;

    if (d->queued_events > 2) {
        memmove(d->events, ((uint8 *)d->events) + d->event_size, (d->queued_events - 1) * d->event_size);
        d->queued_events--;
        return retval;
    }

    return 0;
}

int32 func__button(int32 i, int32 passed) {
    if (device_selected < 1 || device_selected > device_last) {
        error(5);
        return 0;
    }
    static device_struct *d;
    d = &devices[device_selected];
    if (!passed)
        i = 1;
    if (i < 1 || i > d->lastbutton) {
        error(5);
        return 0;
    }
    if (getDeviceEventButtonValue(d, 1, i - 1))
        return -1;
    return 0;
}

int32 func__buttonchange(int32 i, int32 passed) {
    if (device_selected < 1 || device_selected > device_last) {
        error(5);
        return 0;
    }
    static device_struct *d;
    d = &devices[device_selected];
    if (!passed)
        i = 1;
    if (i < 1 || i > d->lastbutton) {
        error(5);
        return 0;
    }
    static int32 old_value, value;
    value = getDeviceEventButtonValue(d, 1, i - 1);
    old_value = getDeviceEventButtonValue(d, 0, i - 1);
    if (value > old_value)
        return -1;
    if (value < old_value)
        return 1;
    return 0;
}

double func__axis(int32 i, int32 passed) {
    if (device_selected < 1 || device_selected > device_last) {
        error(5);
        return 0;
    }
    static device_struct *d;
    d = &devices[device_selected];
    if (!passed)
        i = 1;
    if (i < 1 || i > d->lastaxis) {
        error(5);
        return 0;
    }
    return getDeviceEventAxisValue(d, 1, i - 1);
}

double func__wheel(int32 i, int32 passed) {
    if (device_selected < 1 || device_selected > device_last) {
        error(5);
        return 0;
    }
    static device_struct *d;
    d = &devices[device_selected];
    if (!passed)
        i = 1;
    if (i < 1 || i > d->lastwheel) {
        error(5);
        return 0;
    }
    return getDeviceEventWheelValue(d, 1, i - 1);
}

int32 func__lastbutton(int32 di, int32 passed) {
    if (!passed)
        di = device_selected;
    if (di < 1 || di > device_last)
        error(5);
    static device_struct *d;
    d = &devices[di];
    return d->lastbutton;
}

int32 func__lastaxis(int32 di, int32 passed) {
    if (!passed)
        di = device_selected;
    if (di < 1 || di > device_last)
        error(5);
    static device_struct *d;
    d = &devices[di];
    return d->lastaxis;
}

int32 func__lastwheel(int32 di, int32 passed) {
    if (!passed)
        di = device_selected;
    if (di < 1 || di > device_last)
        error(5);
    static device_struct *d;
    d = &devices[di];
    return d->lastwheel;
}

onstrig_struct *onstrig = (onstrig_struct *)calloc(65536, sizeof(onstrig_struct)); // note: up to 256 controllers with up to
                                                                                   // 256 buttons each supported
int32 onstrig_inprogress = 0;

void onstrig_setup(int32 i, int32 controller, int32 controller_passed, uint32 id, int64 pass) {
    // note: pass is ignored by ids not requiring a pass value
    if (is_error_pending())
        return;
    if (i < 0 || i > 65535) {
        error(5);
        return;
    }
    if (controller_passed) {
        if (controller < 1 || controller > 65535) {
            error(5);
            return;
        }
    } else {
        controller = 1;
        if (i & 2) {
            controller = 2;
            i -= 2;
        }
    }
    static int32 button;
    button = (i >> 2) + 1;
    if (i & 1) {
        error(5);
        return;
    } //'currently down' state cannot be used as an ON STRIG event
    if (controller > 256 || button > 256)
        return;                                // error-less exit for (currently) unsupported ranges
    i = (controller - 1) * 256 + (button - 1); // reindex
    onstrig[i].state = 0;
    onstrig[i].pass = pass;
    onstrig[i].id = id; // id must be set last because it is the trigger
                        // variable
    if (device_last == 0)
        func__devices(); // init device interface (if not already setup)
}

void sub_strig(int32 i, int32 controller, int32 option, int32 passed) {
    // ref: "[(?[,?])]{ON|OFF|STOP}"
    if (is_error_pending())
        return;
    // Note: QuickBASIC ignores STRIG ON and STRIG OFF statements--the
    // statements are provided for compatibility with earlier versions,
    //      Reference: http://www.antonis.de/qbebooks/gwbasman/strig.html
    //      QB64 makes STRIG ON/OFF/STOP change the checking status for all
    //      buttons
    static int32 i1, i2;
    if (passed > 0) {
        if (i < 0 || i > 65535) {
            error(5);
            return;
        }
        if (passed & 2) {
            if (controller < 1 || controller > 65535) {
                error(5);
                return;
            }
        } else {
            controller = 1;
            if (i & 2) {
                controller = 2;
                i -= 2;
            }
        }
        static int32 button;
        button = (i >> 2) + 1;
        if (i & 1) {
            error(5);
            return;
        } //'currently down' state cannot be used as an ON STRIG event
        if (controller > 256 || button > 256)
            return;                                // error-less exit for (currently) unsupported ranges
        i = (controller - 1) * 256 + (button - 1); // reindex
        i1 = i;
        i2 = i;
    } else {
        i1 = 0;
        i2 = 65535;
    }
    for (i = i1; i <= i2; i++) {
        // ref: uint8 active;//0=OFF, 1=ON, 2=STOP
        if (option == 1) { // ON
            onstrig[i].active = 1;
            if (onstrig[i].state)
                qbevent = 1;
        }
        if (option == 2) { // OFF
            onstrig[i].active = 0;
            onstrig[i].state = 0;
        }
        if (option == 3) { // STOP
            onstrig[i].active = 2;
            if (onstrig[i].state)
                onstrig[i].state = 1;
        }
    } // i
}

int32 ontimer_nextfree = 1;
int32 *ontimer_freelist = (int32 *)malloc(32);
uint32 ontimer_freelist_size = 8;      // number of elements in the freelist
uint32 ontimer_freelist_available = 0; // element (if any) which is available)
ontimer_struct *ontimer = (ontimer_struct *)malloc(sizeof(ontimer_struct));
// note: index 0 of the above cannot be allocated/freed

int32 ontimerthread_lock = 0;

void stop_timers() {
    ontimerthread_lock = 1;
    while (ontimerthread_lock != 2)
        ;
}

void start_timers() {
    ontimerthread_lock = 0;
}

int32 func__freetimer() {
    if (is_error_pending())
        return 0;
    static int32 i;
    if (ontimer_freelist_available) {
        i = ontimer_freelist[ontimer_freelist_available--];
    } else {
        ontimerthread_lock = 1;
        while (ontimerthread_lock == 1)
            Sleep(0); // mutex
        ontimer = (ontimer_struct *)realloc(ontimer, sizeof(ontimer_struct) * (ontimer_nextfree + 1));
        if (!ontimer)
            error(257);         // out of memory
        ontimerthread_lock = 0; // mutex
        i = ontimer_nextfree;
        ontimer[i].state = 0; // state is not set to 0 if reusing an existing
                              // index as event could still be in progress
    }
    ontimer[i].active = 0;
    ontimer[i].id = 0;
    ontimer[i].allocated = 1;
    if (i == ontimer_nextfree)
        ontimer_nextfree++;
    return i;
}

void freetimer(int32 i) {
    ontimer[i].allocated = 0;
    ontimer[i].id = 0;
    if (ontimer_freelist_available == ontimer_freelist_size) {
        ontimer_freelist_size *= 2;
        ontimer_freelist = (int32 *)realloc(ontimer_freelist, ontimer_freelist_size * 4);
    }
    ontimer_freelist[++ontimer_freelist_available] = i;
}

void ontimer_setup(int32 i, double sec, uint32 id, int64 pass) {
    // note: pass is ignored by ids not requiring a pass value
    if (is_error_pending())
        return;
    if ((i < 0) || (i >= ontimer_nextfree)) {
        error(5);
        return;
    }
    if (!ontimer[i].allocated) {
        error(5);
        return;
    }
    if (ontimer[i].state == 1)
        ontimer[i].state = 0; // retract prev event if not in progress
    ontimer[i].seconds = sec;
    ontimer[i].pass = pass;
    ontimer[i].last_time = 0;
    ontimer[i].id = id; // id must be set last because it is the trigger
                        // variable
}

void sub_timer(int32 i, int32 option, int32 passed) {
    // ref: "[(?)]{ON|OFF|STOP|FREE}"
    if (is_error_pending())
        return;
    if (!passed)
        i = 0;
    if ((i < 0) || (i >= ontimer_nextfree)) {
        error(5);
        return;
    }
    if (!ontimer[i].allocated) {
        error(5);
        return;
    }
    // ref: uint8 active;//0=OFF, 1=ON, 2=STOP
    if (option == 1) { // ON
        ontimer[i].active = 1;

        // This is necessary so that if a timer triggered while stopped we will run it now.
        qbevent = 1;
        return;
    }
    if (option == 2) { // OFF
        ontimer[i].active = 0;
        if (ontimer[i].state == 1)
            ontimer[i].state = 0; // retract event if not in progress
        ontimer[i].last_time = 0; // when ON is next used, the timer will start over
        return;
    }
    if (option == 3) { // STOP
        ontimer[i].active = 2;
        return;
    }
    if (option == 4) { // FREE
        if (i == 0) {
            error(5);
            return;
        }
        ontimer[i].active = 0;
        if (ontimer[i].state == 1)
            ontimer[i].state = 0; // retract event if not in progress
        freetimer(i);
        // note: if an event is still in progress, it will set state to 0 when
        // it finishes
        //      which may delay the first instance of this index if it is
        //      immediately reused
        return;
    }
}

void TIMERTHREAD(void *unused) {
    static int32 i;
    static double time_now = 100000;
    while (1) {
    quick_lock:
        if (ontimerthread_lock == 1)
            ontimerthread_lock = 2; // mutex, verify lock
        if (!ontimerthread_lock) {  // mutex
            time_now = ((double)GetTicks()) * 0.001;
            for (i = 0; i < ontimer_nextfree; i++) {
                if (ontimer[i].allocated && ontimer[i].id && ontimer[i].active && !ontimer[i].state) {
                    if (!ontimer[i].last_time) {
                        ontimer[i].last_time = time_now;
                    } else if (time_now - ontimer[i].last_time > ontimer[i].seconds) {
                        // keep measured time for accurate
                        // number of calls overall
                        ontimer[i].last_time += ontimer[i].seconds;

                        // if difference between actual time and
                        // measured time is beyond 'seconds' set
                        // measured to actual
                        if (std::fabs(time_now - ontimer[i].last_time) >= ontimer[i].seconds)
                            ontimer[i].last_time = time_now;
                        ontimer[i].state = 1;
                        qbevent = 1;
                    } // time check
                }
                if (ontimerthread_lock == 1)
                    goto quick_lock;
            } // i
        } // not locked
        Sleep(1);
        if (stop_program) {
            exit_ok |= 2;
            return;
        } // close thread #2
    } // while(1)
    return;
}

void events() {
    int32 i, x, d, di;
    int64 i64;

// onstrig events
onstrig_recheck:
    if (!error_handling) { // no new calls happen whilst error handling
        di = 0;
        for (d = 1; d <= device_last; d++) {
            if (devices[d].type == 1) {
                if (di <= 255) {
                    for (i = 0; i <= 255; i++) {
                        if (onstrig[(di << 8) + i].id) {
                            if (onstrig[(di << 8) + i].active == 1) { // if STOPped, event will be postponed
                                if (onstrig[(di << 8) + i].state) {
                                    if (!onstrig_inprogress) {
                                        onstrig_inprogress = 1;
                                        onstrig[(di << 8) + i].state--;
                                        x = onstrig[(di << 8) + i].id;
                                        i64 = onstrig[(di << 8) + i].pass;
                                        switch (x) {
#include "onstrig.txt"
                                        // example.....
                                        // case 1:
                                        //...
                                        // break;
                                        default:
                                            break;
                                        } // switch
                                        onstrig_inprogress = 0;
                                        goto onstrig_recheck;
                                    } //! inprogress
                                } // state
                            } // active==1
                        } // id
                    } // i
                } // di<=255
                di++;
            } // type==1
        } // d
    } //! error_handling

// onkey events
onkey_recheck:
    if (!error_handling) { // no new calls happen whilst error handling
        for (i = 1; i <= 31; i++) {
            if (onkey[i].id) {
                if (onkey[i].active == 1) { // if STOPped, event will be postponed
                    if (onkey[i].state) {
                        if (!onkey_inprogress) {
                            onkey_inprogress = 1;
                            onkey[i].state--;
                            x = onkey[i].id;
                            i64 = onkey[i].pass;
                            switch (x) {
#include "onkey.txt"
                            // example.....
                            // case 1:
                            //...
                            // break;
                            default:
                                break;
                            } // switch
                            onkey_inprogress = 0;
                            goto onkey_recheck;
                        } //! inprogress
                    } // state
                } // active==1
            } // id
        } // i
    } //! error_handling

    // ontimer events
    if (!error_handling) { // no new on timer calls happen whilst error handling
        for (i = 0; i < ontimer_nextfree; i++) {
            if (ontimer[i].allocated) {
                if (ontimer[i].id) {
                    if (ontimer[i].active == 1) { // if timer STOPped, event will be postponed
                        if (ontimer[i].state == 1) {
                            ontimer[i].state = 2; // event in progress
                            x = ontimer[i].id;
                            i64 = ontimer[i].pass;
                            switch (x) {
#include "ontimer.txt"
                            // example.....
                            // case 1:
                            //...
                            // break;
                            default:
                                break;
                            } // switch
                            ontimer[i].state = 0; // event finished
                            sleep_break = 1;
                        } // state==1
                    } // active==1
                } // id
            } // allocated
        } // i
    } //! error_handling
}

extern int64 display_lock_request;
extern int64 display_lock_confirmed;
extern int64 display_lock_released;

uint32 r;

void evnt(uint32 linenumber, uint32 inclinenumber, const char *incfilename) {
    if (disableEvents)
        return;

    qbevent = 0;

    if (sub_gl_called == 0) {
        if (display_lock_request > display_lock_confirmed) {
            display_lock_confirmed = display_lock_request;
            while ((display_lock_released < display_lock_confirmed) && (!close_program) && (!suspend_program) && (!stop_program))
                Sleep(1);
        }
    }

    r = 0;

    while (suspend_program || stop_program) {
        if (stop_program)
            end();
        Sleep(10);
    }

    if (is_error_pending()) {
        error_set_line(linenumber, inclinenumber, incfilename);
        fix_error();
        if (error_retry) {
            error_retry = 0;
            r = 1;
        }
    } else {
        if (sub_gl_called == 0)
            events();
    }
}

uint8 *redim_preserve_cmem_buffer = (uint8 *)malloc(65536); // used for temporary storage only (move to libqbx?)

void division_by_zero_handler(int ignore) {
    error(11);
}

void segv_handler(int ignore) {
    libqb_log_error("Recieved SIGSEGV! Review below stacktrace:");

    // Trigger the SIGSEGV again to crash the program (this allows for things
    // like coredumps to be collected)
    signal(SIGSEGV, SIG_DFL);
    raise(SIGSEGV);
}

// void SIGSEGV_handler(int ignore){
//    error(256);//assume stack overflow? (the most likely cause)
//}

void QBMAIN(void *unused) {
    fpu_reinit();
#ifdef QB64_WINDOWS
    signal(SIGFPE, division_by_zero_handler);
// signal(SIGSEGV, SIGSEGV_handler);
#else
    struct sigaction sig_act;
    sig_act.sa_handler = division_by_zero_handler;
    sigemptyset(&(sig_act.sa_mask));
    sig_act.sa_flags = 0;
    sigaction(SIGFPE, &sig_act, NULL);

    signal(SIGSEGV, segv_handler);
#endif

    ptrszint tmp_long;
    int32 tmp_fileno;
    qbs *tqbs;
    uint32 qbs_tmp_base = qbs_tmp_list_nexti;
    static mem_lock *sf_mem_lock = NULL;
    if (!sf_mem_lock) {
        new_mem_lock();
        sf_mem_lock = mem_lock_tmp;
        sf_mem_lock->type = 3;
    }

#include "maindata.txt"
#include "mainerr.txt"
#include "runline.txt"
    if (timer_event_occurred) {
        timer_event_occurred--;
#include "ontimerj.txt"
    }
    if (key_event_occurred) {
        key_event_occurred--;
#include "onkeyj.txt"
    }
    if (strig_event_occurred) {
        strig_event_occurred--;
#include "onstrigj.txt"
    }
    chain_input();
    libqb_log_info("QB64 code starting.");

// When the program is built as separate object files this holds only the main
// module and the routines pinned to it; the rest live in their own units.
#ifdef QB64_SPLIT_TUS
#    include "main_split.txt"
#else
#    include "main.txt"
#endif

    //} (closed by main.txt)
