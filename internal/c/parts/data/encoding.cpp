//-----------------------------------------------------------------------------------------------------
//  QB64-PE Encoding Library
//  Powered by MODP_B64 (https://github.com/client9/stringencoders)
//-----------------------------------------------------------------------------------------------------

#include "encoding.h"
#include "error_handle.h"
#include "libqb-common.h"
#include "modp_b64.h"
#include "qbs.h"

/// @brief Encodes the given qbs object to Base64 format.
/// @param src The qbs object containing the data to be encoded.
/// @return A new qbs object containing the Base64 encoded data.
qbs *func__base64encode(qbs *src) {
    auto dst = qbs_new(modp_b64_encode_data_len(src->len), 1);
    return qbs_left(dst, modp_b64_encode_data(reinterpret_cast<char *>(dst->chr), reinterpret_cast<const char *>(src->chr), src->len));
}

/// @brief Decodes the given qbs object from Base64 format.
/// @param src The qbs object containing the Base64 encoded data.
/// @return A new qbs object containing the decoded data.
qbs *func__base64decode(qbs *src) {
    auto dst = qbs_new(modp_b64_decode_len(src->len), 1);
    auto outputSize = modp_b64_decode(reinterpret_cast<char *>(dst->chr), reinterpret_cast<const char *>(src->chr), src->len);
    return outputSize == MODP_B64_ERROR ? qbs_left(dst, 0) : qbs_left(dst, outputSize);
}
