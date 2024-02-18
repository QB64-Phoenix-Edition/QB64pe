
#include "libqb-common.h"

#include <string.h>
#include <stdlib.h>

#include "error_handle.h"
#include "file-fields.h"
#include "qbs.h"

extern qbs *nothingstring;

void sub_lset(qbs *dest, qbs *source) {
    if (is_error_pending())
        return;
    if (source->len >= dest->len) {
        if (dest->len)
            memcpy(dest->chr, source->chr, dest->len);
        goto field_check;
    }
    if (source->len)
        memcpy(dest->chr, source->chr, source->len);
    memset(dest->chr + source->len, 32, dest->len - source->len);
field_check:
    if (dest->field)
        lrset_field(dest);
}

void sub_rset(qbs *dest, qbs *source) {
    if (is_error_pending())
        return;
    if (source->len >= dest->len) {
        if (dest->len)
            memcpy(dest->chr, source->chr, dest->len);
        goto field_check;
    }
    if (source->len)
        memcpy(dest->chr + dest->len - source->len, source->chr, source->len);
    memset(dest->chr, 32, dest->len - source->len);
field_check:
    if (dest->field)
        lrset_field(dest);
}

qbs *func_space(int32_t spaces) {
    static qbs *tqbs;
    if (spaces < 0)
        spaces = 0;
    tqbs = qbs_new(spaces, 1);
    if (spaces)
        memset(tqbs->chr, 32, spaces);
    return tqbs;
}

qbs *func_string(int32_t characters, int32_t asciivalue) {
    static qbs *tqbs;
    if (characters < 0)
        characters = 0;
    tqbs = qbs_new(characters, 1);
    if (characters)
        memset(tqbs->chr, asciivalue & 0xFF, characters);
    return tqbs;
}

int32_t func_instr(int32_t start, qbs *str, qbs *substr, int32_t passed) {
    // QB64 difference: start can be 0 or negative
    // justification-start could be larger than the length of string to search in QBASIC
    static uint8_t *limit, *base;
    static uint8_t firstc;
    if (!passed)
        start = 1;
    if (!str->len)
        return 0;
    if (start < 1) {
        start = 1;
        if (!substr->len)
            return 0;
    }
    if (start > str->len)
        return 0;
    if (!substr->len)
        return start;
    if ((start + substr->len - 1) > str->len)
        return 0;
    limit = str->chr + str->len;
    firstc = substr->chr[0];
    base = str->chr + start - 1;
nextchar:
    base = (uint8_t *)memchr(base, firstc, limit - base);
    if (!base)
        return 0;
    if ((base + substr->len) > limit)
        return 0;
    if (!memcmp(base, substr->chr, substr->len))
        return base - str->chr + 1;
    base++;
    if ((base + substr->len) > limit)
        return 0;
    goto nextchar;
}

int32_t func__instrrev(int32_t start, qbs *str, qbs *substr, int32_t passed) {
    if (!str->len)
        return 0;
    if (substr->len > str->len)
        return 0;
    if (!passed) {
        if (substr->len == str->len) {
            if (!memcmp(str->chr, substr->chr, str->len))
                return 1;
        }
        start = str->len - substr->len + 1;
    }
    if (start < 1) {
        start = str->len - substr->len + 1;
    }
    if (start > str->len)
        start = str->len - substr->len + 1;
    if (!substr->len)
        return start - 1;
    if ((start + substr->len - 1) > str->len)
        start = str->len - substr->len + 1;

    int32_t searchForward = 0, lastFound = 0, result = 0;
    do {
        searchForward = func_instr(searchForward + 1, str, substr, 1);
        if (searchForward > 0) {
            lastFound = searchForward;
            if (lastFound <= start)
                result = lastFound;
            if (lastFound > start)
                break;
        }
    } while (searchForward > 0);

    return result;
}

void sub_mid(qbs *dest, int32_t start, int32_t l, qbs *src, int32_t passed) {
    if (is_error_pending())
        return;
    static int32_t src_offset;
    if (!passed)
        l = src->len;
    src_offset = 0;
    if (dest == nothingstring)
        return; // quiet exit, error has already been reported!
    if (start < 1) {
        l = l + start - 1;
        src_offset = -start + 1; // src_offset is a byte offset with base 0!
        start = 1;
    }
    if (l <= 0)
        return;
    if (start > dest->len)
        return;
    if ((start + l - 1) > dest->len)
        l = dest->len - start + 1;
    // start and l are now reflect a valid region within dest
    if (src_offset >= src->len)
        return;
    if (l > (src->len - src_offset))
        l = src->len - src_offset;
    // src_offset and l now reflect a valid region within src
    if (dest == src) {
        if ((start - 1) != src_offset)
            memmove(dest->chr + start - 1, src->chr + src_offset, l);
    } else {
        memcpy(dest->chr + start - 1, src->chr + src_offset, l);
    }
}

qbs *func_mid(qbs *str, int32_t start, int32_t l, int32_t passed) {
    static qbs *tqbs;
    if (passed) {
        if (start < 1) {
            l = l - 1 + start;
            start = 1;
        }
        if ((l >= 1) && (start <= str->len)) {
            if ((start + l) > str->len)
                l = str->len - start + 1;
        } else {
            l = 0;
            start = 1; // nothing!
        }
    } else {
        if (start < 1)
            start = 1;
        l = str->len - start + 1;
        if (l < 1) {
            l = 0;
            start = 1; // nothing!
        }
    }
    if ((start == 1) && (l == str->len))
        return str; // pass on
    if (str->tmp) {
        if (!str->fixed) {
            if (!str->readonly) {
                if (!str->in_cmem) { // acquire
                    str->chr = str->chr + (start - 1);
                    str->len = l;
                    return str;
                }
            }
        }
    }
    tqbs = qbs_new(l, 1);
    if (l)
        memcpy(tqbs->chr, str->chr + start - 1, l);
    if (str->tmp)
        qbs_free(str);
    return tqbs;
}

qbs *qbs_ltrim(qbs *str) {
    if (!str->len)
        return str; // pass on
    if (*str->chr != 32)
        return str; // pass on
    if (str->tmp) {
        if (!str->fixed) {
            if (!str->readonly) {
                if (!str->in_cmem) { // acquire?
                qbs_ltrim_nextchar:
                    if (*str->chr == 32) {
                        str->chr++;
                        if (--str->len)
                            goto qbs_ltrim_nextchar;
                    }
                    return str;
                }
            }
        }
    }
    int32_t i;
    i = 0;
qbs_ltrim_nextchar2:
    if (str->chr[i] == 32) {
        i++;
        if (i < str->len)
            goto qbs_ltrim_nextchar2;
    }
    qbs *tqbs;
    tqbs = qbs_new(str->len - i, 1);
    if (tqbs->len)
        memcpy(tqbs->chr, str->chr + i, tqbs->len);
    if (str->tmp)
        qbs_free(str);
    return tqbs;
}

qbs *qbs_rtrim(qbs *str) {
    if (!str->len)
        return str; // pass on
    if (str->chr[str->len - 1] != 32)
        return str; // pass on
    if (str->tmp) {
        if (!str->fixed) {
            if (!str->readonly) {
                if (!str->in_cmem) { // acquire?
                qbs_rtrim_nextchar:
                    if (str->chr[str->len - 1] == 32) {
                        if (--str->len)
                            goto qbs_rtrim_nextchar;
                    }
                    return str;
                }
            }
        }
    }
    int32_t i;
    i = str->len;
qbs_rtrim_nextchar2:
    if (str->chr[i - 1] == 32) {
        i--;
        if (i)
            goto qbs_rtrim_nextchar2;
    }
    // i is the number of characters to keep
    qbs *tqbs;
    tqbs = qbs_new(i, 1);
    if (i)
        memcpy(tqbs->chr, str->chr, i);
    if (str->tmp)
        qbs_free(str);
    return tqbs;
}

qbs *qbs__trim(qbs *str) { return qbs_rtrim(qbs_ltrim(str)); }

int32_t func__str_nc_compare(qbs *s1, qbs *s2) {
    int32_t limit, l1, l2;
    int32_t v1, v2;
    unsigned char *c1 = s1->chr, *c2 = s2->chr;

    l1 = s1->len;
    l2 = s2->len; // no need to get the length of these strings multiple times.
    if (!l1) {
        if (l2)
            return -1;
        else
            return 0; // if one is a null string we known the answer already.
    }
    if (!l2)
        return 1;
    if (l1 <= l2)
        limit = l1;
    else
        limit = l2; // our limit is going to be the length of the smallest string.

    for (int32_t i = 0; i < limit; i++) { // check the length of our string
        v1 = *c1;
        v2 = *c2;
        if ((v1 > 64) && (v1 < 91))
            v1 = v1 | 32;
        if ((v2 > 64) && (v2 < 91))
            v2 = v2 | 32;
        if (v1 < v2)
            return -1;
        if (v1 > v2)
            return 1;
        c1++;
        c2++;
    }

    if (l1 < l2)
        return -1;
    if (l1 > l2)
        return 1;
    return 0;
}

int32_t func__str_compare(qbs *s1, qbs *s2) {
    int32_t i, limit, l1, l2;
    l1 = s1->len;
    l2 = s2->len; // no need to get the length of these strings multiple times.
    if (!l1) {
        if (l2)
            return -1;
        else
            return 0; // if one is a null string we known the answer already.
    }
    if (!l2)
        return 1;
    if (l1 <= l2)
        limit = l1;
    else
        limit = l2;
    i = memcmp(s1->chr, s2->chr, limit);
    if (i < 0)
        return -1;
    if (i > 0)
        return 1;
    if (l1 < l2)
        return -1;
    if (l1 > l2)
        return 1;
    return 0;
}

