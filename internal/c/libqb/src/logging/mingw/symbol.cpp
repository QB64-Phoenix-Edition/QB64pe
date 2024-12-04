
#include "libqb-common.h"

#include <windows.h>
#include <winnt.h>
#include <cstring>
#include <string>
#include <stdio.h>

#include "pe.hpp"
#include "file.hpp"
#include "pe_symtab.hpp"

#include "logging.h"
#include "../logging_private.h"

std::string libqb_log_resolve_symbol(const void *addr) {
    char utf8_name[4 * 1024] = { 0 };

    MEMORY_BASIC_INFORMATION mbinfo;
    std::memset(&mbinfo, 0, sizeof mbinfo);

    // Use VirtualQuery to find the address at which the module containing the program_counter
    // was loaded.
    if (VirtualQuery(addr, &mbinfo, static_cast<DWORD>(sizeof mbinfo)) == sizeof mbinfo)
    {
        HMODULE mod = static_cast<HMODULE>(mbinfo.AllocationBase);
        wchar_t utf16_name[4 * 1024] = L"";

        const DWORD buffsize = sizeof utf16_name / sizeof *utf16_name;
        DWORD len = GetModuleFileNameW(mod, utf16_name, buffsize);

        if (len == buffsize)
            utf16_name[--len] = L'\0';

        // Load debug info
        pe_symtab symtab;

        try
        {
            file pe(utf16_name);
            pe_symtab temp(mod, pe);
            temp.swap(symtab);
        }
        catch (const std::exception &ex)
        {
            (void)ex;
        }

        const char *symbol_name = symtab.function_spanning(addr);
        if (symbol_name)
            return symbol_name;

        // Store it as UTF-8.
        if (len == 0 ||
            WideCharToMultiByte(CP_UTF8, 0, utf16_name, -1, utf8_name, sizeof utf8_name, 0, 0) == 0)
        {
            utf8_name[0] = '\0';
        }
    }

    return utf8_name;
}
