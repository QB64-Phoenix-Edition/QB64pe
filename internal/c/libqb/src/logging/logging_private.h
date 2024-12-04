#pragma once

#include <string>
#include <optional>
#include "logging.h"

#define runtime_log_trace(...) \
    libqb_log_with_scope_trace(logscope::Runtime, __VA_ARGS__)

#define runtime_log_info(fmt, ...) \
    libqb_log_with_scope_info(logscope::Runtime, __VA_ARGS__)

#define runtime_log_warn(fmt, ...) \
    libqb_log_with_scope_warn(logscope::Runtime, __VA_ARGS__)

#define runtime_log_error(fmt, ...) \
    libqb_log_with_scope_error(logscope::Runtime, __VA_ARGS__)


struct log_entry {
    loglevel level;
    logscope scope;
    double timestamp; // Timed from beginning of program
    std::string message;
    std::string func;
    const char *file;
    int line;

    std::string stacktrace;
};

class log_handler {
  public:
    virtual void write(struct log_entry *) = 0;
};


const char *logLevelName(loglevel lvl);
const char *logScopeName(logscope scope);

std::optional<std::string> libqb_log_resolve_qb64_symbol(const char *symbol);

std::string libqb_log_get_stacktrace(bool qb64_only);

// Returns the demangled name for the symbol at `address`.
std::string libqb_log_resolve_symbol(const void *address);

class fp_log_writer : public log_handler {
  protected:
    FILE *fp;

  public:
    virtual void write(struct log_entry *entry);
};

class console_log_handler : public fp_log_writer {
  public:
    console_log_handler();
};

class file_log_handler : public fp_log_writer {
  public:
    file_log_handler();
};
