//-----------------------------------------------------------------------------------------------------
//  QB64-PE Checksum Library
//  Powered by miniz (https://github.com/richgel999/miniz)
//-----------------------------------------------------------------------------------------------------

#include "checksum.h"
#include "libqb-common.h"
#include "miniz.h"
#include "qbs.h"

uint32_t func__adler32(qbs *text) {
    if (!text->len) {
        return 1;
    }

    return (uint32_t)adler32(1, text->chr, text->len);
}

uint32_t func__crc32(qbs *text) {
    if (!text->len) {
        return 0;
    }

    return (uint32_t)crc32(0, text->chr, text->len);
}
