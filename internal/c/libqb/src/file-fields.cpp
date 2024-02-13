
#include "libqb-common.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "error_handle.h"
#include "qbs.h"
#include "gfs.h"
#include "file-fields.h"

static int32_t field_failed = 1;
static int32_t field_fileno;
static int32_t field_totalsize;
static int32_t field_maxsize;

void field_new(int32_t fileno) {
    field_failed = 1;
    if (is_error_pending())
        return;
    // validate file
    static int32_t i;
    static gfs_file_struct *gfs;
    i = fileno;
    if (i < 0) {
        error(54);
        return;
    } // bad file mode (TCP/IP exclusion)
    if (gfs_fileno_valid(i) != 1) {
        error(52);
        return;
    }                  // Bad file name or number
    i = gfs_get_fileno(i); // convert fileno to gfs index
    gfs = gfs_get_file_struct(i);
    if (gfs->type != 1) {
        error(54);
        return;
    } // Bad file mode (note: must have RANDOM access)
    // set global variables for field_add
    field_fileno = fileno;
    field_totalsize = 0;
    field_maxsize = gfs->record_length;
    field_failed = 0;
    return;
}

void field_update(int32_t fileno) {

    // validate file
    static int32_t i;
    static gfs_file_struct *gfs;
    i = fileno;
    if (i < 0) {
        exit(7701);
    } // bad file mode (TCP/IP exclusion)
    if (gfs_fileno_valid(i) != 1) {
        exit(7702);
    }                  // Bad file name or number
    i = gfs_get_fileno(i); // convert fileno to gfs index
    gfs = gfs_get_file_struct(i);
    if (gfs->type != 1) {
        exit(7703);
    } // Bad file mode (note: must have RANDOM access)

    static qbs *str;
    for (i = 0; i < gfs->field_strings_n; i++) {
        str = gfs->field_strings[i];
        if (!str)
            exit(7704);

        // fix length if necessary
        if (str->len != str->field->size) {
            if (str->len > str->field->size)
                str->len = str->field->size;
            else
                qbs_set(str, qbs_new(str->field->size, 1));
        }

        // copy data from field into string
        memmove(str->chr, gfs->field_buffer + str->field->offset, str->field->size);

    } // i
}

void lrset_field(qbs *str) {
    // validate file
    static int32_t i;
    static gfs_file_struct *gfs;
    i = str->field->fileno;
    if (gfs_fileno_valid(i) != 1)
        goto remove;
    i = gfs_get_fileno(i); // convert fileno to gfs index

    gfs = gfs_get_file_struct(i);
    if (gfs->type != 1)
        goto remove;
    // check file ID
    if (gfs->id != str->field->fileid)
        goto remove;

    // store in field buffer, padding with spaces or truncating data if necessary
    if (str->field->size <= str->len) {

        memmove(gfs->field_buffer + str->field->offset, str->chr, str->field->size);
    } else {
        memmove(gfs->field_buffer + str->field->offset, str->chr, str->len);
        memset(gfs->field_buffer + str->field->offset + str->len, 32, str->field->size - str->len);
    }

    // update field strings for this file
    field_update(str->field->fileno);

    return;
remove:;
    free(str->field);
    str->field = NULL;
}

void field_free(qbs *str) {

    // validate file
    static int32_t i;
    static gfs_file_struct *gfs;
    i = str->field->fileno;
    if (gfs_fileno_valid(i) != 1)
        goto remove;
    i = gfs_get_fileno(i); // convert fileno to gfs index
    gfs = gfs_get_file_struct(i);
    if (gfs->type != 1)
        goto remove;
    // check file ID
    if (gfs->id != str->field->fileid)
        goto remove;

    // remove from string list
    static qbs *str2;
    for (i = 0; i < gfs->field_strings_n; i++) {
        str2 = gfs->field_strings[i];
        if (str == str2) { // match found
            // truncate list
            memmove(&(gfs->field_strings[i]), &(gfs->field_strings[i + 1]), (gfs->field_strings_n - i - 1) * sizeof(void *));
            goto remove;
        }
    } // i

remove:
    free(str->field);
    str->field = NULL;
}

void field_add(qbs *str, int64_t size) {
    if (field_failed)
        return;
    if (is_error_pending())
        goto fail;
    if (size < 0) {
        error(5);
        goto fail;
    }
    if ((field_totalsize + size) > field_maxsize) {
        error(50);
        goto fail;
    }

    // revalidate file
    static int32_t i;
    static gfs_file_struct *gfs;
    i = field_fileno;
    // TCP/IP exclusion (reason: multi-reading from same TCP/IP position would require a more complex implementation)
    if (i < 0) {
        error(54);
        goto fail;
    } // bad file mode
    if (gfs_fileno_valid(i) != 1) {
        error(52);
        goto fail;
    }                  // Bad file name or number
    i = gfs_get_fileno(i); // convert fileno to gfs index
    gfs = gfs_get_file_struct(i);
    if (gfs->type != 1) {
        error(54);
        goto fail;
    } // Bad file mode (note: must have RANDOM access)

    // 1) Remove str from any previous FIELD allocations
    if (str->field)
        field_free(str);

    // 2) Setup qbs field info
    str->field = (qbs_field *)malloc(sizeof(qbs_field));
    str->field->fileno = field_fileno;
    str->field->fileid = gfs->id;
    str->field->size = size;
    str->field->offset = field_totalsize;

    // 3) Add str to qbs list of gfs
    if (!gfs->field_strings) {
        gfs->field_strings_n = 1;
        gfs->field_strings = (qbs **)malloc(sizeof(qbs **));
        gfs->field_strings[0] = str;
    } else {
        gfs->field_strings_n++;
        gfs->field_strings = (qbs **)realloc(gfs->field_strings, sizeof(qbs **) * gfs->field_strings_n);
        gfs->field_strings[gfs->field_strings_n - 1] = str;
    }

    // 4) Update field strings for this file
    field_update(field_fileno);

    field_totalsize += size;
    return;
fail:
    field_failed = 1;
    return;
}

void field_get(int32_t fileno, int64_t offset, int32_t passed) {
    if (is_error_pending())
        return;

    // validate file
    static int32_t i;
    static gfs_file_struct *gfs;
    i = fileno;
    if (i < 0) {
        error(54);
        return;
    } // bad file mode (TCP/IP exclusion)
    if (gfs_fileno_valid(i) != 1) {
        error(52);
        return;
    }                  // Bad file name or number
    i = gfs_get_fileno(i); // convert fileno to gfs index
    gfs = gfs_get_file_struct(i);
    if (gfs->type != 1) {
        error(54);
        return;
    } // Bad file mode (note: must have RANDOM access)

    if (!gfs->read) {
        error(75);
        return;
    } // Path/file access error

    if (passed) {
        offset--;
        if (offset < 0) {
            error(63);
            return;
        } // Bad record number
        offset *= gfs->record_length;
    } else {
        offset = -1;
    }

    static int32_t e;
    e = gfs_read(i, offset, gfs->field_buffer, gfs->record_length);
    if (e) {
        if (e != -10) { // note: on eof, unread buffer area becomes NULL
            if (e == -2) {
                error(258);
                return;
            } // invalid handle
            if (e == -3) {
                error(54);
                return;
            } // bad file mode
            if (e == -4) {
                error(5);
                return;
            } // illegal function call
            if (e == -7) {
                error(70);
                return;
            } // permission denied
            error(75);
            return; // assume[-9]: path/file access error
        }
    }

    field_update(fileno);
}

void field_put(int32_t fileno, int64_t offset, int32_t passed) {
    if (is_error_pending())
        return;

    // validate file
    static int32_t i;
    static gfs_file_struct *gfs;
    i = fileno;
    if (i < 0) {
        error(54);
        return;
    } // bad file mode (TCP/IP exclusion)
    if (gfs_fileno_valid(i) != 1) {
        error(52);
        return;
    }                  // Bad file name or number
    i = gfs_get_fileno(i); // convert fileno to gfs index
    gfs = gfs_get_file_struct(i);
    if (gfs->type != 1) {
        error(54);
        return;
    } // Bad file mode (note: must have RANDOM access)

    if (!gfs->write) {
        error(75);
        return;
    } // Path/file access error

    if (passed) {
        offset--;
        if (offset < 0) {
            error(63);
            return;
        } // Bad record number
        offset *= gfs->record_length;
    } else {
        offset = -1;
    }

    static int32_t e;
    e = gfs_write(i, offset, gfs->field_buffer, gfs->record_length);
    if (e) {
        if (e == -2) {
            error(258);
            return;
        } // invalid handle
        if (e == -3) {
            error(54);
            return;
        } // bad file mode
        if (e == -4) {
            error(5);
            return;
        } // illegal function call
        if (e == -7) {
            error(70);
            return;
        } // permission denied
        error(75);
        return; // assume[-9]: path/file access error
    }
}
