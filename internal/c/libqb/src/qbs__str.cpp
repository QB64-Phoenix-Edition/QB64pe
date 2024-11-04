
#include "libqb-common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_handle.h"
#include "qbs.h"

// modern _STR() functions (no leading space and no QB4.5 compatible rounding)
// signed integers
qbs *qbs__str(int64_t value) {
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%" PRId64, value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}
qbs *qbs__str(int32_t value) {
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%i", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}
qbs *qbs__str(int16_t value) {
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%i", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}
qbs *qbs__str(int8_t value) {
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%i", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}
// unsigned integers
qbs *qbs__str(uint64_t value) {
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%" PRIu64, value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}
qbs *qbs__str(uint32_t value) {
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%u", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}
qbs *qbs__str(uint16_t value) {
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%u", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}
qbs *qbs__str(uint8_t value) {
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%u", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}
// floating points
qbs *qbs__str(float value) {
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%.7G", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    return tqbs;
}
qbs *qbs__str(double value) {
    qbs *tqbs = qbs_new(32, 1);
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%.16G", value);
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    char *ex = strrchr((char*)tqbs->chr, (int)'E');
    if (ex != NULL) ex[0] = 'D';
    return tqbs;
}
qbs *qbs__str(long double value) {
    qbs *tqbs = qbs_new(32, 1);
#ifdef QB64_MINGW
    tqbs->len = __mingw_snprintf((char *)tqbs->chr, 32, "%.19LG", value);
#else
    tqbs->len = snprintf((char *)tqbs->chr, 32, "%.19LG", value);
#endif
    if (tqbs->len < 0 || tqbs->len >= 32) {
        error(QB_ERROR_INTERNAL_ERROR);
        tqbs->len = 0;
    }
    char *ex = strrchr((char*)tqbs->chr, (int)'E');
    if (ex != NULL) ex[0] = 'F';
    return tqbs;
}
