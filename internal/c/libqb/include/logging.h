#pragma once

#include <stdint.h>

#include "qbs.h"

enum class loglevel {
    Trace,
    Information,
    Warning,
    Error,
};

enum class logscope {
    Runtime,
    QB64,
    Libqb,
    Audio,
    Image,

    Count,
};

void libqb_log_init();
void libqb_log(loglevel lvl, logscope scope, const char *file, const char *func, int line, const char *fmt, ...);
void libqb_log_qb64(loglevel lvl, logscope scope, const char *file, const char *func, int line, const char *fmt, ...);
void libqb_log_qbs(loglevel lvl, logscope scope, const char *file, const char *func, int line, qbs *str);

// Returns 1 to 4, indicating a min level of Trace to Error
uint32_t func__logminlevel();


#define libqb_log_with_scope_trace(scope, fmt, ...) \
    libqb_log(loglevel::Trace, (scope), __FILE__, __func__, __LINE__, fmt, ## __VA_ARGS__)

#define libqb_log_with_scope_info(scope, fmt, ...) \
    libqb_log(loglevel::Information, (scope), __FILE__, __func__, __LINE__, fmt, ## __VA_ARGS__)

#define libqb_log_with_scope_warn(scope, fmt, ...) \
    libqb_log(loglevel::Warning, (scope), __FILE__, __func__, __LINE__, fmt, ## __VA_ARGS__)

#define libqb_log_with_scope_error(scope, fmt, ...) \
    libqb_log(loglevel::Error, (scope), __FILE__, __func__, __LINE__, fmt, ## __VA_ARGS__)


#define libqb_log_trace(...) \
    libqb_log_with_scope_trace(logscope::Libqb, __VA_ARGS__)

#define libqb_log_info(...) \
    libqb_log_with_scope_info(logscope::Libqb, __VA_ARGS__)

#define libqb_log_warn(...) \
    libqb_log_with_scope_warn(logscope::Libqb, __VA_ARGS__)

#define libqb_log_error(...) \
    libqb_log_with_scope_error(logscope::Libqb, __VA_ARGS__)


#define sub__logtrace(file, func, line, qbs) \
    libqb_log_qbs(loglevel::Trace, logscope::QB64, file, func, line, (qbs))

#define sub__loginfo(file, func, line, qbs) \
    libqb_log_qbs(loglevel::Information, logscope::QB64, file, func, line, (qbs))

#define sub__logwarn(file, func, line, qbs) \
    libqb_log_qbs(loglevel::Warning, logscope::QB64, file, func, line, (qbs))

#define sub__logerror(file, func, line, qbs) \
    libqb_log_qbs(loglevel::Error, logscope::QB64, file, func, line, (qbs))
