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

const int32_t scrn = 2;
int32_t gfs_eof_passed(int32_t i);
int32_t gfs_eof_reached(int32_t i);
int64_t gfs_getpos(int32_t i);
int32_t gfs_fileno_valid(int32_t f);
int32_t gfs_fileno_freefile(); // like FREEFILE
int32_t gfs_current_lowest_freefile;

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

void gfs_close_all_files();

