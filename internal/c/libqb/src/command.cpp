
#include "libqb-common.h"

#include <stdlib.h>
#include <string.h>

#include "command.h"

qbs *func_command_str = NULL;
static char **func_command_array = NULL;
static int32_t func_command_count = 0;

qbs *func_command(int32_t index, int32_t passed) {
    static qbs *tqbs;
    if (passed) { // Get specific parameter
        // If out of bounds or error getting cmdline args, return empty string.
        if (index >= func_command_count || index < 0 || func_command_array == NULL) {
            tqbs = qbs_new(0, 1);
            return tqbs;
        }
        int len = strlen(func_command_array[index]);
        // Create new temp qbs and copy data into it.
        tqbs = qbs_new(len, 1);
        memcpy(tqbs->chr, func_command_array[index], len);
    } else { // Legacy support; return whole commandline
        tqbs = qbs_new(func_command_str->len, 1);
        memcpy(tqbs->chr, func_command_str->chr, func_command_str->len);
    }
    return tqbs;
}

int32_t func__commandcount() {
    return func_command_count - 1;
}

void command_initialize(int argc, char **argv) {
    int i = argc;
    if (i > 1) {
        // calculate required size of COMMAND$ string
        int i2 = 0;
        for (i = 1; i < argc; i++) {
            i2 += strlen(argv[i]);
            if (i != 1)
                i2++; // for a space
        }
        // create COMMAND$ string
        func_command_str = qbs_new(i2, 0);
        // build COMMAND$ string
        int i3 = 0;
        for (i = 1; i < argc; i++) {
            if (i != 1) {
                func_command_str->chr[i3] = 32;
                i3++;
            }
            memcpy(&func_command_str->chr[i3], argv[i], strlen(argv[i]));
            i3 += strlen(argv[i]);
        }
    } else {
        func_command_str = qbs_new(0, 0);
    }

    func_command_count = argc;
    func_command_array = argv;
}
