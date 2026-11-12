#pragma once

// Declarations shared by every translation unit of a compiled QB64-PE program.
//
// This holds a variety of different things, mostly:
//
//   - Function signatures from libqb.cpp (should be moved to other headers in the future)
//   - Function signatures from qbx.cpp
//   - Inlined functions.
//
// This file mainly exists to support split builds where some of the QB64-PE
// generated C++ is built separate from qbx.cpp (but still needs access to the
// stuff it declares).

#include "common.h"

#include "array-copy.h"
#include "audio.h"
#include "bitops.h"
#include "clipboard.h"
#include "command.h"
#include "compression.h"
#include "console.h"
#include "datetime.h"
#include "encoding.h"
#include "environ.h"
#include "error_handle.h"
#include "event.h"
#include "extended_math.h"
#include "file-fields.h"
#include "filepath.h"
#include "filesystem.h"
#include "font.h"
#include "game_controller.h"
#include "graphics.h"
#include "gui.h"
#include "hashing.h"
#include "hexoctbin.h"
#include "image.h"
#include "key-events.h"
#include "keyboard.h"
#include "logging.h"
#include "memblock.h"
#include "mouse.h"
#include "printer.h"
#include "qbmath.h"
#include "qbs-mk-cv.h"
#include "qbs.h"
#include "rounding.h"
#include "shell.h"
#include "window.h"

#include <atomic>

extern void unlockvWatchHandle();
extern int32 vWatchHandle();

#ifdef QB64_MACOSX
#    include <ApplicationServices/ApplicationServices.h>
#endif

extern int32 sub_gl_called;

#ifdef QB64_GUI
#    ifdef DEPENDENCY_GL
// Generated into the program's temp folder by QB64-PE, and found on the include
// path the generated code is compiled with. Only a program that uses GL is built
// with DEPENDENCY_GL, which is also the only case in which this file exists.
#        include "gl_helper_code.h"
#    endif
#endif

// forward references
void QBMAIN(void *);
void TIMERTHREAD(void *);

extern int32 requestedKeyboardOverlayImage;

void requestKeyboardOverlayImage(int32 handle);

// extern functions

extern void sub__fps(double fps, int32 passed);

extern qbs *func__readfile(qbs *filespec);
extern void sub__writefile(qbs *filespec, qbs *contents);
extern void sub__assert(int32 expression, qbs *assert_message, int32 passed);

extern qbs *func__embedded(qbs *handle);

extern void sub__glrender(int32 method);
extern void sub__displayorder(int32 method1, int32 method2, int32 method3, int32 method4);

extern mem_block func__memimage(int32, int32);

extern void sub__controlchr(int32);
extern int32 func__controlchr();
extern void sub__blink(int32);
extern int32 func__blink();
extern void set_foreground_window(ptrszint i);
extern int32 func_stick(int32 i, int32 axis_group, int32 passed);
extern int32 func_strig(int32 i, int32 controller, int32 passed);
extern void sub_paletteusing(void *element, int32 bits);
extern int64 func_read_int64(uint8 *data, ptrszint *data_offset, ptrszint data_size);
extern int64 func_read_uint64(uint8 *data, ptrszint *data_offset, ptrszint data_size);
extern void key_on();
extern void key_off();
extern void key_list();
extern void key_assign(int32 i, qbs *str);
extern qbs *func__os();
extern qbs *func__compdate();
extern qbs *func__comptime();
extern qbs *func__compvers();
extern void sub__mapunicode(int32 unicode_code, int32 ascii_code);
extern int32 func__mapunicode(int32 ascii_code);
extern int32 func_lpos(int32);
extern void sub__screenprint(qbs *txt);
extern void sub__screenclick(int32 x, int32 y, int32 button, int32 passed);
extern int32 func__screenimage(int32 x1, int32 y1, int32 x2, int32 y2, int32 passed);
extern void sub_lock(int32 i, int64 start, int64 end, int32 passed);
extern void sub_unlock(int32 i, int64 start, int64 end, int32 passed);
void chain_restorescreenstate(int32);
void chain_savescreenstate(int32);
extern int32 func__exit();
extern void revert_input_check();
extern int32 func__openhost(qbs *);
extern int32 func__openconnection(int32);
extern int32 func__openclient(qbs *);
extern int32 func__connected(int32);
extern qbs *func__connectionaddress(int32);
extern void sub_draw(qbs *);
extern void qbs_maketmp(qbs *);
extern void sub_run(qbs *);
extern void sub_run_init();
extern void freeallimages();
extern void call_interrupt(int32, void *, void *);
extern void call_interruptx(int32, void *, void *);
extern void restorepalette(img_struct *im);
extern void pset(int32 x, int32 y, uint32 col);
extern uint32 newimg();
extern int32 freeimg(uint32);
extern void imgrevert(int32);
extern int32 imgframe(uint8 *o, int32 x, int32 y, int32 bpp);
extern int32 imgnew(int32 x, int32 y, int32 bpp);
extern void sub__putimage(double f_dx1, double f_dy1, double f_dx2, double f_dy2, int32 src, int32 dst, double f_sx1, double f_sy1, double f_sx2, double f_sy2,
                          int32 passed);
extern int32 selectfont(int32 f, img_struct *im);
extern uint32 sib();
extern uint32 sib_mod0();
extern uint8 *rm8();
extern uint16 *rm16();
extern uint32 *rm32();
extern void cpu_call();
extern int64 build_int64(uint32 val2, uint32 val1);
extern uint64 build_uint64(uint32 val2, uint32 val1);
extern char *human_error(int32 errorcode);
extern void end();
extern int32 stop_program_state();
extern uint8 *mem_static_malloc(uint32 size);
extern void mem_static_restore(uint8 *restore_point);
extern uint8 *cmem_dynamic_malloc(uint32 size);
extern void cmem_dynamic_free(uint8 *block);
extern void sub_defseg(int32 segment, int32 passed);
extern int32 func_peek(int32 offset);
extern void sub_poke(int32 offset, int32 value);
extern void more_return_points();
extern qbs *func_varptr_helper(uint8 type, uint16 offset);
extern void lineclip(int32 x1, int32 y1, int32 x2, int32 y2, int32 xmin, int32 ymin, int32 xmax, int32 ymax);
extern void qbg_palette(uint32 attribute, uint32 col, int32 passed);
extern void qbg_sub_color(uint32 col1, uint32 col2, uint32 bordercolor, int32 i, int32 passed);
extern void defaultcolors();
extern void qbg_screen(int32 mode, int32 color_switch, int32 active_page, int32 visual_page, int32 refresh, int32 passed);
extern void sub_pcopy(int32 src, int32 dst);
extern void qbsub_width(int32 option, int32 value1, int32 value2, int32 value3, int32 value4, int32 passed);
extern void pset(int32 x, int32 y, uint32 col);
extern void pset_and_clip(int32 x, int32 y, uint32 col);
extern void qb32_boxfill(float x1f, float y1f, float x2f, float y2f, uint32 col);
extern void fast_boxfill(int32 x1, int32 y1, int32 x2, int32 y2, uint32 col);
extern void fast_line(int32 x1, int32 y1, int32 x2, int32 y2, uint32 col);
extern void qb32_line(float x1f, float y1f, float x2f, float y2f, uint32 col, uint32 style);
extern void sub_line(float x1, float y1, float x2, float y2, uint32 col, int32 bf, uint32 style, int32 passed);
extern void sub_paint32(float x, float y, uint32 fillcol, uint32 bordercol, int32 passed);
extern void sub_paint32x(float x, float y, uint32 fillcol, uint32 bordercol, int32 passed);
extern void sub_paint(float x, float y, uint32 fillcol, uint32 bordercol, qbs *backgroundstr, int32 passed);
extern void sub_paint(float x, float y, qbs *fillstr, uint32 bordercol, qbs *backgroundstr, int32 passed);
extern void sub_circle(double x, double y, double r, uint32 col, double start, double end, double aspect, int32 passed);
extern uint32 point(int32 x, int32 y);
extern double func_point(float x, float y, int32 passed);
extern void sub_pset(float x, float y, uint32 col, int32 passed);
extern void sub_preset(float x, float y, uint32 col, int32 passed);
extern void printchr(int32 character);
extern int32_t chrwidth(uint32_t character);
extern void newline();
extern void lprint_makefit(qbs *text);
extern void tab();
extern void qbs_lprint(qbs *str, int32 finish_on_new_line);
extern void qbg_sub_view(int32 x1, int32 y1, int32 x2, int32 y2, int32 fillcolor, int32 bordercolor, int32 passed);
extern void qbg_sub_locate(int32 row, int32 column, int32 cursor, int32 start, int32 stop, int32 passed);
extern int32 hexoct2uint64(qbs *h);
extern void qbs_input(int32 numvariables, uint8 newline);
extern void sub_out(int32 port, int32 data);
extern void sub_randomize(double seed, int32 passed);
extern float func_rnd(float n, int32 passed);
// following are declared below to allow for inlining
// extern double func_abs(double d);
// extern long double func_abs(long double d);
// extern float func_abs(float d);

// extern void sub_open(qbs *name,int32 type,int32 access,int32 sharing,int32
// i,int32 record_length,int32 passed);
extern void sub_open(qbs *name, int32 type, int32 access, int32 sharing, int32 i, int64 record_length, int32 passed);
extern void sub_open_gwbasic(qbs *typestr, int32 i, qbs *name, int64 record_length, int32 passed);

extern void sub_close(int32 i2, int32 passed);
extern int32 file_input_chr(int32 i);
extern void file_input_nextitem(int32 i, int32 lastc);
extern void sub_file_print(int32 i, qbs *str, int32 extraspace, int32 tab, int32 newline);
extern int32 n_roundincrement();
extern int32 n_float();
extern int32 n_int64();
extern int32 n_uint64();
extern int32 n_inputnumberfromdata(uint8 *data, ptrszint *data_offset, ptrszint data_size);
extern int32 n_inputnumberfromfile(int32 fileno);
extern void sub_file_line_input_string(int32 fileno, qbs *deststr);
extern void sub_file_input_string(int32 fileno, qbs *deststr);
extern int64 func_file_input_int64(int32 fileno);
extern uint64 func_file_input_uint64(int32 fileno);
extern void sub_read_string(uint8 *data, ptrszint *data_offset, ptrszint data_size, qbs *deststr);
extern long double func_read_float(uint8 *data, ptrszint *data_offset, ptrszint data_size, int32 typ);
extern long double func_file_input_float(int32 fileno, int32 typ);
extern void *byte_element(uint64 offset, int32 length);
extern void *byte_element(uint64 offset, int32 length, byte_element_struct *info);
extern void sub_get(int32 i, int64 offset, void *element, int32 passed);
extern void sub_get2(int32 i, int64 offset, qbs *str, int32 passed);

extern void sub_put(int32 i, int64 offset, void *element, int32 passed);
extern void sub_put2(int32 i, int64 offset, void *element, int32 passed);
extern void sub_graphics_get(float x1f, float y1f, float x2f, float y2f, void *element, uint32 mask, int32 passed);
extern void sub_graphics_put(float x1f, float y1f, void *element, int32 option, uint32 mask, int32 passed);
extern int32 func_csrlin();
extern void sub_sleep(int32 seconds, int32 passed);
extern ptrszint func_lbound(ptrszint *array, int32 index, int32 num_indexes);
extern ptrszint func_ubound(ptrszint *array, int32 index, int32 num_indexes);

extern int32 func_inp(int32 port);
extern void sub_wait(int32 port, int32 andexpression, int32 xorexpression, int32 passed);
extern qbs *func_tab(int32 pos);
extern qbs *func_spc(int32 spaces);
extern float func_pmap(float val, int32 option);
extern uint32 func_screen(int32 y, int32 x, int32 returncol, int32 passed);
extern void sub_bsave(qbs *filename, int32 offset, int32 size);
extern void sub_bload(qbs *filename, int32 offset, int32 passed);

extern int64 func_lof(int32 i);
extern int32 func_eof(int32 i);
extern void sub_seek(int32 i, int64 pos);
extern int64 func_seek(int32 i);
extern int64 func_loc(int32 i);
extern qbs *func_input(int32 n, int32 i, int32 passed);
extern int32 func__statusCode(int32 handle);

extern int32 func_freefile();

extern void call_absolute(int32 args, uint16 offset);
extern void sub__blend(int32 i, int32 passed);
extern void sub__dontblend(int32 i, int32 passed);
extern void sub__clearcolor(uint32 c, int32 i, int32 passed);
extern void sub__setalpha(int32 a, uint32 c, uint32 c2, int32 i, int32 passed);
extern int32 func__width(int32 i, int32 passed);
extern int32 func__height(int32 i, int32 passed);
extern int32 func__pixelsize(int32 i, int32 passed);
extern int32 func__clearcolor(int32 i, int32 passed);
extern int32 func__blend(int32 i, int32 passed);
extern uint32 func__defaultcolor(int32 i, int32 passed);
extern uint32 func__backgroundcolor(int32 i, int32 passed);
extern uint32 func__palettecolor(int32 n, int32 i, int32 passed);
extern void sub__palettecolor(int32 n, uint32 c, int32 i, int32 passed);
extern void sub__copypalette(int32 i, int32 i2, int32 passed);
extern void sub__printstring(float x, float y, qbs *text, int32 i, int32 passed);
extern int32_t func__loadfont(const qbs *qbsFileName, int32_t size, const qbs *qbsRequirements, int32_t font_index, int32_t passed);
extern void sub__font(int32 f, int32 i, int32 passed);
extern int32 func__fontwidth(int32 f, int32 passed);
extern int32 func__fontheight(int32 f, int32 passed);
extern int32 func__font(int32 i, int32 passed);
extern void sub__freefont(int32 f);
extern void sub__printmode(int32 mode, int32 i, int32 passed);
extern int32 func__printmode(int32 i, int32 passed);
extern uint32 matchcol(int32 r, int32 g, int32 b);
extern uint32 matchcol(int32 r, int32 g, int32 b, int32 i);
extern uint32 func__rgb(int32 r, int32 g, int32 b, int32 i, int32 passed);
extern uint32 func__rgba(int32 r, int32 g, int32 b, int32 a, int32 i, int32 passed);
extern int32 func__alpha(uint32 col, int32 i, int32 passed);
extern int32 func__red(uint32 col, int32 i, int32 passed);
extern int32 func__green(uint32 col, int32 i, int32 passed);
extern int32 func__blue(uint32 col, int32 i, int32 passed);
extern void sub_end();
extern int32 print_using(qbs *f, int32 s2, qbs *dest, qbs *pu_str);
extern int32 print_using_integer64(qbs *format, int64 value, int32 start, qbs *output);
extern int32 print_using_uinteger64(qbs *format, uint64 value, int32 start, qbs *output);
extern int32 print_using_single(qbs *format, float value, int32 start, qbs *output);
extern int32 print_using_double(qbs *format, double value, int32 start, qbs *output);
extern int32 print_using_float(qbs *format, long double value, int32 start, qbs *output);

// shared global variables
extern int32 sleep_break;
extern int64 exit_code;
extern int32 lock_mainloop; // 0=unlocked, 1=lock requested, 2=locked
extern int64 device_event_index;
extern int32_t exit_ok;

// variables defined in qbx.cpp
extern int32 timer_event_occurred;
extern int32 timer_event_id;
extern int32 key_event_occurred;
extern int32 key_event_id;
extern int32 strig_event_occurred;
extern int32 strig_event_id;
extern uint16 call_absolute_offsets[256];
extern uint32 dbgline;
extern uint32 qbs_cmem_sp;
extern uint32 cmem_sp;
extern intptr_t dblock;
extern uint8 close_program;
extern int32 tab_spc_cr_size;
extern int32 tab_fileno;
extern int32 tab_LPRINT;
extern uint64 *nothingvalue;
extern uint32 bkp_new_error;
extern qbs *nothingstring;
extern uint32 qbevent;
extern uint8 suspend_program;
extern uint8 stop_program;
extern uint8_t cmem[1114099];
extern uint8 *cmem_static_pointer;
extern uint8 *cmem_dynamic_base;
extern uint8 *mem_static;
extern uint8 *mem_static_pointer;
extern uint8 *mem_static_limit;
extern double last_line;
extern uint32 next_return_point;
extern uint32 *return_point;
extern uint32 return_points;
extern void *qbs_input_variableoffsets[257];
extern int32 qbs_input_variabletypes[257];
extern char g_tmp_char;
extern uint8 g_tmp_uchar;
extern int16 g_tmp_short;
extern uint16 g_tmp_ushort;
extern int32 g_tmp_long;
extern uint32 g_tmp_ulong;
extern int8 g_tmp_int8;
extern uint8 g_tmp_uint8;
extern int16 g_tmp_int16;
extern uint16 g_tmp_uint16;
extern int32 g_tmp_int32;
extern uint32 g_tmp_uint32;
extern int64 g_tmp_int64;
extern uint64 g_tmp_uint64;
extern float g_tmp_float;
extern double g_tmp_double;
extern long double g_tmp_longdouble;
extern qbs *g_tmp_str;
extern qbs *g_swap_str;
extern qbs *pass_str;
extern ptrszint data_offset;
extern int32 run_from_line;
extern int32 device_last;
extern int32 device_max;
extern device_struct *devices;
extern int32 device_selected;
extern onstrig_struct *onstrig;
extern int32 onstrig_inprogress;
extern int32 ontimer_nextfree;
extern int32 *ontimer_freelist;
extern uint32 ontimer_freelist_size;
extern uint32 ontimer_freelist_available;
extern ontimer_struct *ontimer;
extern int32 ontimerthread_lock;
extern int64 display_lock_request;
extern int64 display_lock_confirmed;
extern int64 display_lock_released;
extern uint32 r;
extern uint8 *redim_preserve_cmem_buffer;

// functions defined in qbx.cpp
void sub__icon(int32 i, int32 i2, int32 passed);
void sub__display();
void sub__autodisplay();
int32 func__autodisplay();
void swap_string(qbs *a, qbs *b);
void swap_block(void *a, void *b, uint32 bytes);
ptrszint check_lbound(ptrszint *array, int32 index, int32 num_indexes);
ptrszint check_ubound(ptrszint *array, int32 index, int32 num_indexes);
uint64 call_getubits(uint32 bsize, ptrszint *array, ptrszint i);
int64 call_getbits(uint32 bsize, ptrszint *array, ptrszint i);
void call_setbits(uint32 bsize, ptrszint *array, ptrszint i, int64 val);
int32 logical_drives();
void set_dynamic_info();
void sub_clear(int32 ignore, int32 ignore2, int32 stack, int32 passed);
void chain_input();
void sub_chain(qbs *f);
uint8 getDeviceEventButtonValue(device_struct *device, int32 eventIndex, int32 objectIndex);
void setDeviceEventButtonValue(device_struct *device, int32 eventIndex, int32 objectIndex, uint8 value);
double getDeviceEventAxisValue(device_struct *device, int32 eventIndex, int32 objectIndex);
void setDeviceEventAxisValue(device_struct *device, int32 eventIndex, int32 objectIndex, double value);
double getDeviceEventWheelValue(device_struct *device, int32 eventIndex, int32 objectIndex);
void setDeviceEventWheelValue(device_struct *device, int32 eventIndex, int32 objectIndex, double value);
void setupDevice(device_struct *device);
int32 createDeviceEvent(device_struct *device);
void commitDeviceEvent(device_struct *device);
int32 func__devices();
qbs *func__device(int32 i, int32 passed);
int32 func__deviceinput(int32 i, int32 passed);
int32 func__button(int32 i, int32 passed);
int32 func__buttonchange(int32 i, int32 passed);
double func__axis(int32 i, int32 passed);
double func__wheel(int32 i, int32 passed);
int32 func__lastbutton(int32 di, int32 passed);
int32 func__lastaxis(int32 di, int32 passed);
int32 func__lastwheel(int32 di, int32 passed);
void onstrig_setup(int32 i, int32 controller, int32 controller_passed, uint32 id, int64 pass);
void sub_strig(int32 i, int32 controller, int32 option, int32 passed);
void stop_timers();
void start_timers();
int32 func__freetimer();
void freetimer(int32 i);
void ontimer_setup(int32 i, double sec, uint32 id, int64 pass);
void sub_timer(int32 i, int32 option, int32 passed);
void TIMERTHREAD(void *unused);
void events();
void evnt(uint32 linenumber, uint32 inclinenumber, const char *incfilename);
void division_by_zero_handler(int ignore);
void segv_handler(int ignore);
void QBMAIN(void *unused);

// inline functions
inline void swap_8(void *a, void *b) {
    uint8 x;
    x = *(uint8 *)a;
    *(uint8 *)a = *(uint8 *)b;
    *(uint8 *)b = x;
}

inline void swap_16(void *a, void *b) {
    uint16 x;
    x = *(uint16 *)a;
    *(uint16 *)a = *(uint16 *)b;
    *(uint16 *)b = x;
}

inline void swap_32(void *a, void *b) {
    uint32 x;
    x = *(uint32 *)a;
    *(uint32 *)a = *(uint32 *)b;
    *(uint32 *)b = x;
}

inline void swap_64(void *a, void *b) {
    uint64 x;
    x = *(uint64 *)a;
    *(uint64 *)a = *(uint64 *)b;
    *(uint64 *)b = x;
}

inline void swap_longdouble(void *a, void *b) {
    long double x;
    x = *(long double *)a;
    *(long double *)a = *(long double *)b;
    *(long double *)b = x;
}

inline ptrszint array_check(uptrszint index, uptrszint limit) {
    // nb. forces signed index into an unsigned variable for quicker comparison
    if (index < limit)
        return index;
    error(9);
    return 0;
}

inline uint16 varptr_dblock_check(uint8 *off) {
    // note: 66816 is the top of DBLOCK (SEG:80+OFF:65536)
    if (off < (&cmem[66816])) { // in DBLOCK?
        return ((uint16)(off - &cmem[1280]));
    } else {
        return ((uint32)(off - cmem)) & 15;
    }
}

inline uint16 varseg_dblock_check(uint8 *off) {
    // note: 66816 is the top of DBLOCK (SEG:80+OFF:65536)
    if (off < (&cmem[66816])) { // in DBLOCK?
        return 80;
    } else {
        return ((uint32)(off - cmem)) / 16;
    }
}
