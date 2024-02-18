#pragma once

#include <stdint.h>

#include "qbs.h"

qbs *func_mksmbf(float val);
qbs *func_mkdmbf(double val);
float func_cvsmbf(qbs *str);
double func_cvdmbf(qbs *str);

qbs *b2string(char v);
qbs *ub2string(char v);
qbs *i2string(int16_t v);
qbs *ui2string(int16_t v);
qbs *l2string(int32_t v);
qbs *ul2string(uint32_t v);
qbs *i642string(int64_t v);
qbs *ui642string(uint64_t v);
qbs *s2string(float v);
qbs *d2string(double v);
qbs *f2string(long double v);
qbs *o2string(intptr_t v);
qbs *uo2string(uintptr_t v);
qbs *bit2string(uint32_t bsize, int64_t v);
qbs *ubit2string(uint32_t bsize, uint64_t v);

char string2b(qbs *str);
uint8_t string2ub(qbs *str);
int16_t string2i(qbs *str);
uint16_t string2ui(qbs *str);
int32_t string2l(qbs *str);
uint32_t string2ul(qbs *str);
int64_t string2i64(qbs *str);
uint64_t string2ui64(qbs *str);
float string2s(qbs *str);
double string2d(qbs *str);
long double string2f(qbs *str);
intptr_t string2o(qbs *str);
uintptr_t string2uo(qbs *str);
uint64_t string2ubit(qbs *str, uint32_t bsize);
int64_t string2bit(qbs *str, uint32_t bsize);
