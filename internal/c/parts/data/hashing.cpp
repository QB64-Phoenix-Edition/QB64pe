//-----------------------------------------------------------------------------------------------------
//  QB64-PE Hashing Library
//  Uses the MD5 implementation from libxmp-lite (https://github.com/libxmp/libxmp/tree/master/lite)
//-----------------------------------------------------------------------------------------------------

#include "hashing.h"
#include "../audio/extras/libxmp-lite/md5.h"
#include "libqb-common.h"
#include "qbs.h"
#include <cstdio>

/// @brief Computes the MD5 hash of the given text.
/// @param text Pointer to the qbs structure containing the text data.
/// @return A new qbs object containing the MD5 hash in hexadecimal format.
qbs *func__md5(qbs *text) {
    MD5_CTX ctx;
    unsigned char md5[MD5_DIGEST_LENGTH];

    MD5Init(&ctx);

    if (text->len) {
        MD5Update(&ctx, text->chr, text->len);
    }

    MD5Final(md5, &ctx);

    auto result = qbs_new(MD5_DIGEST_STRING_LENGTH - 1, 1);

    for (auto i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf((char *)&result->chr[i << 1], "%02X", md5[i]);
    }

    return result;
}
