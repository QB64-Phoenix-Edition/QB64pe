//-----------------------------------------------------------------------------------------------------
//  QB64-PE Hashing Library
//  Powered by FreeType (https://freetype.org/)
//-----------------------------------------------------------------------------------------------------

#include "hashing.h"
#include "libqb-common.h"
#include "qbs.h"
extern "C" {
#include "../video/font/freetype/md5.h"
}
#include <cstdio>

/// @brief Expose freetype's MD5 procedure for public use
/// @param text The message to build the MD5 hash of
/// @return The generated MD5 hash as hexadecimal string
qbs *func__md5(qbs *text) {
    MD5_CTX ctx;
    unsigned char md5[16];

    MD5_Init(&ctx);

    if (text->len) {
        MD5_Update(&ctx, text->chr, text->len);
    }

    MD5_Final(md5, &ctx);

    auto res = qbs_new(32, 1);

    for (auto i = 0; i < 16; i++) {
        sprintf((char *)&res->chr[i * 2], "%02X", md5[i]);
    }

    return res;
}
