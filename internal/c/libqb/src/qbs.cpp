
#include "libqb-common.h"

#include <stdlib.h>
#include <string.h>

#include "error_handle.h"
#include "file-fields.h"
#include "qbs.h"

// FIXME: Put in internal header
void qbs_remove_cmem(qbs *str);
bool qbs_new_fixed_cmem(uint8_t *offset, uint32_t size, uint8_t tmp, qbs *newstr);
void qbs_move_cmem(qbs *deststr, qbs *srcstr);
void qbs_copy_cmem(qbs *deststr, qbs *srcstr);
void qbs_create_cmem(int32_t size, uint8_t tmp, qbs *newstr);

static qbs *qbs_malloc = (qbs *)calloc(sizeof(qbs) * 65536, 1); //~1MEG
static uint32_t qbs_malloc_next = 0;                              // the next idex in qbs_malloc to use
static intptr_t *qbs_malloc_freed = (intptr_t *)malloc(sizeof (*qbs_malloc_freed) * 65536);
static uint32_t qbs_malloc_freed_size = 65536;
static uint32_t qbs_malloc_freed_num = 0; // number of freed qbs descriptors

static qbs *qbs_new_descriptor() {
    // MLP //qbshlp1++;
    if (qbs_malloc_freed_num) {
        /*MLP
            static qbs *s;
            s=(qbs*)memset((void *)qbs_malloc_freed[--qbs_malloc_freed_num],0,sizeof(qbs));
            s->dbgl=dbgline;
            return s;
        */
        return (qbs *)memset((void *)qbs_malloc_freed[--qbs_malloc_freed_num], 0, sizeof(qbs));
    }
    if (qbs_malloc_next == 65536) {
        qbs_malloc = (qbs *)calloc(sizeof(qbs) * 65536, 1); //~1MEG
        qbs_malloc_next = 0;
    }
    /*MLP
        dbglist[dbglisti]=(uint32)&qbs_malloc[qbs_malloc_next];
        static qbs* s;
        s=(qbs*)&qbs_malloc[qbs_malloc_next++];
        s->dbgl=dbgline;
        dbglisti++;
        return s;
    */
    return &qbs_malloc[qbs_malloc_next++];
}

static void qbs_free_descriptor(qbs *str) {
    // MLP //qbshlp1--;
    if (qbs_malloc_freed_num == qbs_malloc_freed_size) {
        qbs_malloc_freed_size *= 2;
        qbs_malloc_freed = (intptr_t *)realloc(qbs_malloc_freed, qbs_malloc_freed_size * sizeof (*qbs_malloc_freed));
        if (!qbs_malloc_freed)
            error(508);
    }
    qbs_malloc_freed[qbs_malloc_freed_num] = (intptr_t)str;
    qbs_malloc_freed_num++;
    return;
}

// Used to track strings in 32bit memory
static intptr_t *qbs_list = (intptr_t *)malloc(65536 * sizeof(intptr_t));
static uint32_t qbs_list_lasti = 65535;
static uint32_t qbs_list_nexti = 0;

// Used to track temporary strings for later removal when they fall out of scope
//*Some string functions delete a temporary string automatically after they have been
// passed one to save memory. In this case qbstring_templist[?]=0xFFFFFFFF
intptr_t *qbs_tmp_list = (intptr_t *)calloc(65536 * sizeof(intptr_t), 1); // first index MUST be 0
uint32_t qbs_tmp_list_lasti = 65535;

uint32_t qbs_tmp_list_nexti;
// entended string memory

static uint8_t *qbs_data = (uint8_t *)malloc(1048576);
static uint32_t qbs_data_size = 1048576;
static uint32_t qbs_sp = 0;

void qbs_free(qbs *str) {

    if (str->field)
        field_free(str);

    if (str->tmplisti) {
        qbs_tmp_list[str->tmplisti] = -1;
        while (qbs_tmp_list[qbs_tmp_list_nexti - 1] == -1) {
            qbs_tmp_list_nexti--;
        }
    }
    if (str->fixed || str->readonly) {
        qbs_free_descriptor(str);
        return;
    }
    if (str->in_cmem) {
        qbs_remove_cmem(str);
    } else {
        qbs_list[str->listi] = -1;
    retry:
        if (qbs_list[qbs_list_nexti - 1] == -1) {
            qbs_list_nexti--;
            if (qbs_list_nexti)
                goto retry;
        }
        if (qbs_list_nexti) {
            qbs_sp = ((qbs *)qbs_list[qbs_list_nexti - 1])->chr - qbs_data + ((qbs *)qbs_list[qbs_list_nexti - 1])->len + 32;
            if (qbs_sp > qbs_data_size)
                qbs_sp = qbs_data_size; // adding 32 could overflow buffer!
        } else {
            qbs_sp = 0;
        }
    }
    qbs_free_descriptor(str);
    return;
}

static void qbs_concat_list() {
    uint32_t i;
    uint32_t d;
    qbs *tqbs;
    d = 0;
    for (i = 0; i < qbs_list_nexti; i++) {
        if (qbs_list[i] != -1) {
            if (i != d) {
                tqbs = (qbs *)qbs_list[i];
                tqbs->listi = d;
                qbs_list[d] = (intptr_t)tqbs;
            }
            d++;
        }
    }
    qbs_list_nexti = d;
    // if string listings are taking up more than half of the list array double the list array's size
    if (qbs_list_nexti >= (qbs_list_lasti / 2)) {
        qbs_list_lasti *= 2;
        qbs_list = (intptr_t *)realloc(qbs_list, (qbs_list_lasti + 1) * sizeof (*qbs_list));
        if (!qbs_list)
            error(510);
    }
    return;
}

static void qbs_tmp_concat_list() {
    if (qbs_tmp_list_nexti >= (qbs_tmp_list_lasti / 2)) {
        qbs_tmp_list_lasti *= 2;
        qbs_tmp_list = (intptr_t *)realloc(qbs_tmp_list, (qbs_tmp_list_lasti + 1) * sizeof (*qbs_tmp_list));
        if (!qbs_tmp_list)
            error(511);
    }
    return;
}

static void qbs_concat(uint32_t bytesrequired) {
    // this does not change indexing, only ->chr pointers and the location of their data
    static int32_t i;
    static uint8_t *dest;
    static qbs *tqbs;
    dest = (uint8_t *)qbs_data;
    if (qbs_list_nexti) {
        qbs_sp = 0;
        for (i = 0; i < qbs_list_nexti; i++) {
            if (qbs_list[i] != -1) {
                tqbs = (qbs *)qbs_list[i];
                if ((tqbs->chr - dest) > 32) {
                    if (tqbs->len) {
                        memmove(dest, tqbs->chr, tqbs->len);
                    }
                    tqbs->chr = dest;
                }
                dest = tqbs->chr + tqbs->len;
                qbs_sp = dest - qbs_data;
            }
        }
    }

    if (((qbs_sp * 2) + (bytesrequired + 32)) >= qbs_data_size) {
        static uint8_t *oldbase;
        oldbase = qbs_data;
        qbs_data_size = qbs_data_size * 2 + bytesrequired;
        qbs_data = (uint8_t *)realloc(qbs_data, qbs_data_size);
        if (qbs_data == NULL)
            error(512); // realloc failed!
        for (i = 0; i < qbs_list_nexti; i++) {
            if (qbs_list[i] != -1) {
                tqbs = (qbs *)qbs_list[i];
                tqbs->chr = tqbs->chr - oldbase + qbs_data;
            }
        }
    }
    return;
}


qbs *qbs_new_txt(const char *txt) {
    qbs *newstr;
    newstr = qbs_new_descriptor();
    if (!txt) { // NULL pointer is converted to a 0-length string
        newstr->len = 0;
    } else {
        newstr->len = strlen(txt);
    }
    newstr->chr = (uint8_t *)txt;
    if (qbs_tmp_list_nexti > qbs_tmp_list_lasti)
        qbs_tmp_concat_list();
    newstr->tmplisti = qbs_tmp_list_nexti;
    qbs_tmp_list[newstr->tmplisti] = (intptr_t)newstr;
    qbs_tmp_list_nexti++;
    newstr->tmp = 1;
    newstr->readonly = 1;
    return newstr;
}

qbs *qbs_new_txt_len(const char *txt, int32_t len) {
    qbs *newstr;
    newstr = qbs_new_descriptor();
    newstr->len = len;
    newstr->chr = (uint8_t *)txt;
    if (qbs_tmp_list_nexti > qbs_tmp_list_lasti)
        qbs_tmp_concat_list();
    newstr->tmplisti = qbs_tmp_list_nexti;
    qbs_tmp_list[newstr->tmplisti] = (intptr_t)newstr;
    qbs_tmp_list_nexti++;
    newstr->tmp = 1;
    newstr->readonly = 1;
    return newstr;
}

// note: qbs_new_fixed detects if string is in DBLOCK
qbs *qbs_new_fixed(uint8_t *offset, uint32_t size, uint8_t tmp) {
    qbs *newstr;
    newstr = qbs_new_descriptor();
    newstr->len = size;
    newstr->chr = offset;
    newstr->fixed = 1;
    if (tmp) {
        if (qbs_tmp_list_nexti > qbs_tmp_list_lasti)
            qbs_tmp_concat_list();
        newstr->tmplisti = qbs_tmp_list_nexti;
        qbs_tmp_list[newstr->tmplisti] = (intptr_t)newstr;
        qbs_tmp_list_nexti++;
        newstr->tmp = 1;
    } else {
        qbs_new_fixed_cmem(offset, size, tmp, newstr);
    }
    return newstr;
}

qbs *qbs_new(int32_t size, uint8_t tmp) {
    static qbs *newstr;
    if ((qbs_sp + size + 32) > qbs_data_size)
        qbs_concat(size + 32);
    newstr = qbs_new_descriptor();
    newstr->len = size;
    newstr->chr = qbs_data + qbs_sp;
    qbs_sp += size + 32;
    if (qbs_list_nexti > qbs_list_lasti)
        qbs_concat_list();
    newstr->listi = qbs_list_nexti;
    qbs_list[newstr->listi] = (intptr_t)newstr;
    qbs_list_nexti++;
    if (tmp) {
        if (qbs_tmp_list_nexti > qbs_tmp_list_lasti)
            qbs_tmp_concat_list();
        newstr->tmplisti = qbs_tmp_list_nexti;
        qbs_tmp_list[newstr->tmplisti] = (intptr_t)newstr;
        qbs_tmp_list_nexti++;
        newstr->tmp = 1;
    }
    return newstr;
}

qbs *qbs_new_cmem(int32_t size, uint8_t tmp) {
    qbs *newstr = qbs_new_descriptor();
    if (tmp && qbs_tmp_list_nexti > qbs_tmp_list_lasti)
        qbs_tmp_concat_list();

    qbs_create_cmem(size, tmp, newstr);

    return newstr;
}

void qbs_maketmp(qbs *str) {
    // WARNING: assumes str is a non-tmp string in non-cmem
    if (qbs_tmp_list_nexti > qbs_tmp_list_lasti)
        qbs_tmp_concat_list();
    str->tmplisti = qbs_tmp_list_nexti;
    qbs_tmp_list[str->tmplisti] = (intptr_t)str;
    qbs_tmp_list_nexti++;
    str->tmp = 1;
}

qbs *qbs_set(qbs *deststr, qbs *srcstr) {
    int32_t i;
    qbs *tqbs;
    // fixed deststr
    if (deststr->fixed) {
        if (srcstr->len >= deststr->len) {
            memcpy(deststr->chr, srcstr->chr, deststr->len);
        } else {
            memcpy(deststr->chr, srcstr->chr, srcstr->len);
            memset(deststr->chr + srcstr->len, 32, deststr->len - srcstr->len); // pad with spaces
        }
        goto qbs_set_return;
    }
    // non-fixed deststr

    // can srcstr be acquired by deststr?
    if (srcstr->tmp && srcstr->fixed == 0 && srcstr->readonly == 0 && (srcstr->in_cmem == deststr->in_cmem)) {
        if (deststr->in_cmem) {
            qbs_move_cmem(deststr, srcstr);
        } else {
            // unlist deststr and acquire srcstr's list index
            qbs_list[deststr->listi] = -1;
            qbs_list[srcstr->listi] = (intptr_t)deststr;
            deststr->listi = srcstr->listi;
        }

        qbs_tmp_list[srcstr->tmplisti] = -1;
        if (srcstr->tmplisti == (qbs_tmp_list_nexti - 1))
            qbs_tmp_list_nexti--; // correct last tmp index for performance

        deststr->chr = srcstr->chr;
        deststr->len = srcstr->len;
        qbs_free_descriptor(srcstr);

        return deststr; // nb. This return cannot be changed to a goto qbs_set_return!
    }

    // srcstr is equal length or shorter
    if (srcstr->len <= deststr->len) {
        memcpy(deststr->chr, srcstr->chr, srcstr->len);
        deststr->len = srcstr->len;
        goto qbs_set_return;
    }

    // srcstr is longer
    if (deststr->in_cmem) {
        qbs_copy_cmem(deststr, srcstr);
        goto qbs_set_return;
    }

    // not in cmem
    if (deststr->listi == (qbs_list_nexti - 1)) {                                             // last index
        if (((intptr_t)deststr->chr + srcstr->len) <= ((intptr_t)qbs_data + qbs_data_size)) { // space available
            memcpy(deststr->chr, srcstr->chr, srcstr->len);
            deststr->len = srcstr->len;
            qbs_sp = ((intptr_t)deststr->chr) + (intptr_t)deststr->len - (intptr_t)qbs_data;
            goto qbs_set_return;
        }
        goto qbs_set_concat_required;
    }
    // deststr is not the last index so locate next valid index
    i = deststr->listi + 1;
qbs_set_nextindex2:
    if (qbs_list[i] != -1) {
        tqbs = (qbs *)qbs_list[i];
        if (tqbs == srcstr) {
            if (srcstr->tmp == 1)
                goto skippedtmpsrcindex2;
        }
        if ((deststr->chr + srcstr->len) > tqbs->chr)
            goto qbs_set_concat_required;
        memcpy(deststr->chr, srcstr->chr, srcstr->len);
        deststr->len = srcstr->len;
        goto qbs_set_return;
    }
skippedtmpsrcindex2:
    i++;
    if (i != qbs_list_nexti)
        goto qbs_set_nextindex2;
    // all next indexes invalid!

    qbs_list_nexti = deststr->listi + 1;                                                  // adjust nexti
    if (((intptr_t)deststr->chr + srcstr->len) <= ((intptr_t)qbs_data + qbs_data_size)) { // space available
        memmove(deststr->chr, srcstr->chr, srcstr->len);                                  // overlap possible due to sometimes acquiring srcstr's space
        deststr->len = srcstr->len;
        qbs_sp = ((intptr_t)deststr->chr) + (intptr_t)deststr->len - (intptr_t)qbs_data;
        goto qbs_set_return;
    }

qbs_set_concat_required:
    // srcstr could not fit in deststr
    //"realloc" deststr
    qbs_list[deststr->listi] = -1;                // unlist
    if ((qbs_sp + srcstr->len) > qbs_data_size) { // must concat!
        qbs_concat(srcstr->len);
    }
    if (qbs_list_nexti > qbs_list_lasti)
        qbs_concat_list();
    deststr->listi = qbs_list_nexti;
    qbs_list[qbs_list_nexti] = (intptr_t)deststr;
    qbs_list_nexti++; // relist

    deststr->chr = qbs_data + qbs_sp;
    deststr->len = srcstr->len;
    qbs_sp += deststr->len;
    memcpy(deststr->chr, srcstr->chr, srcstr->len);

//(fall through to qbs_set_return)
qbs_set_return:
    if (srcstr->tmp) { // remove srcstr if it is a tmp string
        qbs_free(srcstr);
    }

    return deststr;
}

qbs *qbs_add(qbs *str1, qbs *str2) {
    qbs *tqbs;
    if (!str2->len)
        return str1; // pass on
    if (!str1->len)
        return str2; // pass on
    // may be possible to acquire str1 or str2's space but...
    // 1. check if dest has enough space (because its data is already in the correct place)
    // 2. check if source has enough space
    // 3. give up
    // nb. they would also have to be a tmp, var. len str in ext memory!
    // brute force method...
    tqbs = qbs_new(str1->len + str2->len, 1);
    memcpy(tqbs->chr, str1->chr, str1->len);
    memcpy(tqbs->chr + str1->len, str2->chr, str2->len);

    // exit(qbs_sp);
    if (str1->tmp)
        qbs_free(str1);
    if (str2->tmp)
        qbs_free(str2);
    return tqbs;
}

qbs *qbs_ucase(qbs *str) {
    if (!str->len)
        return str;
    qbs *tqbs = NULL;
    if (str->tmp && !str->fixed && !str->readonly && !str->in_cmem) {
        tqbs = str;
    } else {
        tqbs = qbs_new(str->len, 1);
        memcpy(tqbs->chr, str->chr, str->len);
    }
    unsigned char *c = tqbs->chr;
    for (int32_t i = 0; i < str->len; i++) {
        if ((*c >= 'a') && (*c <= 'z'))
            *c = *c & 223;
        c++;
    }
    if (tqbs != str && str->tmp)
        qbs_free(str);
    return tqbs;
}

qbs *qbs_lcase(qbs *str) {
    if (!str->len)
        return str;
    qbs *tqbs = NULL;
    if (str->tmp && !str->fixed && !str->readonly && !str->in_cmem) {
        tqbs = str;
    } else {
        tqbs = qbs_new(str->len, 1);
        memcpy(tqbs->chr, str->chr, str->len);
    }
    unsigned char *c = tqbs->chr;
    for (int32_t i = 0; i < str->len; i++) {
        if ((*c >= 'A') && (*c <= 'Z'))
            *c = *c | 32;
        c++;
    }
    if (tqbs != str && str->tmp)
        qbs_free(str);
    return tqbs;
}

qbs *qbs_left(qbs *str, int32_t l) {
    if (l > str->len)
        l = str->len;
    if (l < 0)
        l = 0;
    if (l == str->len)
        return str; // pass on
    if (str->tmp) {
        if (!str->fixed) {
            if (!str->readonly) {
                if (!str->in_cmem) {
                    str->len = l;
                    return str;
                }
            }
        }
    }
    qbs *tqbs;
    tqbs = qbs_new(l, 1);
    if (l)
        memcpy(tqbs->chr, str->chr, l);
    if (str->tmp)
        qbs_free(str);
    return tqbs;
}

qbs *qbs_right(qbs *str, int32_t l) {
    if (l > str->len)
        l = str->len;
    if (l < 0)
        l = 0;
    if (l == str->len)
        return str; // pass on
    if (str->tmp) {
        if (!str->fixed) {
            if (!str->readonly) {
                if (!str->in_cmem) {
                    str->chr = str->chr + (str->len - l);
                    str->len = l;
                    return str;
                }
            }
        }
    }
    qbs *tqbs;
    tqbs = qbs_new(l, 1);
    if (l)
        memcpy(tqbs->chr, str->chr + str->len - l, l);
    tqbs->len = l;
    if (str->tmp)
        qbs_free(str);
    return tqbs;
}

void set_qbs_size(intptr_t *target_qbs, int32_t newlength) {
    qbs_set((qbs *)(*target_qbs), func_space(newlength));
    return;
}

int32_t qbs_equal(qbs *str1, qbs *str2) {
    if (str1->len != str2->len)
        return 0;
    if (memcmp(str1->chr, str2->chr, str1->len) == 0)
        return -1;
    return 0;
}

int32_t qbs_notequal(qbs *str1, qbs *str2) {
    if (str1->len != str2->len)
        return -1;
    if (memcmp(str1->chr, str2->chr, str1->len) == 0)
        return 0;
    return -1;
}

int32_t qbs_greaterthan(qbs *str2, qbs *str1) {
    // same process as for lessthan; we just reverse the string order
    int32_t i, limit, l1, l2;
    l1 = str1->len;
    l2 = str2->len;
    if (!l1)
        if (l2)
            return -1;
        else
            return 0;
    if (l1 <= l2)
        limit = l1;
    else
        limit = l2;
    i = memcmp(str1->chr, str2->chr, limit);
    if (i < 0)
        return -1;
    if (i > 0)
        return 0;
    if (l1 < l2)
        return -1;
    return 0;
}

int32_t qbs_lessthan(qbs *str1, qbs *str2) {
    int32_t i, limit, l1, l2;
    l1 = str1->len;
    l2 = str2->len; // no need to get the length of these strings multiple times.
    if (!l1)
        if (l2)
            return -1;
        else
            return 0; // if one is a null string we known the answer already.
    if (l1 <= l2)
        limit = l1;
    else
        limit = l2;                          // our limit is going to be the length of the smallest string.
    i = memcmp(str1->chr, str2->chr, limit); // check only to the length of the shortest string
    if (i < 0)
        return -1; // if the number is smaller by this point, say so
    if (i > 0)
        return 0; // if it's larger by this point, say so
    // if the number is the same at this point, compare length.
    // if the length of the first one is smaller, then the string is smaller. Otherwise the second one is the same string, or longer.
    if (l1 < l2)
        return -1;
    return 0;
}

int32_t qbs_lessorequal(qbs *str1, qbs *str2) {
    // same process as lessthan, but we check to see if the lengths are equal here also.
    int32_t i, limit, l1, l2;
    l1 = str1->len;
    l2 = str2->len;
    if (!l1)
        return -1; // if the first string has no length then it HAS to be smaller or equal to the second
    if (l1 <= l2)
        limit = l1;
    else
        limit = l2;
    i = memcmp(str1->chr, str2->chr, limit);
    if (i < 0)
        return -1;
    if (i > 0)
        return 0;
    if (l1 <= l2)
        return -1;
    return 0;
}

int32_t qbs_greaterorequal(qbs *str2, qbs *str1) {
    // same process as for lessorequal; we just reverse the string order
    int32_t i, limit, l1, l2;
    l1 = str1->len;
    l2 = str2->len;
    if (!l1)
        return -1;
    if (l1 <= l2)
        limit = l1;
    else
        limit = l2;
    i = memcmp(str1->chr, str2->chr, limit);
    if (i < 0)
        return -1;
    if (i > 0)
        return 0;
    if (l1 <= l2)
        return -1;
    return 0;
}

int32_t qbs_asc(qbs *str, uint32_t i) { // uint32 speeds up checking for negative
    i--;
    if (i < str->len) {
        return str->chr[i];
    }
    error(5);
    return 0;
}

int32_t qbs_asc(qbs *str) {
    if (str->len)
        return str->chr[0];
    error(5);
    return 0;
}

int32_t qbs_len(qbs *str) {
    return str->len;
}

qbs *func_chr(int32_t value) {
    qbs *tqbs;
    if ((value < 0) || (value > 255)) {
        tqbs = qbs_new(0, 1);
        error(5);
    } else {
        tqbs = qbs_new(1, 1);
        tqbs->chr[0] = value;
    }
    return tqbs;
}

