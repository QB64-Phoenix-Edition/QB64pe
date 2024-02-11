#ifndef INC_LIBQB_H
#define INC_LIBQB_H
#include "common.h"

#include "qbs.h"

void sub_shell4(qbs *, int32); //_DONTWAIT & _HIDE
int32 func__source();
int32 func_pos(int32 ignore);
void sub__printimage(int32 i);
double func_timer(double accuracy, int32_t passed);
int32 func__newimage(int32 x, int32 y, int32 bpp, int32 passed);
void display();
void validatepage(int32);
void sub__dest(int32);
void sub__source(int32);
int32 func__printwidth(qbs *, int32, int32);
void sub_cls(int32, uint32, int32);
void qbs_print(qbs *, int32);
int32 func__copyimage(int32 i, int32 mode, int32 passed);
void sub__freeimage(int32 i, int32 passed);
int32 func__dest();
int32 func__display();
void qbg_sub_view_print(int32, int32, int32);
qbs *func_space(int32 spaces);
void makefit(qbs *text);
void qbg_sub_window(float, float, float, float, int32);
extern int32 autodisplay;
// GFS forward references
int32 gfs_eof_passed(int32 i);
int32 gfs_eof_reached(int32 i);
int64 gfs_getpos(int32 i);
int32 gfs_fileno_valid(int32 f);
int32 gfs_fileno_freefile(); // like FREEFILE
void gfs_fileno_use(int32 f, int32 i);
int32 gfs_open(qbs *filename, int32 access, int32 restrictions, int32 how);
int32 gfs_close(int32 i);
int64 gfs_lof(int32 i);
int32 gfs_setpos(int32 i, int64 position);
int32 gfs_write(int32 i, int64 position, uint8 *data, int64 size);
int32 gfs_read(int32 i, int64 position, uint8 *data, int64 size);
int64 gfs_read_bytes();

extern uint8 cmem[1114099]; // 16*65535+65535+3 (enough for highest referencable dword in conv memory)

// keyhit cyclic buffer
extern int64 keyhit[8192];
//    keyhit specific internal flags: (stored in high 32-bits)
//    &4294967296->numpad was used
extern int32 keyhit_nextfree;
extern int32 keyhit_next;
// note: if full, the oldest message is discarded to make way for the new message

extern uint8 port60h_event[256];
extern int32 port60h_events;

extern int32 window_exists;
extern int32 no_control_characters2;

#endif
