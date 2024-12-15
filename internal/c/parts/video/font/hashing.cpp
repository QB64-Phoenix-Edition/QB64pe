//-----------------------------------------------------------------------------------------------------
//  QB64-PE Hashing Library
//  Uses the MD5 implementation from FreeType (https://freetype.org/)
//-----------------------------------------------------------------------------------------------------

#include "hashing.h"
extern "C" {
#include "freetype/md5.h"
}
#include "libqb-common.h"
#include "qbs.h"
#include <cstdio>

/// @brief Computes the MD5 hash of the given text.
/// @param text Pointer to the qbs structure containing the text data.
/// @return A new qbs object containing the MD5 hash in hexadecimal format.
qbs *func__md5(qbs *text) {
    static const auto MD5_DIGEST_LENGTH = 16;
    static const auto MD5_DIGEST_STRING_LENGTH = MD5_DIGEST_LENGTH * 2;

    MD5_CTX ctx;
    unsigned char md5[MD5_DIGEST_LENGTH];

    MD5_Init(&ctx);

    if (text->len) {
        MD5_Update(&ctx, text->chr, text->len);
    }

    MD5_Final(md5, &ctx);

    auto result = qbs_new(MD5_DIGEST_STRING_LENGTH, 1);

    for (auto i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf((char *)&result->chr[i << 1], "%02X", md5[i]);
    }

    return result;
}
