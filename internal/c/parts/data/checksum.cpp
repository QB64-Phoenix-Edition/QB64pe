//-----------------------------------------------------------------------------------------------------
//  QB64-PE Checksum Library
//  Powered by miniz (https://github.com/richgel999/miniz)
//-----------------------------------------------------------------------------------------------------

#include "checksum.h"
#include "libqb-common.h"
#include "miniz.h"
#include "qbs.h"

/// @brief Computes the Adler-32 checksum of the given text.
/// @param text Pointer to the qbs structure containing the text data.
/// @return The Adler-32 checksum as a uint32_t value.
uint32_t func__adler32(qbs *text) {
    if (!text->len) {
        return 1;
    }

    return adler32(1, text->chr, text->len);
}

/// @brief Computes the CRC-32 checksum of the given text.
/// @param text Pointer to the qbs structure containing the text data.
/// @return The CRC-32 checksum as a uint32_t value.
uint32_t func__crc32(qbs *text) {
    if (!text->len) {
        return 0;
    }

    return crc32(0, text->chr, text->len);
}
