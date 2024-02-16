
#include "libqb-common.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "qbs.h"

// STR() functions
// singed integers
qbs *qbs_str(int64_t value) {
    qbs *tqbs;
    tqbs = qbs_new(20, 1);
#ifdef QB64_WINDOWS
    tqbs->len = sprintf((char *)tqbs->chr, "% I64i", value);
#else
    tqbs->len = sprintf((char *)tqbs->chr, "% lli", value);
#endif
    return tqbs;
}
qbs *qbs_str(int32_t value) {
    qbs *tqbs;
    tqbs = qbs_new(11, 1);
    tqbs->len = sprintf((char *)tqbs->chr, "% i", value);
    return tqbs;
}
qbs *qbs_str(int16_t value) {
    qbs *tqbs;
    tqbs = qbs_new(6, 1);
    tqbs->len = sprintf((char *)tqbs->chr, "% i", value);
    return tqbs;
}
qbs *qbs_str(int8_t value) {
    qbs *tqbs;
    tqbs = qbs_new(4, 1);
    tqbs->len = sprintf((char *)tqbs->chr, "% i", value);
    return tqbs;
}
// unsigned integers
qbs *qbs_str(uint64_t value) {
    qbs *tqbs;
    tqbs = qbs_new(21, 1);
#ifdef QB64_WINDOWS
    tqbs->len = sprintf((char *)tqbs->chr, " %I64u", value);
#else
    tqbs->len = sprintf((char *)tqbs->chr, " %llu", value);
#endif
    return tqbs;
}
qbs *qbs_str(uint32_t value) {
    qbs *tqbs;
    tqbs = qbs_new(11, 1);
    tqbs->len = sprintf((char *)tqbs->chr, " %u", value);
    return tqbs;
}
qbs *qbs_str(uint16_t value) {
    qbs *tqbs;
    tqbs = qbs_new(6, 1);
    tqbs->len = sprintf((char *)tqbs->chr, " %u", value);
    return tqbs;
}
qbs *qbs_str(uint8_t value) {
    qbs *tqbs;
    tqbs = qbs_new(4, 1);
    tqbs->len = sprintf((char *)tqbs->chr, " %u", value);
    return tqbs;
}

uint8_t func_str_fmt[7];
uint8_t qbs_str_buffer[32];
uint8_t qbs_str_buffer2[32];

qbs *qbs_str(float value) {
    static qbs *tqbs;
    tqbs = qbs_new(16, 1);
    static int32_t l, i, i2, i3, digits, exponent;
    l = sprintf((char *)&qbs_str_buffer, "% .6E", value);
    // IMPORTANT: assumed l==14
    if (l == 13) {
        memmove(&qbs_str_buffer[12], &qbs_str_buffer[11], 2);
        qbs_str_buffer[11] = 48;
        l = 14;
    }

    digits = 7;
    for (i = 8; i >= 1; i--) {
        if (qbs_str_buffer[i] == 48) {
            digits--;
        } else {
            if (qbs_str_buffer[i] != 46)
                break;
        }
    } // i
    // no significant digits? simply return 0
    if (digits == 0) {
        tqbs->len = 2;
        tqbs->chr[0] = 32;
        tqbs->chr[1] = 48; // tqbs=[space][0]
        return tqbs;
    }
    // calculate exponent
    exponent = (qbs_str_buffer[11] - 48) * 100 + (qbs_str_buffer[12] - 48) * 10 + (qbs_str_buffer[13] - 48);
    if (qbs_str_buffer[10] == 45)
        exponent = -exponent;
    if ((exponent <= 6) && ((exponent - digits) >= -8))
        goto asdecimal;
    // fix up exponent to conform to QBASIC standards
    // i. cull trailing 0's after decimal point (use digits to help)
    // ii. cull leading 0's of exponent

    i3 = 0;
    i2 = digits + 2;
    if (digits == 1)
        i2--; // don't include decimal point
    for (i = 0; i < i2; i++) {
        tqbs->chr[i3] = qbs_str_buffer[i];
        i3++;
    }
    for (i = 9; i <= 10; i++) {
        tqbs->chr[i3] = qbs_str_buffer[i];
        i3++;
    }
    exponent = abs(exponent);
    // i2=13;
    // if (exponent>9) i2=12;
    i2 = 12; // override: if exponent is less than 10 still display a leading 0
    if (exponent > 99)
        i2 = 11;
    for (i = i2; i <= 13; i++) {
        tqbs->chr[i3] = qbs_str_buffer[i];
        i3++;
    }
    tqbs->len = i3;
    return tqbs;
/////////////////////
asdecimal:
    // calculate digits after decimal point in var. i
    i = -(exponent - digits + 1);
    if (i < 0)
        i = 0;
    func_str_fmt[0] = 37; //"%"
    func_str_fmt[1] = 32; //" "
    func_str_fmt[2] = 46; //"."
    func_str_fmt[3] = i + 48;
    func_str_fmt[4] = 102; //"f"
    func_str_fmt[5] = 0;
    tqbs->len = sprintf((char *)tqbs->chr, (const char *)&func_str_fmt, value);
    if (tqbs->chr[1] == 48) { // must manually cull leading 0
        memmove(tqbs->chr + 1, tqbs->chr + 2, tqbs->len - 2);
        tqbs->len--;
    }
    return tqbs;
}

qbs *qbs_str(double value) {
    static qbs *tqbs;
    tqbs = qbs_new(32, 1);
    static int32_t l, i, i2, i3, digits, exponent;

    l = sprintf((char *)&qbs_str_buffer, "% .15E", value);
    // IMPORTANT: assumed l==23
    if (l == 22) {
        memmove(&qbs_str_buffer[21], &qbs_str_buffer[20], 2);
        qbs_str_buffer[20] = 48;
        l = 23;
    }

    // check if the 16th significant digit is 9, if it is round to 15 significant digits
    if (qbs_str_buffer[17] == 57) {
        sprintf((char *)&qbs_str_buffer2, "% .14E", value);
        memmove(&qbs_str_buffer, &qbs_str_buffer2, 17);
        qbs_str_buffer[17] = 48;
    }
    qbs_str_buffer[18] = 68; // change E to D (QBASIC standard)
    digits = 16;
    for (i = 17; i >= 1; i--) {
        if (qbs_str_buffer[i] == 48) {
            digits--;
        } else {
            if (qbs_str_buffer[i] != 46)
                break;
        }
    } // i
    // no significant digits? simply return 0
    if (digits == 0) {
        tqbs->len = 2;
        tqbs->chr[0] = 32;
        tqbs->chr[1] = 48; // tqbs=[space][0]
        return tqbs;
    }
    // calculate exponent
    exponent = (qbs_str_buffer[20] - 48) * 100 + (qbs_str_buffer[21] - 48) * 10 + (qbs_str_buffer[22] - 48);
    if (qbs_str_buffer[19] == 45)
        exponent = -exponent;
    // OLD if ((exponent<=15)&&((exponent-digits)>=-16)) goto asdecimal;
    if ((exponent <= 15) && ((exponent - digits) >= -17))
        goto asdecimal;
    // fix up exponent to conform to QBASIC standards
    // i. cull trailing 0's after decimal point (use digits to help)
    // ii. cull leading 0's of exponent
    i3 = 0;
    i2 = digits + 2;
    if (digits == 1)
        i2--; // don't include decimal point
    for (i = 0; i < i2; i++) {
        tqbs->chr[i3] = qbs_str_buffer[i];
        i3++;
    }
    for (i = 18; i <= 19; i++) {
        tqbs->chr[i3] = qbs_str_buffer[i];
        i3++;
    }
    exponent = abs(exponent);
    // i2=22;
    // if (exponent>9) i2=21;
    i2 = 21; // override: if exponent is less than 10 still display a leading 0
    if (exponent > 99)
        i2 = 20;
    for (i = i2; i <= 22; i++) {
        tqbs->chr[i3] = qbs_str_buffer[i];
        i3++;
    }
    tqbs->len = i3;
    return tqbs;
/////////////////////
asdecimal:
    // calculate digits after decimal point in var. i
    i = -(exponent - digits + 1);
    if (i < 0)
        i = 0;
    func_str_fmt[0] = 37; //"%"
    func_str_fmt[1] = 32; //" "
    func_str_fmt[2] = 46; //"."
    if (i > 9) {
        func_str_fmt[3] = 49; //"1"
        func_str_fmt[4] = (i - 10) + 48;
    } else {
        func_str_fmt[3] = 48; //"0"
        func_str_fmt[4] = i + 48;
    }
    func_str_fmt[5] = 102; //"f"
    func_str_fmt[6] = 0;
    tqbs->len = sprintf((char *)tqbs->chr, (const char *)&func_str_fmt, value);
    if (tqbs->chr[1] == 48) { // must manually cull leading 0
        memmove(tqbs->chr + 1, tqbs->chr + 2, tqbs->len - 2);
        tqbs->len--;
    }
    return tqbs;
}

qbs *qbs_str(long double value) {
    // not fully implemented
    return qbs_str((double)value);
}

