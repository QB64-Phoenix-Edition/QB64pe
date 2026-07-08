
#include "libqb-common.h"

#include <algorithm>
#include <limits>
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
static uint32_t qbs_malloc_next = 0;                            // the next idex in qbs_malloc to use
static intptr_t *qbs_malloc_freed = (intptr_t *)malloc(sizeof(*qbs_malloc_freed) * 65536);
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
        qbs_malloc_freed = (intptr_t *)realloc(qbs_malloc_freed, qbs_malloc_freed_size * sizeof(*qbs_malloc_freed));
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
static size_t qbs_data_size = 1048576;
static size_t qbs_sp = 0;

#ifdef QB64_QBS_TEST
static void *(*qbs_data_realloc)(void *, size_t) = realloc;
#else
static void *qbs_data_realloc(void *memory, size_t size) {
    return realloc(memory, size);
}
#endif

static bool qbs_size_add_overflow(size_t left, size_t right, size_t *result) {
    if (left > std::numeric_limits<size_t>::max() - right)
        return true;

    *result = left + right;
    return false;
}

static size_t qbs_data_offset(const uint8_t *position) {
    return (size_t)((uintptr_t)position - (uintptr_t)qbs_data);
}

static bool qbs_data_has_space(const uint8_t *position, size_t bytes) {
    size_t offset = qbs_data_offset(position);
    return offset <= qbs_data_size && bytes <= qbs_data_size - offset;
}

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
            qbs *laststr = (qbs *)qbs_list[qbs_list_nexti - 1];
            size_t offset = qbs_data_offset(laststr->chr);
            size_t length = laststr->len;
            size_t used;

            if (offset > qbs_data_size || qbs_size_add_overflow(offset, length, &used) || used > qbs_data_size ||
                qbs_size_add_overflow(used, 32, &qbs_sp) || qbs_sp > qbs_data_size) {
                qbs_sp = qbs_data_size;
            }
        } else {
            qbs_sp = 0;
        }
    }
    qbs_free_descriptor(str);
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
        qbs_list = (intptr_t *)realloc(qbs_list, (qbs_list_lasti + 1) * sizeof(*qbs_list));
        if (!qbs_list)
            error(510);
    }
}

static void qbs_tmp_concat_list() {
    if (qbs_tmp_list_nexti >= (qbs_tmp_list_lasti / 2)) {
        qbs_tmp_list_lasti *= 2;
        qbs_tmp_list = (intptr_t *)realloc(qbs_tmp_list, (qbs_tmp_list_lasti + 1) * sizeof(*qbs_tmp_list));
        if (!qbs_tmp_list)
            error(511);
    }
}

static bool qbs_resize_data(size_t new_size) {
    uintptr_t oldbase = (uintptr_t)qbs_data;
    uint8_t *newbase = (uint8_t *)qbs_data_realloc(qbs_data, new_size);
    if (newbase == NULL)
        return false;

    qbs_data = newbase;
    qbs_data_size = new_size;

    for (uint32_t i = 0; i < qbs_list_nexti; i++) {
        if (qbs_list[i] != -1) {
            qbs *tqbs = (qbs *)qbs_list[i];
            tqbs->chr = qbs_data + (size_t)((uintptr_t)tqbs->chr - oldbase);
        }
    }

    return true;
}

static void qbs_concat(size_t bytesrequired) {
    // this does not change indexing, only ->chr pointers and the location of their data
    static uint32_t i;
    static uint8_t *dest;
    static qbs *tqbs;
    dest = (uint8_t *)qbs_data;
    if (qbs_list_nexti) {
        qbs_sp = 0;
        for (i = 0; i < qbs_list_nexti; i++) {
            if (qbs_list[i] != -1) {
                tqbs = (qbs *)qbs_list[i];
                if ((size_t)((uintptr_t)tqbs->chr - (uintptr_t)dest) > 32) {
                    if (tqbs->len) {
                        memmove(dest, tqbs->chr, tqbs->len);
                    }
                    tqbs->chr = dest;
                }
                dest = tqbs->chr + tqbs->len;
                qbs_sp = qbs_data_offset(dest);
            }
        }
    }

    size_t required_size;
    if (qbs_size_add_overflow(qbs_sp, bytesrequired, &required_size)) {
        error(512);
        return;
    }

    // Keep the existing growth policy without performing an overflowing 2 * qbs_sp calculation.
    size_t reserve;
    bool grow = required_size > qbs_data_size;
    if (!grow) {
        if (qbs_size_add_overflow(bytesrequired, 32, &reserve) || reserve >= qbs_data_size) {
            grow = true;
        } else {
            size_t threshold = qbs_data_size - reserve;
            grow = qbs_sp >= (threshold / 2 + threshold % 2);
        }
    }

    if (grow) {
        size_t new_size;
        const size_t max_size = std::numeric_limits<size_t>::max();

        if (qbs_data_size <= (max_size - bytesrequired) / 2) {
            new_size = qbs_data_size * 2 + bytesrequired;
        } else {
            // Geometric growth no longer fits in size_t. Grow only by the amount actually required
            // so 32-bit targets can still use the remaining address space instead of wrapping.
            new_size = required_size;
        }

        if (new_size > qbs_data_size) {
            if (!qbs_resize_data(new_size)) {
                // The geometric reserve is optional. On constrained address spaces (especially
                // 32-bit targets), allocating both the old arena and a doubled replacement can
                // fail even though the bytes required by the current request would still fit.
                if (required_size <= qbs_data_size)
                    return;

                // realloc leaves the old allocation untouched on failure, so retry with only
                // the minimum size needed by this request before reporting an actual OOM.
                if (new_size == required_size || !qbs_resize_data(required_size)) {
                    error(512);
                    return;
                }
            }
        } else if (required_size > qbs_data_size) {
            error(512);
            return;
        }
    }
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
    if (size < 0) {
        error(512);
        return NULL;
    }

    size_t bytesrequired;
    size_t new_sp;
    if (qbs_size_add_overflow((size_t)size, 32, &bytesrequired) || qbs_size_add_overflow(qbs_sp, bytesrequired, &new_sp)) {
        error(512);
        return NULL;
    }
    if (new_sp > qbs_data_size) {
        qbs_concat(bytesrequired);
        if (qbs_size_add_overflow(qbs_sp, bytesrequired, &new_sp) || new_sp > qbs_data_size) {
            error(512);
            return NULL;
        }
    }

    newstr = qbs_new_descriptor();
    newstr->len = size;
    newstr->chr = qbs_data + qbs_sp;
    qbs_sp = new_sp;
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
    uint32_t i;
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
    if (deststr->listi == (qbs_list_nexti - 1)) {                                                       // last index
        if (qbs_data_has_space(deststr->chr, (size_t)srcstr->len)) { // space available
            memcpy(deststr->chr, srcstr->chr, srcstr->len);
            deststr->len = srcstr->len;
            qbs_sp = qbs_data_offset(deststr->chr) + (size_t)deststr->len;
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
        if ((size_t)srcstr->len > (size_t)((uintptr_t)tqbs->chr - (uintptr_t)deststr->chr))
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

    qbs_list_nexti = deststr->listi + 1;                                                            // adjust nexti
    if (qbs_data_has_space(deststr->chr, (size_t)srcstr->len)) { // space available
        memmove(deststr->chr, srcstr->chr, srcstr->len); // overlap possible due to sometimes acquiring srcstr's space
        deststr->len = srcstr->len;
        qbs_sp = qbs_data_offset(deststr->chr) + (size_t)deststr->len;
        goto qbs_set_return;
    }

qbs_set_concat_required:
    // srcstr could not fit in deststr
    //"realloc" deststr
    qbs_list[deststr->listi] = -1; // unlist
    size_t new_sp;
    if (qbs_size_add_overflow(qbs_sp, (size_t)srcstr->len, &new_sp)) {
        error(512);
        return deststr;
    }
    if (new_sp > qbs_data_size) { // must concat!
        qbs_concat((size_t)srcstr->len);
        if (qbs_size_add_overflow(qbs_sp, (size_t)srcstr->len, &new_sp) || new_sp > qbs_data_size) {
            error(512);
            return deststr;
        }
    }
    if (qbs_list_nexti > qbs_list_lasti)
        qbs_concat_list();
    deststr->listi = qbs_list_nexti;
    qbs_list[qbs_list_nexti] = (intptr_t)deststr;
    qbs_list_nexti++; // relist

    deststr->chr = qbs_data + qbs_sp;
    deststr->len = srcstr->len;
    qbs_sp = new_sp;
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

    size_t combined_length;
    if (str1->len < 0 || str2->len < 0 ||
        qbs_size_add_overflow((size_t)str1->len, (size_t)str2->len, &combined_length) ||
        combined_length > (size_t)std::numeric_limits<int32_t>::max()) {
        error(512);
        return NULL;
    }
    // may be possible to acquire str1 or str2's space but...
    // 1. check if dest has enough space (because its data is already in the correct place)
    // 2. check if source has enough space
    // 3. give up
    // nb. they would also have to be a tmp, var. len str in ext memory!
    // brute force method...
    tqbs = qbs_new((int32_t)combined_length, 1);
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
    auto l1 = str1->len;
    auto l2 = str2->len;

    if (!l1) {
        if (l2)
            return -1;
        else
            return 0;
    }

    auto limit = std::min(l1, l2);

    auto i = memcmp(str1->chr, str2->chr, limit);

    if (i < 0)
        return -1;

    if (i > 0)
        return 0;

    if (l1 < l2)
        return -1;

    return 0;
}

int32_t qbs_lessthan(qbs *str1, qbs *str2) {
    auto l1 = str1->len;
    auto l2 = str2->len; // no need to get the length of these strings multiple times.

    if (!l1) {
        if (l2)
            return -1;
        else
            return 0; // if one is a null string we known the answer already.
    }

    auto limit = std::min(l1, l2); // our limit is going to be the length of the smallest string.

    auto i = memcmp(str1->chr, str2->chr, limit); // check only to the length of the shortest string

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
    auto l1 = str1->len;
    auto l2 = str2->len;

    if (!l1)
        return -1; // if the first string has no length then it HAS to be smaller or equal to the second

    auto limit = std::min(l1, l2);

    auto i = memcmp(str1->chr, str2->chr, limit);

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
    auto l1 = str1->len;
    auto l2 = str2->len;

    if (!l1)
        return -1;

    auto limit = std::min(l1, l2);

    auto i = memcmp(str1->chr, str2->chr, limit);

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
    if (i < uint32_t(str->len)) {
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
