
#include "libqb-common.h"

#include <cstring>
#include <string>
#include <stdio.h>
#include <dlfcn.h>

#include <cxxabi.h>

#include "logging.h"
#include "../logging_private.h"

std::string libqb_log_resolve_symbol(const void *addr) {
    Dl_info info;
    memset(&info, 0, sizeof(info));

    if (dladdr(addr, &info) != 0) {
        if (info.dli_sname && *info.dli_sname) {
            char namebuf[1024];
            size_t len = sizeof(namebuf);
            int status = 0;

            char *demangled = abi::__cxa_demangle(info.dli_sname, namebuf, &len, &status);

            if (status != 0)
                return info.dli_sname;

            return demangled;
        } else if (info.dli_fname) {
            return info.dli_fname;
        }
    }

    return "none";
}
