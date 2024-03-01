
#include "libqb-common.h"

#include <stdlib.h>
#include <string.h>
#include <fstream>

#include "filepath.h"
#include "gfs.h"

static int64_t gfs_nextid = 1;

static gfs_file_struct *gfs_file = (gfs_file_struct *)malloc(1);

static int32_t gfs_n = 0;
static int32_t gfs_freed_n = 0;
static int32_t *gfs_freed = (int32_t *)malloc(1);
static int32_t gfs_freed_size = 0;

static int32_t *gfs_fileno = (int32_t *)malloc(1);
static int32_t gfs_fileno_n = 0;

int32_t gfs_get_fileno(int file_number) {
    return gfs_fileno[file_number];
}

gfs_file_struct *gfs_get_file_struct(int fileno) {
    return gfs_file + fileno;
}

void gfs_close_all_files() {
    for (int32_t i = 1; i <= gfs_fileno_n; i++) {
        if (gfs_fileno_valid(i) == 1)
            gfs_close(gfs_get_fileno(i));
    }
}

int32_t gfs_new() {
    int32_t i;
    if (gfs_freed_n) {
        i = gfs_freed[--gfs_freed_n];
    } else {
        i = gfs_n;
        gfs_n++;
        gfs_file = (gfs_file_struct *)realloc(gfs_file, gfs_n * sizeof(gfs_file_struct));
    }
    memset(&gfs_file[i], 0, sizeof(gfs_file_struct));
    gfs_file[i].id = gfs_nextid++;
    return i;
}

int32_t gfs_validhandle(int32_t i) {
    if ((i < 0) || (i >= gfs_n))
        return 0;
    if (gfs_file[i].scrn)
        return 1;
    if (gfs_file[i].open)
        return 1;
    return 0;
}

int32_t gfs_fileno_valid(int32_t f) {
    // returns: -2   invalid handle
    //          1   in use
    //          0   unused

    if (f <= 0)
        return -2;
    if (f <= gfs_fileno_n) {
        if (gfs_fileno[f] == -1)
            return 0;
        else
            return 1;
    }
    gfs_fileno = (int32_t *)realloc(gfs_fileno, (f + 1) * 4);
    memset(gfs_fileno + gfs_fileno_n + 1, -1, (f - gfs_fileno_n) * 4);
    gfs_fileno_n = f;
    return 0;
}

int32_t gfs_fileno_freefile() { // like FREEFILE
    // note: for QBASIC compatibility the lowest available file number is returned
    int32_t x;
    for (x = 1; x <= gfs_fileno_n; x++)
        if (gfs_fileno[x] == -1)
            return x;
    return gfs_fileno_n + 1;
}

void gfs_fileno_use(int32_t f, int32_t i) {
    // assumes valid handles
    gfs_fileno[f] = i;
    gfs_file[i].fileno = f;
}

void gfs_fileno_free(int32_t f) { // note: called by gfs_free (DO NOT CALL THIS FUNCTION)
    gfs_fileno[f] = -1;
}

int32_t gfs_free(int32_t i) {

    if (!gfs_validhandle(i))
        return -2; // invalid handle
    if (gfs_freed_size <= gfs_freed_n) {
        gfs_freed_size++;
        gfs_freed = (int32_t *)realloc(gfs_freed, gfs_freed_size * 4);
    }

    gfs_file[i].open = 0;
    if (gfs_file[i].fileno)
        gfs_fileno_free(gfs_file[i].fileno);
    gfs_freed[gfs_freed_n++] = i;
    return 0;
}

int32_t gfs_close(int32_t i) {
    int32_t x;
    if ((x = gfs_free(i)))
        return x;

    if (gfs_file[i].scrn)
        return 0; // No further action needed
    if (gfs_file[i].field_buffer) {
        free(gfs_file[i].field_buffer);
        gfs_file[i].field_buffer = NULL;
    }
    if (gfs_file[i].field_strings) {
        free(gfs_file[i].field_strings);
        gfs_file[i].field_strings = NULL;
    }

#ifdef GFS_C
    gfs_file_struct *f = &gfs_file[i];
    f->file_handle->close();
    delete f->file_handle;
    return 0;
#endif

#ifdef GFS_WINDOWS
    gfs_file_struct *f = &gfs_file[i];
    CloseHandle(f->win_handle);
    return 0;
#endif

    return -1;
}

int64_t gfs_lof(int32_t i) {
    if (!gfs_validhandle(i))
        return -2; // invalid handle
    gfs_file_struct *f = &gfs_file[i];
    if (f->scrn)
        return -4;
#ifdef GFS_C
    f->file_handle->clear();
    if (f->read) {
        int64_t bytes;
        f->file_handle->seekg(0, std::ios::end);
        bytes = f->file_handle->tellg();
        f->file_handle->seekg(f->pos);
        return bytes;
    }
    if (f->write) {
        int64_t bytes;
        f->file_handle->seekp(0, std::ios::end);
        bytes = f->file_handle->tellp();
        f->file_handle->seekp(f->pos);
        return bytes;
    }
    return -1;
#endif

#ifdef GFS_WINDOWS
    int64_t bytes;
    *((int32_t *)&bytes) = GetFileSize(f->win_handle, (DWORD *)(((int32_t *)&bytes) + 1));
    if ((bytes & 0xFFFFFFFF) == 0xFFFFFFFF) {
        if (GetLastError() != NO_ERROR)
            return -3; // bad/incorrect file mode
    }
    return bytes;
#endif

    return -1;
}

int32_t gfs_open_com_syntax(qbs *fstr, gfs_file_struct *f) {
    // 0=not an open com statement
    //-1=syntax error
    // 1=valid
    // check if filename is a COM open command

    f->com_port = 0;

    if (fstr->len <= 3 || (fstr->chr[0] & 223) != 67 || (fstr->chr[1] & 223) != 79 || (fstr->chr[2] & 223) != 77)
        return 0; // ! C/c,O/o,M/m

    int32_t c, i, v = -1, x = 0;
    for (i = 3; i < fstr->len - 1; i++) {
        c = fstr->chr[i];
        if (c == ':')
            goto comstatment;
        if ((c < '0') || (c > '9'))
            return 0; // not 0-9
        if (v == -1) {
            if (c == '0')
                return 0; // first digit 0
            v = 0;
        }
        v = v * 10 + (c - '0');
    }

    return 0; // no ":"

comstatment:
    if (x >= 7 || v <= 0 || v > 255)
        return -1; // invalid port number (1-255)

    f->com_port = v;

    // COM open confirmed
    static qbs *str = nullptr;

    if (!str)
        str = qbs_new(0, 0);

    qbs_set(str, qbs_ucase(fstr));
    str->len--; // remove null term.

    // set option values to uninitialized
    //---group 1
    f->com_baud_rate = -1;
    f->com_parity = -1;
    f->com_data_bits_per_byte = -1;
    f->com_stop_bits = -1;
    //---group 2
    f->com_rs = -1;
    f->com_bin_asc = -1;
    f->com_asc_lf = -1;
    f->com_cd_x = -1;
    f->com_cs_x = -1;
    f->com_ds_x = -1;
    f->com_op_x = -1;

    int32_t str_or_num = 1;
    int64_t strv = 0;
    int32_t stage = 1;
    int32_t com_rb_used = 0;
    int32_t com_tb_used = 0;

    v = -1;
    for (i = i + 1; i < str->len; i++) {
        c = str->chr[i];

        if (c != ',') {
            if ((c < '0') || ((c > '9') && (c < 'A')) || (c > 'Z'))
                return -1; // invalid character

            if ((str_or_num == 2) && (c >= 'A'))
                return -1; // invalid character

            if (c < 'A')
                str_or_num = 2; // ABC->123

            if ((str_or_num == 1) || (stage == 4)) { // note: stage 4 is interpreted as a string
                if (strv & 0xFF0000)
                    strv = strv | (c << 24);
                else if (strv & 0x00FF00)
                    strv = strv | (c << 16);
                else if (strv & 0x0000FF)
                    strv = strv | (c << 8);
                else
                    strv = c;

                if (strv > 16777216)
                    return -1; // string option too long (max 3 characters)

            } else {
                if ((c > '0') && (c <= '9')) {
                    if (v == -2)
                        return -1; // leading 0s are invalid

                    if (v == -1)
                        v = 0;

                    v = v * 10 + (c - '0');
                } else { // 0
                    if (v == -2)
                        return -1; // leading 0s are invalid

                    if (v == -1)
                        v = -2; // 0...

                    if (v > 0)
                        v = v * 10;
                }
                if (v > 2147483647)
                    return -1; // numeric value too large (LONG values only)
            }
        } // c!=44

        if ((c == ',') || (i == str->len - 1)) {
            if (v == -2)
                v = 0;

            // note: v==-1 means omit
            if (stage == 1) {
                if (f->com_baud_rate != -1)
                    return -1;

                if (strv)
                    return -1;

                if (v == 0)
                    return -1;

                if (v == -1)
                    v = 300;

                f->com_baud_rate = v;
                stage++;

                goto done_stage;
            } else if (stage == 2) {
                if (f->com_parity != -1)
                    return -1;

                if (v != -1)
                    return -1;

                x = -1;

                if (strv == 78)
                    x = 0; // N

                if (strv == 0)
                    x = 1; // E*

                if (strv == 69)
                    x = 1; // E

                if (strv == 79)
                    x = 2; // O

                if (strv == 83)
                    x = 3; // S

                if (strv == 77)
                    x = 4; // M

                if (strv == 17744)
                    x = 5; // PE

                if (x == -1)
                    return -1;

                f->com_parity = x;
                stage++;

                goto done_stage;
            } else if (stage == 3) {
                if (f->com_data_bits_per_byte != -1)
                    return -1;

                if (strv)
                    return -1;

                x = -1;

                if (v == -1)
                    x = 7;

                if (v == 5)
                    x = 5;

                if (v == 6)
                    x = 6;

                if (v == 7)
                    x = 7;

                if (v == 8)
                    x = 8;

                if (x == -1)
                    return -1;

                f->com_data_bits_per_byte = x;
                stage++;

                goto done_stage;
            } else if (stage == 4) {
                if (f->com_stop_bits != -1)
                    return -1;

                if (v != -1)
                    return -1;

                x = -1;
                if (strv == 0) {
                    x = 10;
                    if (f->com_baud_rate <= 110) {
                        x = 20;
                        if (f->com_data_bits_per_byte == 5)
                            x = 15;
                    }
                } // 0

                if (strv == 49)
                    x = 10; //"1"

                if (strv == 3485233)
                    x = 15; //"1.5"

                if (strv == 50)
                    x = 20; //"2"

                if (x == -1)
                    return -1;

                f->com_stop_bits = x;
                stage++;
                goto done_stage;
            }

            // stage > 4
            if (!strv)
                return -1; // all options after 4 require a string

            if (strv == 21330) {
                if (f->com_rs != -1)
                    return -1; // RS

                f->com_rs = 1;
                goto done_stage;
            }

            if (strv == 5130562) {
                if (f->com_bin_asc != -1)
                    return -1; // BIN

                f->com_bin_asc = 0;
                goto done_stage;
            }

            if (strv == 4412225) {
                if (f->com_bin_asc != -1)
                    return -1; // ASC

                f->com_bin_asc = 1;
                goto done_stage;
            }

            if (strv == 16980) {
                if (com_tb_used)
                    return -1; // TB

                com_tb_used = 1;
                goto done_stage;
            }

            if (strv == 16978) {
                if (com_rb_used)
                    return -1; // RB

                com_rb_used = 1;
                goto done_stage;
            }

            if (strv == 17996) {
                if (f->com_asc_lf != -1)
                    return -1; // LF

                f->com_asc_lf = 1;
                goto done_stage;
            }

            if (strv == 17475) {
                if (f->com_cd_x != -1)
                    return -1; // CD

                if (v == -1)
                    v = 0;

                if (v > 65535)
                    return -1;

                f->com_cd_x = v;
                goto done_stage;
            }

            if (strv == 21315) {
                if (f->com_cs_x != -1)
                    return -1; // CS

                if (v == -1)
                    v = 1000;

                if (v > 65535)
                    return -1;

                f->com_cs_x = v;
                goto done_stage;
            }

            if (strv == 21316) {
                if (f->com_ds_x != -1)
                    return -1; // DS

                if (v == -1)
                    v = 1000;

                if (v > 65535)
                    return -1;

                f->com_ds_x = v;
                goto done_stage;
            }

            if (strv == 20559) {
                if (f->com_op_x != -1)
                    return -1; // OP

                if (v == -1)
                    v = 10000;

                if (v > 65535)
                    return -1;

                f->com_op_x = v;
                goto done_stage;
            }

            return -1; // invalid option

        done_stage:
            str_or_num = 1;
            strv = 0;
            v = -1;
        }
    } // i

    // complete omitted options with defaults
    if (f->com_baud_rate == -1)
        f->com_baud_rate = 300;

    if (f->com_parity == -1)
        f->com_parity = 1;

    if (f->com_data_bits_per_byte == -1)
        f->com_data_bits_per_byte = 7;

    if (f->com_stop_bits == -1) {
        x = 10;
        if (f->com_baud_rate <= 110) {
            x = 20;
            if (f->com_data_bits_per_byte == 5)
                x = 15;
        }

        f->com_stop_bits = x;
    }

    if (f->com_bin_asc == -1)
        f->com_bin_asc = 0;

    if (f->com_asc_lf == -1)
        f->com_asc_lf = 0;

    if (f->com_asc_lf == 1) {
        if (f->com_bin_asc == 0)
            f->com_asc_lf = 0;
    }

    if (f->com_rs == -1)
        f->com_rs = 0;

    if (f->com_cd_x == -1)
        f->com_cd_x = 0;

    if (f->com_cs_x == -1)
        f->com_cs_x = 1000;

    if (f->com_ds_x == -1)
        f->com_ds_x = 1000;

    if (f->com_op_x == -1) {
        x = f->com_cd_x * 10;
        auto z = f->com_ds_x * 10;
        if (z > x)
            x = z;

        if (x > 65535)
            x = 65535;

        f->com_op_x = x;
    }

    return 1; // valid
}

int32_t gfs_open(qbs *filename, int32_t access, int32_t restrictions, int32_t how) {
    // filename - an OS compatible filename (doesn't need to be NULL terminated)
    // access - 1=read, 2=write, 3=read and write
    // restrictions - 1=others cannot read, 2=others cannot write, 3=others cannot read or write(exclusive access)
    // how - 1=create(if it doesn't exist), 2=create(if it doesn't exist) & truncate
    //      3=create(if it doesn't exist)+undefined access[get whatever access is available]
    static int32_t i, x, x2, x3, e;
    static qbs *filenamez = NULL;
    static gfs_file_struct *f;

    if (!filenamez)
        filenamez = qbs_new(0, 0);
    qbs_set(filenamez, qbs_add(filename, qbs_new_txt_len("\0", 1)));

    i = gfs_new();
    f = &gfs_file[i];

    int32_t v1;
    unsigned char *c1 = filename->chr;
    v1 = *c1;
    if (v1 == 83 || v1 == 115) { // S
        c1++;
        v1 = *c1;
        if (v1 == 67 || v1 == 99) { // C
            c1++;
            v1 = *c1;
            if (v1 == 82 || v1 == 114) { // R
                c1++;
                v1 = *c1;
                if (v1 == 78 || v1 == 110) { // N
                    c1++;
                    v1 = *c1;
                    if (v1 == 58) { //:
                        f->scrn = 1;
                        return i;
                    };
                };
            };
        };
    };

    if (access & 1)
        f->read = 1;
    if (access & 2)
        f->write = 1;
    if (restrictions & 1)
        f->lock_read = 1;
    if (restrictions & 2)
        f->lock_write = 1;
    f->pos = 0;

    // check for OPEN COM syntax
    if ((x = gfs_open_com_syntax(filenamez, f))) {
        if (x == -1) {
            gfs_free(i);
            return -11;
        } //-11 bad file name
        // note: each GFS implementation will handle COM communication differently
    }

#ifdef GFS_C
    // note: GFS_C ignores restrictions/locking
    f->file_handle = new std::fstream();
    // attempt as if it exists:
    if (how == 2) {
        // with truncate
        if (access == 1)
            f->file_handle->open(filepath_fix_directory(filenamez), std::ios::in | std::ios::binary | std::ios::trunc);
        if (access == 2)
            f->file_handle->open(filepath_fix_directory(filenamez), std::ios::out | std::ios::binary | std::ios::trunc);
        if (access == 3)
            f->file_handle->open(filepath_fix_directory(filenamez), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    } else {
        // without truncate
        if (access == 1)
            f->file_handle->open(filepath_fix_directory(filenamez), std::ios::in | std::ios::binary);
        if (access == 2)
            f->file_handle->open(filepath_fix_directory(filenamez), std::ios::out | std::ios::binary | std::ios::app);
        if (access == 3)
            f->file_handle->open(filepath_fix_directory(filenamez), std::ios::in | std::ios::out | std::ios::binary);
    }
    if (how) {
        if (!f->file_handle->is_open()) { // couldn't open file, so attempt creation
            f->file_handle_o = new std::ofstream();
            f->file_handle_o->open(filepath_fix_directory(filenamez), std::ios::out);
            if (f->file_handle_o->is_open()) { // created new file
                f->file_handle_o->close();
                // retry open
                f->file_handle->clear();
                if (how == 2) {
                    // with truncate
                    if (access == 1)
                        f->file_handle->open(filepath_fix_directory(filenamez), std::ios::in | std::ios::binary | std::ios::trunc);
                    if (access == 2)
                        f->file_handle->open(filepath_fix_directory(filenamez), std::ios::out | std::ios::binary | std::ios::trunc);
                    if (access == 3)
                        f->file_handle->open(filepath_fix_directory(filenamez), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
                } else {
                    // without truncate
                    if (access == 1)
                        f->file_handle->open(filepath_fix_directory(filenamez), std::ios::in | std::ios::binary);
                    if (access == 2)
                        f->file_handle->open(filepath_fix_directory(filenamez), std::ios::out | std::ios::binary | std::ios::app);
                    if (access == 3)
                        f->file_handle->open(filepath_fix_directory(filenamez), std::ios::in | std::ios::out | std::ios::binary);
                }
            }
            delete f->file_handle_o;
        }
    }                                 // how
    if (!f->file_handle->is_open()) { // couldn't open file
        delete f->file_handle;
        gfs_free(i);
        if (how)
            return -11; // Bad file name
        return -5;      // File not found
    }
    // file opened successfully
    f->open = 1;
    return i;
#endif

#ifdef GFS_WINDOWS
    x = 0;
    if (access & 1)
        x |= GENERIC_READ;
    if (access & 2)
        x |= GENERIC_WRITE;
    x2 = FILE_SHARE_READ | FILE_SHARE_WRITE;
    if (restrictions & 1)
        x2 ^= FILE_SHARE_READ;
    if (restrictions & 2)
        x2 ^= FILE_SHARE_WRITE;

    if (f->com_port) { // open a com port
        static qbs *portname = NULL;
        if (!portname)
            portname = qbs_new(0, 0);
        qbs_set(portname, qbs_add(qbs_new_txt("CO"), qbs_str(f->com_port)));
        qbs_set(portname, qbs_add(portname, qbs_new_txt_len(":\0", 2)));
        portname->chr[2] = 77; // replace " " with "M"
        f->win_handle= CreateFile((char *)portname->chr, x, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (f->win_handle== INVALID_HANDLE_VALUE) {
            gfs_free(i);
            return -8;
        } // device unavailable
        static DCB cs;
        ZeroMemory(&cs, sizeof(DCB));
        cs.DCBlength = sizeof(DCB);
        if (!GetCommState(f->win_handle, &cs)) {
            CloseHandle(f->win_handle);
            gfs_free(i);
            return -8;
        } // device unavailable
        static COMMTIMEOUTS ct;
        ZeroMemory(&ct, sizeof(COMMTIMEOUTS));

        cs.BaudRate = f->com_baud_rate;
        x = f->com_stop_bits;
        if (x == 10)
            x2 = 0;
        if (x == 15)
            x2 = 1;
        if (x == 20)
            x2 = 2;
        cs.StopBits = x2;
        cs.ByteSize = f->com_data_bits_per_byte;
        x = f->com_parity;
        if (x == 0)
            x2 = 0;
        if (x == 1)
            x2 = 2;
        if (x == 2)
            x2 = 1;
        if (x == 3)
            x2 = 4;
        if (x == 4)
            x2 = 3;
        // if (x==5) x2=... ***"PE" will be supported later***
        // 0-4=None,Odd,Even,Mark,Space
        cs.Parity = x2;
        if (x2 == 0)
            cs.fParity = 0;
        else
            cs.fParity = 1;
        if (f->com_rs)
            cs.fRtsControl = RTS_CONTROL_DISABLE;
        if (f->com_bin_asc == 0)
            cs.fBinary = 1;
        else
            cs.fBinary = 0;
        cs.EofChar = 26;
        if (!SetCommState(f->win_handle, &cs)) {
            CloseHandle(f->win_handle);
            gfs_free(i);
            return -8;
        } // device unavailable
        if (f->com_ds_x == 0) {
            // A value of MAXDWORD, combined with zero values for both the ReadTotalTimeoutConstant and ReadTotalTimeoutMultiplier members, specifies that the
            // read operation is to return immediately with the characters that have already been received, even if no characters have been received.
            ct.ReadIntervalTimeout = MAXDWORD;
            ct.ReadTotalTimeoutMultiplier = 0;
            ct.ReadTotalTimeoutConstant = 0;
        } else {
            ct.ReadIntervalTimeout = 0;
            ct.ReadTotalTimeoutMultiplier = 0;
            ct.ReadTotalTimeoutConstant = f->com_ds_x;
        }
        ct.WriteTotalTimeoutMultiplier = 0;
        ct.WriteTotalTimeoutConstant = f->com_cs_x;
        if (!SetCommTimeouts(f->win_handle, &ct)) {
            CloseHandle(f->win_handle);
            gfs_free(i);
            return -8;
        } // device unavailable
        f->open = 1;
        return i;
    }

    /*
        #define CREATE_NEW          1
        #define CREATE_ALWAYS       2
        #define OPEN_EXISTING       3
        #define OPEN_ALWAYS         4
        #define TRUNCATE_EXISTING   5
    */
    x3 = OPEN_EXISTING;
    if (how)
        x3 = OPEN_ALWAYS;
undefined_retry:
    f->win_handle = CreateFile(filepath_fix_directory(filenamez), x, x2, NULL, x3, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f->win_handle == INVALID_HANDLE_VALUE) {

        if (how == 3) {
            // attempt read access only
            x = GENERIC_READ;
            f->read = 1;
            f->write = 0;
            how++;
            goto undefined_retry;
        }

        if (how == 4) {
            // attempt write access only
            x = GENERIC_WRITE;
            f->read = 0;
            f->write = 1;
            how++;
            goto undefined_retry;
        }

        gfs_free(i);
        e = GetLastError();
        if (e == 3)
            return -6;
        if ((e == 4) || (e == 29) || (e == 30))
            return -9;
        if ((e == 5) || (e == 19) || (e == 33) || (e == 32))
            return -7;
        if ((e == 15) || (e == 21))
            return -8;
        if (e == 2)
            return -5;
        // showvalue(e);
        return -5; // assume (2)
    }              // invalid handle

    if (how == 2) {
        // truncate file if size is not 0
        static DWORD GetFileSize_low, GetFileSize_high;
        GetFileSize_low = GetFileSize(f->win_handle, &GetFileSize_high);
        if (GetFileSize_low || GetFileSize_high) {
            CloseHandle(f->win_handle);
            x3 = TRUNCATE_EXISTING;
            f->win_handle = CreateFile(filepath_fix_directory(filenamez), x, x2, NULL, x3, FILE_ATTRIBUTE_NORMAL, NULL);

            if (f->win_handle == INVALID_HANDLE_VALUE) {
                gfs_free(i);
                e = GetLastError();
                if (e == 3)
                    return -6;
                if ((e == 4) || (e == 29) || (e == 30))
                    return -9;
                if ((e == 5) || (e == 19) || (e == 33) || (e == 32))
                    return -7;
                if ((e == 15) || (e == 21))
                    return -8;
                if (e == 2)
                    return -5;
                // showvalue(e);
                return -5; // assume (2)
            }              // invalid handle
        }
    } // how==2
    f->open = 1;
    return i;
#endif

    return -1; // non-specific fail
}

int32_t gfs_setpos(int32_t i, int64_t position) {
    if (!gfs_validhandle(i))
        return -2; // invalid handle
    if (position < 0)
        return -4; // illegal function call

    static gfs_file_struct *f;
    f = &gfs_file[i];

#ifdef GFS_C
    if (f->read) {
        f->file_handle->clear();
        f->file_handle->seekg(position);
    }
    if (f->write) {
        f->file_handle->clear();
        f->file_handle->seekp(position);
    }
    f->pos = position;
    if (f->pos <= gfs_lof(i)) {
        f->eof_passed = 0;
        f->eof_reached = 0;
    }
    return 0;
#endif

#ifdef GFS_WINDOWS
    if (SetFilePointer(f->win_handle, (int32_t)position, (long *)(((int32_t *)&position) + 1), FILE_BEGIN) ==
        0xFFFFFFFF) { /*Note that it is not an error to set the file pointer to a position beyond the end of the file. The size of the file does not increase
                         until you call the SetEndOfFile, WriteFile, or WriteFileEx function.*/
        if (GetLastError() != NO_ERROR) {
            return -3; // bad file mode
        }
    }
    f->pos = position;
    if (f->pos <= gfs_lof(i)) {
        f->eof_passed = 0;
        f->eof_reached = 0;
    }
    return 0;
#endif

    return -1;
}

int64_t gfs_getpos(int32_t i) {
    if (!gfs_validhandle(i))
        return -2; // invalid handle
    static gfs_file_struct *f;
    f = &gfs_file[i];
    return f->pos;
}

int32_t gfs_write(int32_t i, int64_t position, uint8_t *data, int64_t size) {
    if (!gfs_validhandle(i))
        return -2; // invalid handle
    static int32_t e;
    static gfs_file_struct *f;
    f = &gfs_file[i];
    if (!f->write)
        return -3; // bad file mode
    if (size < 0)
        return -4; // illegal function call
    static int32_t x;
    if (position != -1) {
        if ((x = gfs_setpos(i, position)))
            return x; //(pass on error)
    }

#ifdef GFS_C
    f->file_handle->clear();
    f->file_handle->write((char *)data, size);
    if (f->file_handle->bad()) {
        return -7; // assume: permission denied
    }
    f->pos += size;
    return 0;
#endif

#ifdef GFS_WINDOWS
    static uint32_t size2;
    static int64_t written = 0;
    while (size) {
        if (size > 4294967295) {
            size2 = 4294967295;
            size -= 4294967295;
        } else {
            size2 = size;
            size = 0;
        }
        if (!WriteFile(f->win_handle, data, size2, (unsigned long *)&written, NULL)) {
            e = GetLastError();
            if ((e == 5) || (e == 33))
                return -7; // permission denied
            return -9;     // assume: path/file access error
        }
        data += written;
        f->pos += written;
        if (written != size2)
            return -1;
    }
    return 0;
#endif

    return -1;
}


int64_t gfs_read_bytes_value;
int64_t gfs_read_bytes() { return gfs_read_bytes_value; }

int32_t gfs_read(int32_t i, int64_t position, uint8_t *data, int64_t size) {
    gfs_read_bytes_value = 0;
    if (!gfs_validhandle(i))
        return -2; // invalid handle
    static int32_t e;
    static gfs_file_struct *f;
    f = &gfs_file[i];
    if (!f->read)
        return -3; // bad file mode
    if (size < 0)
        return -4; // illegal function call
    static int32_t x;
    if (position != -1) {
        if ((x = gfs_setpos(i, position)))
            return x; //(pass on error)
    }

#ifdef GFS_C
    f->file_handle->clear();
    f->file_handle->read((char *)data, size);
    if (f->file_handle->bad()) { // note: 'eof' also sets the 'fail' flag, so only the 'bad' flag is checked
        return -7;               // assume: permission denied
    }
    static int64_t bytesread;
    bytesread = f->file_handle->gcount();
    gfs_read_bytes_value = bytesread;
    f->pos += bytesread;
    if (bytesread < size) {
        memset(data + bytesread, 0, size - bytesread);
        f->eof_passed = 1;
        return -10;
    }
    f->eof_passed = 0;
    return 0;
#endif

#ifdef GFS_WINDOWS
    static uint32_t size2;
    static int64_t bytesread = 0;
    while (size) {
        if (size > 4294967295) {
            size2 = 4294967295;
            size -= 4294967295;
        } else {
            size2 = size;
            size = 0;
        }

        if (ReadFile(f->win_handle, data, size2, (unsigned long *)&bytesread, NULL)) {
            data += bytesread;
            f->pos += bytesread;
            gfs_read_bytes_value += bytesread;
            if (bytesread != size2) {
                ZeroMemory(data, size + (size2 - bytesread)); // nullify remaining buffer
                f->eof_passed = 1;
                return -10;
            } // eof passed
        } else {
            // error
            e = GetLastError();
            if ((e == 5) || (e == 33))
                return -7; // permission denied
            // showvalue(e);
            return -9; // assume: path/file access error
        }
    }
    f->eof_passed = 0;
    return 0;
#endif

    return -1;
}

int32_t gfs_eof_reached(int32_t i) {
    if (!gfs_validhandle(i))
        return -2; // invalid handle
    if (gfs_getpos(i) >= gfs_lof(i))
        return 1;
    return 0;
}

int32_t gfs_eof_passed(int32_t i) {
    if (!gfs_validhandle(i))
        return -2; // invalid handle
    static gfs_file_struct *f;
    f = &gfs_file[i];
    if (f->eof_passed)
        return 1;
    return 0;
}

int32_t gfs_lock(int32_t i, int64_t offset_start, int64_t offset_end) {
    // if offset_start==-1, 'from beginning' (typically offset 0) is assumed
    // if offset_end==-1, 'to end/infinity' is assumed
    // range is inclusive of start and end
    if (!gfs_validhandle(i))
        return -2; // invalid handle
    static gfs_file_struct *f;
    f = &gfs_file[i];

    if (offset_start == -1)
        offset_start = 0;
    if (offset_start < 0)
        return -4; // illegal function call
    if (offset_end < -1)
        return -4; // illegal function call
        // note: -1 equates to highest uint64 value (infinity)
        //      All other negative end values are illegal

#ifdef GFS_C
    return 0;
#endif

#ifdef GFS_WINDOWS
    int64_t bytes;
    bytes = offset_end;
    if (bytes != -1)
        bytes = bytes - offset_start + 1;
    if (!LockFile(f->win_handle, *((DWORD *)(&offset_start)), *(((DWORD *)(&offset_start)) + 1), *((DWORD *)(&bytes)), *(((DWORD *)(&bytes)) + 1))) {
        // failed
        static int32_t e;
        e = GetLastError();
        if ((e == 5) || (e == 33))
            return -7; // permission denied
        // showvalue(e);
        return -9; // assume: path/file access error
    }
    return 0;
#endif

    return -1;
}

int32_t gfs_unlock(int32_t i, int64_t offset_start, int64_t offset_end) {
    // if offset_start==-1, 'from beginning' (typically offset 0) is assumed
    // if offset_end==-1, 'to end/infinity' is assumed
    // range is inclusive of start and end
    if (!gfs_validhandle(i))
        return -2; // invalid handle
    static gfs_file_struct *f;
    f = &gfs_file[i];

    if (offset_start == -1)
        offset_start = 0;
    if (offset_start < 0)
        return -4; // illegal function call
    if (offset_end < -1)
        return -4; // illegal function call
        // note: -1 equates to highest uint64 value (infinity)
        //      All other negative end values are illegal

#ifdef GFS_C
    return 0;
#endif

#ifdef GFS_WINDOWS
    int64_t bytes;
    bytes = offset_end;
    if (bytes != -1)
        bytes = bytes - offset_start + 1;
    if (!UnlockFile(f->win_handle, *((DWORD *)(&offset_start)), *(((DWORD *)(&offset_start)) + 1), *((DWORD *)(&bytes)), *(((DWORD *)(&bytes)) + 1))) {
        // failed
        static int32_t e;
        e = GetLastError();
        if ((e == 5) || (e == 33) || (e == 158))
            return -7; // permission denied
        // showvalue(e);
        return -9; // assume: path/file access error
    }
    return 0;
#endif

    return -1;
}

