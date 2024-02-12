#pragma once

#include <stdint.h>
#include <iostream>

#include "qbs.h"

#ifdef QB64_WINDOWS
#    define GFS_WINDOWS

#    include <wtypes.h>
#endif

#ifndef GFS_WINDOWS
#    define GFS_C
#endif


/* Generic File System (GFS)
    GFS allows OS specific access whilst still maintaining 'pure' C-based routines for
    multiplatform compatibility. 'Pure' C-based routines may not allow certain functionality,
    such as partial file locking.
    GFS handles/indexes are independent of QB64 handles/indexes to allow for internal files
    to be open but not interfere with the QB64 file handle numbers.

    GFS error codes:
    -1 non-specific fail
    -2 invalid handle
    -3 bad/incorrect file mode
    -4 illegal function call (input is out of range)
    -5 file not found (win:2)
    -6 path not found (win:3)
    -7 access/permission denied (win:5,19)
    -8 device unavailable/drive invalid (win:15,21)
    -9 path/file access error
    -10 read past eof
    -11 bad file name
*/

struct gfs_file_struct { // info applicable to all files
    int64_t id;            // a unique ID given to all files (currently only referenced by the FIELD statement to remove old field conditions)
    uint8_t open;
    uint8_t read;
    uint8_t write;
    uint8_t lock_read;
    uint8_t lock_write;
    int64_t pos;           //-1=unknown
    uint8_t eof_reached;   // read last character of file (set/reset by gfs_read only)
    uint8_t eof_passed;    // attempted to read past eof (set/reset by gfs_read only)
    int32_t fileno;        // link to fileno index
    uint8_t type;          // qb access method (1=RANDOM,2=BINARY,3=INPUT,4=OUTPUT)
    int64_t record_length; // used by RANDOM
    uint8_t *field_buffer;
    qbs **field_strings;   // list of qbs pointers linked to this file
    int32_t field_strings_n; // number of linked strings
    int64_t column;          // used by OUTPUT/APPEND to tab correctly (base 0)
#ifdef GFS_C
    // GFS_C data follows: (unused by custom GFS interfaces)
    std::fstream *file_handle;
    std::ofstream *file_handle_o;
#endif
#ifdef GFS_WINDOWS
    HANDLE win_handle;
#endif
    // COM port data follows (*=default)
    uint8_t com_port;              // 0=not a com port
    int32_t com_baud_rate;         //(bits per second)75,110,150,300*,600,1200,1800,2400,9600,?
    int8_t com_parity;             //[0]N,[1]E*,[2]O,[3]S,[4]M,[5]PE(none,even*,odd,space,mark,error-checking)
    int8_t com_data_bits_per_byte; // 5,6,7*,8
    int8_t com_stop_bits;          //[10]1,[15]1.5,[20]2
    // The default value is 1 for baud rates greater than 110. For
    // baud rates less than or equal to 110, the default value is
    // 1.5 when data is 5; otherwise, the value is 2.
    int8_t com_bin_asc; //[0]=BIN*,[1]=ASC
    int8_t com_asc_lf;  //[0]omit*,[1]LF(only valid with ASC)
    // note: rb_x and tb_x are ignored by QB64 (receive and transmit buffer sizes)
    int8_t com_rs;    //[0]detect*,[1]dont-detect
    int32_t com_cd_x; // 0*-65535
    int32_t com_cs_x; // 1000*,0-65535
    int32_t com_ds_x; // 1000*,0-65535
    int32_t com_op_x;
    //                 OP not used:          x omitted:     x specified:
    //                 10 times the CD or    10000 ms       0 - 65,535 milliseconds
    //                 DS timeout value,
    //                 whichever is greater

    // SCRN: support follows
    uint8_t scrn; // 0 = not a file opened as "SCRN:"
};

int32_t gfs_eof_passed(int32_t i);
int32_t gfs_eof_reached(int32_t i);
int64_t gfs_getpos(int32_t i);
int32_t gfs_fileno_valid(int32_t f);
int32_t gfs_fileno_freefile(); // like FREEFILE
void gfs_fileno_use(int32_t f, int32_t i);
int32_t gfs_open(qbs *filename, int32_t access, int32_t restrictions, int32_t how);
int32_t gfs_close(int32_t i);
int64_t gfs_lof(int32_t i);
int32_t gfs_setpos(int32_t i, int64_t position);
int32_t gfs_write(int32_t i, int64_t position, uint8_t *data, int64_t size);
int32_t gfs_read(int32_t i, int64_t position, uint8_t *data, int64_t size);
int64_t gfs_read_bytes();

int32_t gfs_lock(int32_t i, int64_t offset_start, int64_t offset_end);
int32_t gfs_unlock(int32_t i, int64_t offset_start, int64_t offset_end);

int32_t gfs_get_fileno(int file_number);
gfs_file_struct *gfs_get_file_struct(int fileno);

void gfs_close_all_files();

