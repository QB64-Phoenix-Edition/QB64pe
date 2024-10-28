
#include "libqb-common.h"

#include <string.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <stdio.h>

#include "logging.h"
#include "../logging_private.h"

void fp_log_writer::write(struct log_entry *entry) {
    if (!fp)
        return;

    const char *scope = logScopeName(entry->scope);

    // The main code may not have a file associated with it when compiled from
    // the IDE
    if (entry->file && *entry->file)
        fprintf(fp, "[%0.5lf] %s %s %s: %s: %d: %s\n",
                entry->timestamp,
                logLevelName(entry->level),
                scope? scope: "",
                entry->file,
                entry->func.c_str(),
                entry->line,
                entry->message.c_str());
    else
        fprintf(fp, "[%0.5lf] %s %s %s: %d: %s\n",
                entry->timestamp,
                logLevelName(entry->level),
                scope? scope: "",
                entry->func.c_str(),
                entry->line,
                entry->message.c_str());

    if (entry->stacktrace != "")
        fprintf(fp, "%s", entry->stacktrace.c_str());
}

console_log_handler::console_log_handler() {
    fp = stderr;
}

file_log_handler::file_log_handler() {
    const char *filepath = getenv("QB64PE_LOG_FILE_PATH");

    fp = fopen(filepath, "a");
    if (!fp)
        fprintf(stderr, "Unable to open file '%s' for logging!", filepath);
}
