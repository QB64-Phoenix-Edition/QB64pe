//-----------------------------------------------------------------------------------------------------
//  QB64-PE Compression Library
//  Powered by miniz (https://github.com/richgel999/miniz)
//-----------------------------------------------------------------------------------------------------

#include "compression.h"
#include "hashing.h"
#include "libqb-common.h"
#include "miniz.h"
#include "qbs.h"
#include <vector>

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

/// @brief Compresses a string using the DEFLATE algorithm.
/// @param text The qbs object containing the string to be compressed.
/// @param level The compression level (0-10). 10 is the highest level and 0 is no compression.
/// @param passed Flag indicating if level was passed by the caller.
/// @return A new qbs object containing the compressed data.
qbs *func__deflate(qbs *src, int32_t level, int32_t passed) {
    auto fileSize = uLongf(src->len);
    auto compSize = compressBound(fileSize);
    auto dest = qbs_new(compSize, 1); // compressing directly to the qbs gives us a performance boost

    if (!passed) {
        level = MZ_DEFAULT_COMPRESSION; // set default compression level
    }

    compress2(dest->chr, &compSize, src->chr, fileSize, level); // discard result because we do not do any error checking

    return qbs_left(dest, compSize); // resize the qbs to the actual compressed size
}

/// @brief Decompresses a string using the INFLATE algorithm.
/// @param text The qbs object containing the compressed data.
/// @param originalSize The expected original size of the uncompressed data.
/// @param passed Flag indicating if originalSize was passed by the caller.
/// @return A new qbs object containing the uncompressed data.
qbs *func__inflate(qbs *text, int64_t originalSize, int32_t passed) {
    if (passed) {
        // Passing negative values can do bad things to qbs
        if (originalSize > 0) {
            auto uncompSize = uLongf(originalSize);
            auto dest = qbs_new(uncompSize, 1);                               // decompressing directly to the qbs gives us a performance boost
            uncompress(dest->chr, &uncompSize, text->chr, uLongf(text->len)); // discard result because we do not do any error checking
            return dest;                                                      // no size adjustment is done assuming the exact original size was passed
        } else {
            return qbs_new(0, 1); // simply return an empty qbs if originalSize is zero or negative
        }
    } else {
        static const uLongf InflateChunkSize = 10 * 1024 * 1024;
        std::vector<Byte> dest; // to get rid of malloc() and free()
        auto compSize = uLongf(text->len);
        uLongf uncompSize = 0;
        do {
            uncompSize += InflateChunkSize; // 10 mb original buffer, resized by 10 mb each pass until it's large enough to hold the uncompressed data.
            dest.resize(uncompSize);
        } while (uncompress(&dest[0], &uncompSize, text->chr, compSize) == Z_BUF_ERROR); // and try again with a larger buffer
        auto ret = qbs_new(uncompSize, 1);
        memcpy(ret->chr, &dest[0], uncompSize);
        return ret;
    }
}
