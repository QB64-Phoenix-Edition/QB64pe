
#include "libqb-common.h"

#include <stdlib.h>
#include <string.h>

#include "error_handle.h"
#include "qbs.h"
#include "environ.h"

#ifdef QB64_WINDOWS
#    define envp _environ
#else
extern char **environ;
#    define envp environ
#endif

int32_t func__environcount() {
    // count array bound
    char **p = envp;
    while (*++p)
        ;
    return p - envp;
}

qbs *func_environ(qbs *name) {
    char *query, *result;
    qbs *tqbs;
    query = (char *)malloc(name->len + 1);
    query[name->len] = '\0'; // add NULL terminator
    memcpy(query, name->chr, name->len);
    result = getenv(query);
    if (result) {
        int result_length = strlen(result);
        tqbs = qbs_new(result_length, 1);
        memcpy(tqbs->chr, result, result_length);
    } else {
        tqbs = qbs_new(0, 1);
    }
    return tqbs;
}

qbs *func_environ(int32_t number) {
    char *result;
    qbs *tqbs;
    int result_length;
    if (number <= 0) {
        tqbs = qbs_new(0, 1);
        error(5);
        return tqbs;
    }
    // Check we do not go beyond array bound
    char **p = envp;
    while (*++p)
        ;
    if (number > p - envp) {
        tqbs = qbs_new(0, 1);
        return tqbs;
    }
    result = envp[number - 1];
    result_length = strlen(result);
    tqbs = qbs_new(result_length, 1);
    memcpy(tqbs->chr, result, result_length);
    return tqbs;
}

void sub_environ(qbs *str) {
    char *buf;
    char *separator;
    buf = (char *)malloc(str->len + 1);
    buf[str->len] = '\0';
    memcpy(buf, str->chr, str->len);
    // Name and value may be separated by = or space
    separator = strchr(buf, ' ');
    if (!separator) {
        separator = strchr(buf, '=');
    }
    if (!separator) {
        // It is an error is there is no separator
        error(5);
        return;
    }
    // Split into two separate strings
    *separator = '\0';
    if (separator == &buf[str->len] - 1) {
        // Separator is at end of string, so remove the variable
#ifdef QB64_WINDOWS
        *separator = '=';
        _putenv(buf);
#else
        unsetenv(buf);
#endif
    } else {
#ifdef QB64_WINDOWS
#    if WINVER >= 0x0600
        _putenv_s(buf, separator + 1);
#    else
        *separator = '=';
        _putenv(buf);
#    endif
#else
        setenv(buf, separator + 1, 1);
#endif
    }
    free(buf);
}
