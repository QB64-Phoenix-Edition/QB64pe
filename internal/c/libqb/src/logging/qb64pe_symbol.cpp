
#include "libqb-common.h"

#include <string.h>
#include <stdlib.h>
#include <string>
#include <unwind.h>

#include "logging.h"
#include "logging_private.h"

std::optional<std::string> libqb_log_resolve_qb64_symbol(const char *symbol) {
    const char *qbsym = nullptr;

    if (strncmp(symbol, "SUB_", 4) == 0)
        qbsym = symbol + 4;
    else if (strncmp(symbol, "FUNC_", 5) == 0)
        qbsym = symbol + 5;

    if (strncmp(symbol, "QBMAIN(", 7) == 0)
        return "Main QB64 code";

    if (!qbsym)
        return std::nullopt;

    std::string ret = qbsym;

    return ret + " (QB64)";
}
