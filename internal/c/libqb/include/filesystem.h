#pragma once

#include <stdint.h>

struct qbs;

extern qbs *startDir;

qbs *func__cwd();
qbs *func__dir(qbs *context);
int32_t func__direxists(qbs *);
int32_t func__fileexists(qbs *);
qbs *func__startdir();
void sub_chdir(qbs *str);
void sub_files(qbs *str, int32_t passed);
void sub_kill(qbs *str);
void sub_mkdir(qbs *str);
void sub_name(qbs *oldname, qbs *newname);
void sub_rmdir(qbs *str);
