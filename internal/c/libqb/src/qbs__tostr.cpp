
#include "libqb-common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_handle.h"
#include "qbs.h"

// modern _TOSTR() functions (no leading space and no QB4.5 compatible rounding)
// signed integers
qbs *qbs__tostr(int64_t value, int32_t digits, int32_t passed) {
    (void)digits;
    (void)passed;
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%" PRId64, value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}

qbs *qbs__tostr(int32_t value, int32_t digits, int32_t passed) {
    (void)digits;
    (void)passed;
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%i", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}

qbs *qbs__tostr(int16_t value, int32_t digits, int32_t passed) {
    (void)digits;
    (void)passed;
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%i", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}

qbs *qbs__tostr(int8_t value, int32_t digits, int32_t passed) {
    (void)digits;
    (void)passed;
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%i", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}

// unsigned integers
qbs *qbs__tostr(uint64_t value, int32_t digits, int32_t passed) {
    (void)digits;
    (void)passed;
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%" PRIu64, value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}

qbs *qbs__tostr(uint32_t value, int32_t digits, int32_t passed) {
    (void)digits;
    (void)passed;
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%u", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}

qbs *qbs__tostr(uint16_t value, int32_t digits, int32_t passed) {
    (void)digits;
    (void)passed;
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%u", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}

qbs *qbs__tostr(uint8_t value, int32_t digits, int32_t passed) {
    (void)digits;
    (void)passed;
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%u", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}

// floating points
qbs *qbs__tostr(float value, int32_t digits, int32_t passed) {
    if (passed) {
        if (digits < 0) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return qbs_new_txt("");
        }
        if (digits < 1)
            digits = 1;
        if (digits > 7)
            digits = 7;
    } else {
        digits = 7;
    }
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%.*G", digits, value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}

qbs *qbs__tostr(double value, int32_t digits, int32_t passed) {
    if (passed) {
        if (digits < 0) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return qbs_new_txt("");
        }
        if (digits < 1)
            digits = 1;
        if (digits > 16)
            digits = 16;
    } else {
        digits = 16;
    }
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%.*G", digits, value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    } else {
        char *ex = strrchr((char *)tqbs->chr, (int)'E');
        if (ex != NULL)
            ex[0] = 'D';
    }
    return tqbs;
}

qbs *qbs__tostr(long double value, int32_t digits, int32_t passed) {
    if (passed) {
        if (digits < 0) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return qbs_new_txt("");
        }
        if (digits < 1)
            digits = 1;
        if (digits > 19)
            digits = 19;
    } else {
        digits = 19;
    }
    qbs *tqbs = qbs_new(32, 1);
#ifdef QB64_MINGW
    tqbs->len = __mingw_snprintf((char *)tqbs->chr, 32, "%.*LG", digits, value);
#else
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%.*LG", digits, value);
#endif
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    } else {
        char *ex = strrchr((char *)tqbs->chr, (int)'E');
        if (ex != NULL)
            ex[0] = 'F';
    }
    return tqbs;
}
