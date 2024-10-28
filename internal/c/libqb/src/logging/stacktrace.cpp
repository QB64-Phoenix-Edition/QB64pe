
#include "libqb-common.h"

#include <string>
#include <string.h>
#include <stdio.h>
#include <unwind.h>

#include "logging.h"
#include "logging_private.h"

struct stacktrace_state {
    bool qb64_only;
    bool started_qb64_stack;
    int frame;
    int skipped;
    std::string str;
};

static bool startsWith(const std::string &l, const char *r) {
    size_t rlen = strlen(r);
    if (l.size() < rlen)
        return false;

    return strncmp(l.c_str(), r, rlen) == 0;
}

static _Unwind_Reason_Code handler(struct _Unwind_Context *context, void *ref) {
    stacktrace_state *result = static_cast<stacktrace_state *>(ref);

	if (result->frame > 40)
		return _URC_NORMAL_STOP;

    result->frame++;

    const void *addr = (const void *)_Unwind_GetIP(context);
    if (!addr)
        return _URC_NORMAL_STOP;

    std::string symbol = libqb_log_resolve_symbol(addr);

    // Remove these symbols from the stacktrace
    if (startsWith(symbol, "libqb_log")
        || startsWith(symbol, "libqb_vlog")) {

        result->skipped++;
        return _URC_NO_REASON;
    }

    std::optional<std::string> qb64_symbol = libqb_log_resolve_qb64_symbol(symbol.c_str());

    if (qb64_symbol.has_value()) {
        symbol = *qb64_symbol;
        result->started_qb64_stack = true;
    } else if (result->qb64_only) {
        if (result->started_qb64_stack)
            return _URC_NORMAL_STOP;
        else
            return _URC_NO_REASON;
    }

    char line[200];
    snprintf(line, sizeof(line), "#%-2d [0x%p] in %s\n", result->frame - result->skipped, addr, symbol.c_str());

    result->str += line;
	return _URC_NO_REASON;
}

std::string libqb_log_get_stacktrace(bool qb64_only) {
    stacktrace_state state = {
        .qb64_only = qb64_only,
        .started_qb64_stack = false,
        .frame = 0,
        .skipped = 0,
        .str = "",
    };

    _Unwind_Backtrace(&handler, &state);

    return state.str;
}
