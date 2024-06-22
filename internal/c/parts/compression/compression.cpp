//----------------------------------------------------------------------------------------------------
//  QB64-PE Compression Library
//  Powered by miniz (https://github.com/richgel999/miniz)
//-----------------------------------------------------------------------------------------------------

#include "libqb-common.h"

#include "compression.h"
#include "qbs.h"

#include "miniz.h"

#include <vector>

uint32_t func__adler32(qbs *text) {
    if (!text->len)
        return 1;
    return (uint32_t)adler32(1, text->chr, text->len);
}

uint32_t func__crc32(qbs *text) {
    if (!text->len)
        return 0;
    return (uint32_t)crc32(0, text->chr, text->len);
}

qbs *func__deflate(qbs *text) {
    auto fileSize = uLongf(text->len);
    auto compSize = compressBound(fileSize);
    auto dest = qbs_new(compSize, 1);                    // compressing directly to the qbs gives us a performance boost
    compress(dest->chr, &compSize, text->chr, fileSize); // discard result because we do not do any error checking
    return qbs_left(dest, compSize);
}

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
