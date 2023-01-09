
#include "libqb-common.h"

#include "rounding.h"

#ifdef QB64_NOT_X86
int64_t qbr(long double f) {
    int64_t i;
    int temp = 0;
    if (f > 9223372036854775807) {
        temp = 1;
        f = f - 9223372036854775808u;
    } // if it's too large for a signed int64, make it an unsigned int64 and return that value if possible.
    if (f < 0)
        i = f - 0.5f;
    else
        i = f + 0.5f;
    if (temp)
        return i | 0x8000000000000000; //+9223372036854775808;
    return i;
}
uint64_t qbr_longdouble_to_uint64(long double f) {
    if (f < 0)
        return (f - 0.5f);
    else
        return (f + 0.5f);
}
int32_t qbr_float_to_long(float f) {
    if (f < 0)
        return (f - 0.5f);
    else
        return (f + 0.5f);
}
int32_t qbr_double_to_long(double f) {
    if (f < 0)
        return (f - 0.5f);
    else
        return (f + 0.5f);
}
void fpu_reinit() {} // do nothing
#else
// QBASIC compatible rounding via FPU:
// FLDS=load single
// FLDL=load double
// FLDT=load long double
int64_t qbr(long double f) {
    int64_t i;
    int temp = 0;
    if (f > 9223372036854775807) {
        temp = 1;
        f = f - 9223372036854775808u;
    } // if it's too large for a signed int64, make it an unsigned int64 and return that value if possible.
    __asm__("fldt %1;"
            "fistpll %0;"
            : "=m"(i)
            : "m"(f));
    if (temp)
        return i | 0x8000000000000000; // if it's an unsigned int64, manually set the bit flag
    return i;
}
uint64_t qbr_longdouble_to_uint64(long double f) {
    uint64_t i;
    __asm__("fldt %1;"
            "fistpll %0;"
            : "=m"(i)
            : "m"(f));
    return i;
}
int32_t qbr_float_to_long(float f) {
    int32_t i;
    __asm__("flds %1;"
            "fistpl %0;"
            : "=m"(i)
            : "m"(f));
    return i;
}
int32_t qbr_double_to_long(double f) {
    int32_t i;
    __asm__("fldl %1;"
            "fistpl %0;"
            : "=m"(i)
            : "m"(f));
    return i;
}
void fpu_reinit() {
    unsigned int mode = 0x37F;
    asm("fldcw %0" : : "m"(*&mode));
}
#endif // x86 support


