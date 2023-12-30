//-----------------------------------------------------------------------------------------------------
//  QB64-PE filesystem related functions
//-----------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

struct qbs;

/// @brief This is a global variable that is set on startup and holds the directory that was current when the program was loaded
extern qbs *g_startDir;

qbs *func__cwd();
qbs *func__dir(qbs *qbsContext);
int32_t func__direxists(qbs *path);
int32_t func__fileexists(qbs *path);
qbs *func__startdir();
void sub_chdir(qbs *str);
qbs *func__files(qbs *qbsFileSpec, int32_t passed);
qbs *func__fullpath(qbs *qbsPathName);
void sub_files(qbs *str, int32_t passed);
void sub_kill(qbs *str);
void sub_mkdir(qbs *str);
void sub_name(qbs *oldname, qbs *newname);
void sub_rmdir(qbs *str);
