/**
 * JebP - Single header WebP decoder
 */

/**
 * LICENSE
 **
 * MIT No Attribution
 *
 * Copyright 2022 Jasmine Minter
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Attribution is not required, but would be appreciated :)

/**
 * DOCUMENTATION
 **
 * First and foremost, this project uses some custom types:
 *   `jebp_byte`/`jebp_ubyte` is a singular byte.
 *   `jebp_short`/jebp_ushort` is an integer of atleast 16-bits.
 *   `jebp_int`/`jebp_uint` is an integer of atleast 32-bits.
 *
 * This is a header only file. This means that it operates as a standard header
 * and to generate the source file you define `JEBP_IMPLEMENTATION` in ONE file
 * only. For example:
 * ```c
 * #define JEBP_IMPLEMENTATION
 * #include "jebp.h"
 * ```
 *
 * The most basic API call in this library is:
 * ```c
 * err = jebp_decode(&image, size, data);
 * ```
 * where:
 *   `jebp_image_t *image` is a pointer to an image structure to receive the
 *                         decoded data.
 *   `size_t size` is the size of the WebP-encoded data buffer.
 *   `const void *data` is a pointer to the WebP encoded data buffer, `size`
 *                      bytes large.
 *   `jebp_error_t err` is the result of the operation (OK or an error code).
 *
 * For reading from a provided file path, this API call can be used instead:
 * ```c
 * err = jebp_read(&image, path);
 * ```
 * where:
 *   `const char *path` is the path of the file to be read.
 *
 * It is currently not possible to read from a `FILE *` object.
 * If you only want to get the size of the image without a full read, these
 * functions can be used instead:
 * ```c
 * err = jebp_decode_size(&image, size, data);
 * err = jebp_read_size(&image, path);
 * ```
 *
 * The `jebp_image_t` structure has the following properties:
 *   `jebp_int width` is the width of the image.
 *   `jebp_int height` is the height of the image.
 *   `jebp_color_t *pixels` is a pointer to an array pixels. Each `jebp_color_t`
 *                          structure contains four `jebp_ubyte` values for `r`,
 *                          `g`, `b` and `a`. This allows the `pixels` pointer
 *                          to be cast to `jebp_ubyte *` to get an RGBA pixel
 *                          buffer.
 * The allocated data in the image can be free'd with:
 * ```c
 * jebp_free_image(&image);
 * ```
 * This function will also clear the structure, notably width and height will be
 * set to 0.
 *
 * The `jebp_error_t` enumeration has the following values:
 *   `JEBP_OK` means the operation completed successfully.
 *   `JEBP_ERROR_INVAL` means one of the arguments provided is invalid, usually
 *                      this refers to a NULL pointer.
 *   `JEBP_ERROR_INVDATA` means the WebP-encoded data is invalid or corrupted.
 *   `JEBP_ERROR_INVDATA_HEADER` is a suberror of `INVDATA` that indicates that
 *                      the header bytes are invalid. This file is likely not a
 *                      WebP file.
 *   `JEBP_ERROR_EOF` means the end of the file (or data buffer) was reached
 *                    before the operation could successfully complete.
 *   `JEBP_ERROR_NOSUP` means there is a feature in the WebP stream that is not
 *                      currently supported (see below). This can also represent
 *                      new features, versions or RIFF-chunks that were not in
 *                      the specification when writing.
 *   `JEBP_ERROR_NOSUP_CODEC` is a suberror of `NOSUP` that indicates that the
 *                      RIFF chunk that is most likely for the codec is not
 *                      recognized. Currently extended file formats (see below)
 *                      are not supported and both lossy and lossless codecs can
 *                      be disabled (see `JEBP_NO_VP8` and `JEBP_NO_VP8L`).
 *   `JEBP_ERROR_NOSUP_PALETTE` is a suberror of `NOSUP` that indicates that the
 *                      image has a color-index transform (in WebP terminology,
 *                      this would be a paletted image). Color-indexing
 *                      transforms are not currently supported (see below). Note
 *                      that this error code might be removed after
 *                      color-indexing transform support is added, this is only
 *                      here for now to help detecting common issues.
 *   `JEBP_ERROR_NOMEM` means that a memory allocation failed, indicating that
 *                      there is no more memory available.
 *   `JEBP_ERROR_IO` represents any generic I/O error, usually from
 *                   file-reading.
 *   `JEBP_ERROR_UNKNOWN` means any unknown error. Currently this is only used
 *                        when an unknown value is passed into
 *                        `jebp_error_string`.
 * To get a human-readable string of the error, the following function can be
 * used:
 * ```c
 * const char *error = jebp_error_string(err);
 * ```
 *
 * This is not a feature-complete WebP decoder and has the following
 * limitations:
 *   - Does not support extended file-formats with VP8X.
 *   - Does not support VP8L lossless images with the color-indexing transform
 *     (palleted images).
 *   - Does not support VP8L images with more than 256 huffman groups. This is
 *     an arbitrary limit to prevent bad images from using too much memory. In
 *     theory, images requiring more groups should be very rare. This limit may
 *     be increased in the future.
 *
 * Features that will probably never be supported due to complexity or API
 * constraints:
 *   - Decoding color profiles.
 *   - Decoding metadata.
 *   - Full color-indexing/palette support will be a bit of a mess, so don't
 *     expect full support of that coming anytime soon. Simple color-indexing
 *     support (more than 16 colors, skipping the need for bit-packing) is
 *     definitely alot more do-able.
 *
 * Along with `JEBP_IMPLEMENTATION` defined above, there are a few other macros
 * that can be defined to change how JebP operates:
 *   `JEBP_NO_STDIO` will disable the file-reading API.
 *   `JEBP_NO_SIMD` will disable SIMD optimizations. These are currently
 *                  not-used but the detection is there ready for further work.
 *   `JEBP_NO_VP8` will disable VP8 (lossy) decoding support.
 *   `JEBP_NO_VP8L` will disable VP8L (lossless) decoding support. Note that
 *                  either VP8 or VP8L decoding support is required and it is an
 *                  error to disable both.
 *   `JEBP_ONLY_VP8` and `JEBP_ONLY_VP8L` will disable all other features except
 *                   the specified feature.
 *   `JEBP_ALLOC` and `JEBP_FREE` can be defined to functions for a custom
 *                allocator. They either both have to be defined or neither
 *                defined.
 *
 * This single-header library requires C99 to be supported. Along with this it
 * requires the following headers from the system to successfully compile. Some
 * of these can be disabled with the above macros:
 *   `stddef.h` is used for the definition of the `size_t` type.
 *   `limits.h` is used for the `UINT_MAX` macro to check the size of `int`. If
 *              `int` is not 32-bits, `long` will be used for `jebp_int`
 *              instead.
 *   `string.h` is used for `memset` to clear out memory.
 *   `stdio.h` is used for I/O support and logging errors. If `JEBP_NO_STDIO` is
 *             defined and `JEBP_LOG_ERRORS` is not defined, this will not be
 *             included.
 *   `stdlib.h` is used for the default implementations of `JEBP_ALLOC`
 *              and `JEBP_FREE`, using `malloc` and `free` respectively. If
 *              those macros are already defined to something else, this will
 *              not be included.
 *   `emmintrin.h` and `arm_neon.h` is used for SIMD intrinsice. If
 *                 `JEBP_NO_SIMD` is defined these will not be included.
 *
 * The following predefined macros are also used for compiler-feature, SIMD and
 * endianness detection. These can be changed or modified before import to
 * change JebP's detection logic:
 *   `__STDC_VERSION__` is used to detect if the compiler supports C99 and also
 *                      checks for C11 support to use `_Noreturn`.
 *   `__has_attribute` and `__has_builtin` are used to detect the `noreturn` and
 *                     `always_inline` attributes, along with the
 *                     `__builtin_bswap16` and `__builtin_bswap32` builtins.
 *                     Note that `__has_attribute` does not fallback to compiler
 *                     version checks since most compilers already support
 *                     `__has_attribute`.
 *   `__GNUC__` and `__GNUC_MINOR__` are used to detect if the compiler is GCC
 *              (or GCC compatible) and what version of GCC it is. This, in
 *              turn, is used to polyfill `__has_builtin` on older compilers
 *              that may not support it.
 *   `__clang__` is used to detect the Clang compiler. This is only used to set
 *               the detected GCC version higher since Clang still marks itself
 *               as GCC 4.2 by default. No Clang version detection is done.
 *   `_MSC_VER` is used to detect the MSVC compiler. This is used to check
 *              support for `__declspec(noreturn)`, `__forceinline` and
 *              `_byteswap_ulong`. No MSVC version detection is done.
 *   `__LITTLE_ENDIAN__` is used to check if the architecture is little-endian.
 *                       Note that this is only checked either if the
 *                       architecture cannot be detected or, in special cases,
 *                       where there is not enough information from the
 *                       architecture or compiler to detect endianness. Also
 *                       note that big-endian and other more-obscure endian
 *                       types are not detected. Little-endian is the only
 *                       endianness detected and is used for optimization in a
 *                       few areas. If the architecture is not little-endian or
 *                       cannot be detected as such, a naive solution is used
 *                       instead.
 *   `__i386`, `__i386__` and `_M_IX86` are used to detect if this is being
 *           compiled for x86-32 (also known as x86, IA-32, or i386). If one of
 *           these are defined, it is also assumed that the architecture is
 *           little-endian. `_M_IX86` is usually present on MSVC, while
 *           the other two are usually present on most other compilers.
 *   `__SSE2__` and `_M_IX86_FP` are used to detect SSE2 support on x86-32.
 *              `_M_IX86`, which is usually present on MSVC, must equal 2 to
 *              indicate that the code is being compiled for a SSE2-compatible
 *              floating-point unit. `__SSE2__` is usually present on most other
 *              compilers.
 *   `__x86_64`, `__x86_64__` and `_M_X64` are used to detect if this is being
 *            compiled for x86-64 (also known as AMD64). If one of these are
 *            defined, it is also assumed that the architecture is little-endian
 *            and that SSE2 is supported (which is required for x86-64 support).
 *            `_M_X64` is usually present on MSVC, while the other two are
 *            usually present on most other compilers.
 *   `__arm`, `__arm__` and `_M_ARM` are used to detect if this is being
 *            compiled for AArch32 (also known as arm32 or armhf). If one of
 *            these are defined on Windows, it is also assumed that Neon is
 *            supported (which is required for Windows). `_M_ARM` is usually
 *            present on MSVC while the other two are usually present on most
 *            other compilers.
 *   `__ARM_NEON` is used to detect Neon support on AArch32. MSVC doesn't seem
 *                to support this and I can't find any info on detecting Neon
 *                support for MSVC. I have found mentions of Windows requiring
 *                Neon support but cannot find any concrete proof anywhere.
 *   `__aarch64`, `__aarch64__` and `_M_ARM64` are used to detect if this is
 *                being compiled for AArch64 (also known as arm64). If one of
 *                these are defined, it is also assumed that Neon is supported
 *                (which is required for AArch64 support). `_M_ARM64` is usually
 *                present on MSVC, while the other two are usually present on
 *                most other compilers.
 *   `__ARM_BIG_ENDIAN` is used to detect, on AArch/ARM architectures, if it is
 *                      in big-endian mode. However, as mentioned above, there
 *                      is no special code for big-endian and it's worth noting
 *                      that this is just used to force-disable little-endian.
 *                      If this is not present, it falls back to using
 *                      `__LITTLE_ENDIAN__`. It is also worth noting that MSVC
 *                      does not seem to provide a way to detect endianness. It
 *                      may be that Windows requires little-endian but I can't
 *                      find any concrete sources on this so currently
 *                      little-endian detection is not supported on MSVC.
 */

/**
 * HEADER
 */
#ifndef JEBP__HEADER
#define JEBP__HEADER
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#include <limits.h>
#include <stddef.h>

#if UINT_MAX >= 0xffffffff
#define JEBP__INT int
#else
#define JEBP__INT long
#endif
typedef signed char jebp_byte;
typedef unsigned char jebp_ubyte;
typedef short jebp_short;
typedef unsigned short jebp_ushort;
typedef JEBP__INT jebp_int;
typedef unsigned JEBP__INT jebp_uint;

typedef enum jebp_error_t {
    JEBP_OK,
    JEBP_ERROR_INVAL,
    JEBP_ERROR_INVDATA,
    JEBP_ERROR_INVDATA_HEADER,
    JEBP_ERROR_EOF,
    JEBP_ERROR_NOSUP,
    JEBP_ERROR_NOSUP_CODEC,
    JEBP_ERROR_NOSUP_PALETTE,
    JEBP_ERROR_NOMEM,
    JEBP_ERROR_IO,
    JEBP_ERROR_UNKNOWN,
    JEBP_NB_ERRORS
} jebp_error_t;

typedef struct jebp_color_t {
    jebp_ubyte r;
    jebp_ubyte g;
    jebp_ubyte b;
    jebp_ubyte a;
} jebp_color_t;

typedef struct jebp_image_t {
    jebp_int width;
    jebp_int height;
    jebp_color_t *pixels;
} jebp_image_t;

const char *jebp_error_string(jebp_error_t err);
void jebp_free_image(jebp_image_t *image);
jebp_error_t jebp_decode_size(jebp_image_t *image, size_t size,
                              const void *data);
jebp_error_t jebp_decode(jebp_image_t *image, size_t size, const void *data);

// I/O API
#ifndef JEBP_NO_STDIO
jebp_error_t jebp_read_size(jebp_image_t *image, const char *path);
jebp_error_t jebp_read(jebp_image_t *image, const char *path);
#endif // JEBP_NO_STDIO

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // JEBP__HEADER

/**
 * IMPLEMENTATION
 */
#ifdef JEBP_IMPLEMENTATION
#include <string.h>
#if !defined(JEBP_NO_STDIO)
#include <stdio.h>
#endif
#if !defined(JEBP_ALLOC) && !defined(JEBP_FREE)
#include <stdlib.h>
#define JEBP_ALLOC malloc
#define JEBP_FREE free
#elif !defined(JEBP_ALLOC) || !defined(JEBP_FREE)
#error "Both JEBP_ALLOC and JEBP_FREE have to be defined"
#endif

#if defined(JEBP_ONLY_VP8) || defined(JEBP_ONLY_VP8L)
#ifndef JEBP_ONLY_VP8
#define JEBP_NO_VP8L
#endif // JEBP_ONLY_VP8
#ifndef JEBP_ONLY_VP8L
#define JEBP_NO_VP8
#endif // JEBP_ONLY_VP8L
#endif
#if defined(JEBP_NO_VP8) && defined(JEBP_NO_VP8L)
#error "Either VP8 or VP8L has to be enabled"
#endif

/**
 * Predefined macro detection
 */
#ifdef __STDC_VERSION__
#if __STDC_VERSION__ < 199901
#error "Standard C99 support is required"
#endif
#else // __STDC_VERSION__
#if defined(__GNUC__)
#warning "C version cannot be checked, compilation may fail"
#elif defined(_MSC_VER)
#pragma message(                                                               \
    "MSVC by default is C89 'with extensions', use /std:c11 to ensure there are no errors")
#endif
#endif // __STDC_VERSION__
#if defined(__clang__)
// The default GNUC version provided by Clang is just short of what we need
#define JEBP__GNU_VERSION 403
#elif defined(__GNUC__)
#define JEBP__GNU_VERSION ((__GNUC__ * 100) + __GNUC_MINOR__)
#else
#define JEBP__GNU_VERSION 0
#endif // __GNUC__

#ifdef __has_attribute
#define JEBP__HAS_ATTRIBUTE __has_attribute
#else // __has_attribute
// We don't add GCC version checks since, unlike __has_builtin, __has_attribute
// has been out for so long that its more likely that the compiler supports it.
#define JEBP__HAS_ATTRIBUTE(attr) 0
#endif // __has_attribute
#if JEBP__HAS_ATTRIBUTE(always_inline)
#define JEBP__ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define JEBP__ALWAYS_INLINE __forceinline
#else
#define JEBP__ALWAYS_INLINE
#endif
#define JEBP__INLINE static inline JEBP__ALWAYS_INLINE
#if JEBP__HAS_ATTRIBUTE(aligned)
#define JEBP__ALIGN_TYPE(type, align) type __attribute__((aligned(align)))
#elif defined(_MSC_VER)
#define JEBP__ALIGN_TYPE(type, aligned) __declspec(align(aligned)) type
#else
#define JEBP__ALIGN_TYPE(type, align) type
#endif

#ifdef __has_builtin
#define JEBP__HAS_BUILTIN __has_builtin
#else // __has_builtin
#define JEBP__HAS_BUILTIN(builtin)                                             \
    JEBP__VERSION##builtin != 0 && JEBP__GNU_VERSION >= JEBP__VERSION##builtin
// I believe this was added earlier but GCC 4.3 is the first time it was
// mentioned in the changelog and manual.
#define JEBP__VERSION__builtin_bswap16 403
#define JEBP__VERSION__builtin_bswap32 403
#endif // __has_builtin
#if JEBP__HAS_BUILTIN(__builtin_bswap16)
#define JEBP__SWAP16(value) __builtin_bswap16(value)
#elif defined(_MSC_VER)
#define JEBP__SWAP16(value) _byteswap_ushort(value)
#endif
#if JEBP__HAS_BUILTIN(__builtin_bswap32)
#define JEBP__SWAP32(value) __builtin_bswap32(value)
#elif defined(_MSC_VER)
#define JEBP__SWAP32(value) _byteswap_ulong(value)
#endif

// We don't do any SIMD runtime detection since that causes alot of
// heavily-documented issues that I won't go into here. Instead, if the compiler
// supports it (and requests it) we will use it. It helps that both x86-64 and
// AArch64 always support the SIMD from their 32-bit counterparts.
#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
#define JEBP__ARCH_X86
#if defined(__SSE2__) || _M_IX86_FP == 2
#define JEBP__SIMD_SSE2
#endif
#elif defined(__x86_64) || defined(__x86_64__) || defined(_M_X64)
#define JEBP__ARCH_X86
#define JEBP__SIMD_SSE2
#elif defined(__arm) || defined(__arm__) || defined(_M_ARM)
#define JEBP__ARCH_ARM
#if defined(__ARM_NEON) || defined(_MSC_VER)
// According to the following article, MSVC requires Neon support
// https://docs.microsoft.com/en-us/cpp/build/overview-of-arm-abi-conventions
#define JEBP__SIMD_NEON
#endif
#elif defined(__aarch64) || defined(__aarch64__) || defined(_M_ARM64)
#define JEBP__ARCH_ARM
#define JEBP__SIMD_NEON
#define JEBP__SIMD_NEON64
#endif

#if defined(JEBP__ARCH_X86)
// x86 is always little-endian
#define JEBP__LITTLE_ENDIAN
#elif defined(JEBP__ARCH_ARM) && defined(__ARM_BIG_ENDIAN)
// The ACLE big-endian define overrules everything else, including the defualt
// endianness detection
#elif defined(JEBP__ARCH_ARM) && (defined(__ARM_ACLE) || defined(_MSC_VER))
// If ACLE is supported and big-endian is not defined, it must be little-endian
// According to the article linked above, MSVC only supports little-endian
#define JEBP__LITTLE_ENDIAN
#elif defined(__LITTLE_ENDIAN__) || __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define JEBP__LITTLE_ENDIAN
#endif

#ifdef JEBP_NO_SIMD
#undef JEBP__SIMD_SSE2
#undef JEBP__SIMD_NEON
#endif // JEBP_NO_SIMD
#ifdef JEBP__SIMD_SSE2
#include <emmintrin.h>
#define JEBP__SIMD_ALIGN 16
#endif // JEBP__SIMD_SSE2
#ifdef JEBP__SIMD_NEON
#include <arm_neon.h>
#define JEBP__SIMD_ALIGN 16
#endif // JEBP__SIMD_NEON
#ifndef JEBP__SIMD_ALIGN
#define JEBP__SIMD_ALIGN 1
#endif // JEBP__SIMD_ALIGN

/**
 * Common utilities
 */
// TODO: Maybe we should  have a logging flag and add custom logs with more
//       information to each error (and maybe other stuff like allocations)
#define JEBP__MIN(a, b) ((a) < (b) ? (a) : (b))
#define JEBP__MAX(a, b) ((a) > (b) ? (a) : (b))
#define JEBP__ABS(a) ((a) < 0 ? -(a) : (a))
#define JEBP__CLAMP(x, min, max) JEBP__MIN(JEBP__MAX(x, min), max)
#define JEBP__CLAMP_UBYTE(x) JEBP__CLAMP(x, 0, 255)
// F=floor, C=ceil, R=round
#define JEBP__CSHIFT(a, b) (((a) + (1 << (b)) - 1) >> (b))
#define JEBP__RSHIFT(a, b) (((a) + (1 << ((b)-1))) >> (b))
#define JEBP__FAVG(a, b) (((a) + (b)) / 2)
#define JEBP__RAVG(a, b) JEBP__RSHIFT((a) + (b), 1)
#define JEBP__RAVG3(a, b, c) JEBP__RSHIFT((a) + (b) + (b) + (c), 2)
#define JEBP__CALIGN(a, b) (((a) + (b)-1) & ~((b)-1))
#define JEBP__SET_MASK(x, m, v) ((x) = ((x) & ~(m)) | ((v) & (m)))
#define JEBP__SET_BIT(x, b, v) JEBP__SET_MASK(x, b, (v) ? (b) : 0)
#define JEBP__CLEAR(ptr, size) memset(ptr, 0, size)

// A simple utility that updates an error pointer if it currently does not have
// an error
JEBP__INLINE jebp_error_t jebp__error(jebp_error_t *err, jebp_error_t error) {
    if (*err == JEBP_OK) {
        *err = error;
    }
    return *err;
}

static jebp_error_t jebp__alloc_image(jebp_image_t *image) {
    image->pixels =
        JEBP_ALLOC(image->width * image->height * sizeof(jebp_color_t));
    if (image->pixels == NULL) {
        return JEBP_ERROR_NOMEM;
    }
    return JEBP_OK;
}

/**
 * Reader abstraction
 */
#define JEBP__BUFFER_SIZE 4096

typedef struct jebp__reader_t {
    size_t nb_bytes;
    const jebp_ubyte *bytes;
#ifndef JEBP_NO_STDIO
    FILE *file;
    void *buffer;
#endif // JEBP_NO_STDIO
} jebp__reader_t;

static void jebp__init_memory(jebp__reader_t *reader, size_t size,
                              const void *data) {
    reader->nb_bytes = size;
    reader->bytes = data;
#ifndef JEBP_NO_STDIO
    reader->file = NULL;
#endif // JEBP_NO_STDIO
}

#ifndef JEBP_NO_STDIO
static jebp_error_t jebp__open_file(jebp__reader_t *reader, const char *path) {
    reader->nb_bytes = 0;
    reader->file = fopen(path, "rb");
    if (reader->file == NULL) {
        return JEBP_ERROR_IO;
    }
    reader->buffer = JEBP_ALLOC(JEBP__BUFFER_SIZE);
    if (reader->buffer == NULL) {
        fclose(reader->file);
        return JEBP_ERROR_NOMEM;
    }
    return JEBP_OK;
}

static void jebp__close_file(jebp__reader_t *reader) {
    JEBP_FREE(reader->buffer);
    fclose(reader->file);
}
#endif // JEBP_NO_STDIO

static jebp_error_t jebp__buffer_bytes(jebp__reader_t *reader) {
    if (reader->nb_bytes > 0) {
        return JEBP_OK;
    }
#ifndef JEBP_NO_STDIO
    if (reader->file != NULL) {
        reader->nb_bytes =
            fread(reader->buffer, 1, JEBP__BUFFER_SIZE, reader->file);
        reader->bytes = reader->buffer;
        if (ferror(reader->file)) {
            return JEBP_ERROR_IO;
        }
    }
#endif // JEBP_NO_STDIO
    if (reader->nb_bytes == 0) {
        return JEBP_ERROR_EOF;
    }
    return JEBP_OK;
}

// TODO: Most reads are only a few bytes so maybe I should optimize for that
static jebp_error_t jebp__read_bytes(jebp__reader_t *reader, size_t size,
                                     void *data) {
    jebp_error_t err;
    jebp_ubyte *bytes = data;
    while (size > 0) {
        if ((err = jebp__buffer_bytes(reader)) != JEBP_OK) {
            return err;
        }
        size_t nb_bytes = JEBP__MIN(size, reader->nb_bytes);
        if (bytes != NULL) {
            memcpy(bytes, reader->bytes, nb_bytes);
            bytes += nb_bytes;
        }
        size -= nb_bytes;
        reader->nb_bytes -= nb_bytes;
        reader->bytes += nb_bytes;
    }
    return JEBP_OK;
}

// Reader mapping is only used by VP8
#ifndef JEBP_NO_VP8
static jebp_error_t jebp__map_reader(jebp__reader_t *reader,
                                     jebp__reader_t *map, size_t size) {
    jebp_error_t err;
#ifndef JEBP_NO_STDIO
    if (reader->file != NULL) {
        void *data = JEBP_ALLOC(size);
        if (data == NULL) {
            return JEBP_ERROR_NOMEM;
        }
        if ((err = jebp__read_bytes(reader, size, data)) != JEBP_OK) {
            JEBP_FREE(data);
            return err;
        }
        jebp__init_memory(map, size, data);
        map->buffer = data;
        return JEBP_OK;
    }
    map->buffer = NULL;
#endif // JEBP_NO_STDIO
    const void *data = reader->bytes;
    if ((err = jebp__read_bytes(reader, size, NULL)) != JEBP_OK) {
        return err;
    }
    jebp__init_memory(map, size, data);
    return JEBP_OK;
}

static void jebp__unmap_reader(jebp__reader_t *map) {
#ifndef JEBP_NO_STDIO
    JEBP_FREE(map->buffer);
#else  // JEBP_NO_STDIO
    (void)map;
#endif // JEBP_NO_STDIO
}
#endif // JEBP_NO_VP8

static jebp_ubyte jebp__read_uint8(jebp__reader_t *reader, jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
    if ((*err = jebp__buffer_bytes(reader)) != JEBP_OK) {
        return 0;
    }
    reader->nb_bytes -= 1;
    return *(reader->bytes++);
}

// 16-bit and 24-bit uint reading is only used by VP8
#ifndef JEBP_NO_VP8
static jebp_ushort jebp__read_uint16(jebp__reader_t *reader,
                                     jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
#ifdef JEBP__LITTLE_ENDIAN
    jebp_ushort value = 0;
    *err = jebp__read_bytes(reader, 2, &value);
    return value;
#else  // JEBP__LITTLE_ENDIAN
    jebp_ubyte bytes[2];
    *err = jebp__read_bytes(reader, 2, bytes);
    return bytes[0] | (bytes[1] << 8);
#endif // JEBP__LITTLE_ENDIAN
}

static jebp_int jebp__read_uint24(jebp__reader_t *reader, jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
#ifdef JEBP__LITTLE_ENDIAN
    jebp_int value = 0;
    *err = jebp__read_bytes(reader, 3, &value);
    return value;
#else  // JEBP__LITTLE_ENDIAN
    jebp_ubyte bytes[3];
    *err = jebp__read_bytes(reader, 3, bytes);
    return (jebp_int)bytes[0] | ((jebp_int)bytes[1] << 8) |
           ((jebp_int)bytes[2] << 16);
#endif // JEBP__LITTLE_ENDIAN
}
#endif // JEBP_NO_VP8

static jebp_uint jebp__read_uint32(jebp__reader_t *reader, jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
#ifdef JEBP__LITTLE_ENDIAN
    jebp_uint value = 0;
    *err = jebp__read_bytes(reader, 4, &value);
    return value;
#else  // JEBP__LITTLE_ENDIAN
    jebp_ubyte bytes[4];
    *err = jebp__read_bytes(reader, 4, bytes);
    return (jebp_uint)bytes[0] | ((jebp_uint)bytes[1] << 8) |
           ((jebp_uint)bytes[2] << 16) | ((jebp_uint)bytes[3] << 24);
#endif // JEBP__LITTLE_ENDIAN
}

/**
 * RIFF container
 */
#define JEBP__RIFF_TAG 0x46464952
#define JEBP__WEBP_TAG 0x50424557

typedef struct jebp__chunk_t {
    jebp_uint tag;
    jebp_uint size;
} jebp__chunk_t;

typedef struct jebp__riff_reader_t {
    jebp__reader_t *reader;
    jebp__chunk_t header;
} jebp__riff_reader_t;

static jebp_error_t jebp__read_chunk(jebp__riff_reader_t *riff,
                                     jebp__chunk_t *chunk) {
    jebp_error_t err = JEBP_OK;
    chunk->tag = jebp__read_uint32(riff->reader, &err);
    chunk->size = jebp__read_uint32(riff->reader, &err);
    chunk->size += chunk->size % 2; // round up to even
    return err;
}

static jebp_error_t jebp__read_riff_header(jebp__riff_reader_t *riff,
                                           jebp__reader_t *reader) {
    jebp_error_t err;
    riff->reader = reader;
    if ((err = jebp__read_chunk(riff, &riff->header)) != JEBP_OK) {
        return err;
    }
    if (riff->header.tag != JEBP__RIFF_TAG) {
        return JEBP_ERROR_INVDATA_HEADER;
    }
    if (jebp__read_uint32(reader, &err) != JEBP__WEBP_TAG) {
        return jebp__error(&err, JEBP_ERROR_INVDATA_HEADER);
    }
    return err;
}

static jebp_error_t jebp__read_riff_chunk(jebp__riff_reader_t *riff,
                                          jebp__chunk_t *chunk) {
    jebp_error_t err;
    if ((err = jebp__read_chunk(riff, chunk)) != JEBP_OK) {
        return err;
    }
    if (chunk->size > riff->header.size) {
        return JEBP_ERROR_INVDATA;
    }
    riff->header.size -= chunk->size;
    return JEBP_OK;
}

/**
 * YUV image
 */
#ifndef JEBP_NO_VP8

//  R = 255 * ((Y-16)/219 + (Cr-128)/224 * 1.402)
#define JEBP__CONVERT_R(y, v)                                                  \
    JEBP__CLAMP_UBYTE(((y)*298 + (v)*409 - 57068) >> 8)
// Eg = (Ey - Er*0.299 - Eb*0.114)/0.587
//    = Ey/0.587 - (Ey+Ecr*1.402)*(0.299/0.587) - (Ey+Ecb*1.772)*(0.114/0.587)
//    = Ey - Ecr*(1.402*0.299/0.587) - Ecb*(1.772*0.114/0.587)
//  G = 255 * ((Y-16)/219 - (Cr-128)/224 * (1.402*0.299/0.587) - (Cb-128)/224 *
//      (1.772*0.114/0.587))
#define JEBP__CONVERT_G(y, u, v)                                               \
    JEBP__CLAMP_UBYTE(((y)*298 - (u)*208 - (v)*100 + 34707) >> 8)
//  B = 255 * ((Y-16)/219 + (Cb-128)/224 * 1.772)
#define JEBP__CONVERT_B(y, u)                                                  \
    JEBP__CLAMP_UBYTE(((y)*298 + (u)*516 - 70870) >> 8)

typedef struct jebp__yuv_image_t {
    jebp_int width;
    jebp_int height;
    jebp_int stride;
    jebp_int uv_width;
    jebp_int uv_height;
    jebp_int uv_stride;
    jebp_ubyte *buffer;
    jebp_ubyte *y;
    jebp_ubyte *u;
    jebp_ubyte *v;
} jebp__yuv_image_t;

static void jebp__fill_yuv_edge(jebp_ubyte *pred, jebp_int stride,
                                jebp_int height) {
    jebp_ubyte *top = &pred[-stride];
    memset(top, 127, stride - JEBP__SIMD_ALIGN);
    top[-1] = 127;
    for (jebp_int y = 0; y < height; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        row[-1] = 129;
    }
}

static jebp_error_t jebp__alloc_yuv_image(jebp__yuv_image_t *image) {
    // The only time this function is used, width/height are even
    image->uv_width = image->width / 2;
    image->uv_height = image->height / 2;
    // We have extra columns to the left for filling default prediction values,
    // aligned to the SIMD alignment
    image->stride = image->width + JEBP__SIMD_ALIGN;
    image->uv_stride = image->uv_width + JEBP__SIMD_ALIGN;
    // We also have one row above for the same reason
    size_t y_size = image->stride * (image->height + 1);
    size_t uv_size = image->uv_stride * (image->uv_height + 1);
    image->buffer = JEBP_ALLOC(y_size + uv_size * 2 + JEBP__SIMD_ALIGN);
    if (image->buffer == NULL) {
        return JEBP_ERROR_NOMEM;
    }

    // Setup the actual pointers
    // TODO: maybe move this to a function and use native aligned alloc if
    //       available
    image->y = (void *)JEBP__CALIGN((size_t)image->buffer, JEBP__SIMD_ALIGN);
    image->u = image->y + y_size;
    image->v = image->u + uv_size;
    image->y += image->stride + JEBP__SIMD_ALIGN;
    size_t uv_offset = image->uv_stride + JEBP__SIMD_ALIGN;
    image->u += uv_offset;
    image->v += uv_offset;
    // Setup default values for edge prediction
    jebp__fill_yuv_edge(image->y, image->stride, image->height);
    jebp__fill_yuv_edge(image->u, image->uv_stride, image->uv_height);
    jebp__fill_yuv_edge(image->v, image->uv_stride, image->uv_height);
    return JEBP_OK;
}

static void jebp__free_yuv_image(jebp__yuv_image_t *image) {
    JEBP_FREE(image->buffer);
}

JEBP__INLINE void jebp__upscale_uv_row(jebp_ubyte *out, jebp_ubyte *in,
                                       jebp_int width) {
    jebp_int x = 0;
    for (; x < width - 1; x += 1) {
        out[x * 2] = in[x];
        out[x * 2 + 1] = JEBP__RAVG(in[x], in[x + 1]);
    }
    out[x * 2] = in[x];
    out[x * 2 + 1] = in[x];
}

static jebp_error_t jebp__convert_yuv_image(jebp_image_t *out,
                                            jebp__yuv_image_t *in) {
    // Buffers to upscale UV rows into
    jebp_ubyte *uv_buffer = JEBP_ALLOC(in->width * 4);
    if (uv_buffer == NULL) {
        return JEBP_ERROR_NOMEM;
    }
    jebp_ubyte *u_prev = uv_buffer;
    jebp_ubyte *v_prev = u_prev + in->width;
    jebp_ubyte *u_next = v_prev + in->width;
    jebp_ubyte *v_next = u_next + in->width;
    jebp__upscale_uv_row(u_prev, in->u, in->uv_width);
    jebp__upscale_uv_row(v_prev, in->v, in->uv_width);

    for (jebp_int y = 0; y < out->height; y += 2) {
        // Rec. 601 doesn't specify the chroma location for 420, for now I'm
        // assuming it is top-left
        // Even rows
        jebp_color_t *row = &out->pixels[y * out->width];
        jebp_ubyte *y_row = &in->y[y * in->stride];
        for (jebp_int x = 0; x < out->width; x += 1) {
            row[x].r = JEBP__CONVERT_R(y_row[x], v_prev[x]);
            row[x].g = JEBP__CONVERT_G(y_row[x], u_prev[x], v_prev[x]);
            row[x].b = JEBP__CONVERT_B(y_row[x], u_prev[x]);
            row[x].a = 255;
        }

        if (y + 1 == out->height) {
            // If the image height is odd, end here
            break;
        } else if (y + 2 == in->height) {
            // If this is the final row, duplicate the UV rows
            u_next = u_prev;
            v_next = v_prev;
        } else {
            // Upscale next row
            jebp_int uv_next = (y / 2 + 1) * in->uv_stride;
            jebp__upscale_uv_row(u_next, &in->u[uv_next], in->uv_width);
            jebp__upscale_uv_row(v_next, &in->v[uv_next], in->uv_width);
        }

        // Odd rows
        row = &out->pixels[(y + 1) * out->width];
        y_row = &in->y[(y + 1) * in->stride];
        for (jebp_int x = 0; x < out->width; x += 1) {
            jebp_ubyte u_avg = JEBP__RAVG(u_prev[x], u_next[x]);
            jebp_ubyte v_avg = JEBP__RAVG(v_prev[x], v_next[x]);
            row[x].r = JEBP__CONVERT_R(y_row[x], v_avg);
            row[x].g = JEBP__CONVERT_G(y_row[x], u_avg, v_avg);
            row[x].b = JEBP__CONVERT_B(y_row[x], u_avg);
            row[x].a = 255;
        }
        // Swap buffers
        jebp_ubyte *tmp;
        tmp = u_prev;
        u_prev = u_next;
        u_next = tmp;
        tmp = v_prev;
        v_prev = v_next;
        v_next = tmp;
    }
    JEBP_FREE(uv_buffer);
    return JEBP_OK;
}

/**
 * Boolean entropy coding
 */
#define JEBP__NB_PROBS(nb) ((nb)-1)
#define JEBP__NB_TREE(nb) (2 * JEBP__NB_PROBS(nb))

typedef struct jebp__bec_reader_t {
    jebp__reader_t *reader;
    size_t nb_bytes;
    jebp_int nb_bits;
    jebp_int value;
    jebp_int range;
} jebp__bec_reader_t;

static jebp_error_t jebp__init_bec_reader(jebp__bec_reader_t *bec,
                                          jebp__reader_t *reader, size_t size) {
    jebp_error_t err;
    if (size < 2) {
        return JEBP_ERROR_INVDATA;
    }
    bec->reader = reader;
    bec->nb_bytes = size - 2;
    bec->nb_bits = 8;
#if defined(JEBP__LITTLE_ENDIAN) && defined(JEBP__SWAP16)
    jebp_ushort value = 0;
    err = jebp__read_bytes(reader, 2, &value);
    bec->value = JEBP__SWAP16(value);
#else
    jebp_ubyte bytes[2];
    err = jebp__read_bytes(reader, 2, bytes);
    bec->value = (bytes[0] << 8) | bytes[1];
#endif
    if (err != JEBP_OK) {
        return err;
    }
    bec->range = 255;
    return JEBP_OK;
}

// TODO: this code can definitely be improved, especially since its used alot
//       and probably needs to be very fast. Notable changes:
//        - instead of a while loop do all the shifts at once
//        - fetch 16 or 24-bits at a time from the reader (instead of
//          byte-by-byte)
//        - check bit size and fetch more if needed at the start of a new call
//          (instead of at the end of the previous call)
//        - optimize the prob = 128 variant, maybe optimize int reading with
//          multiple prob=128 bits
//        - it might be possible to simplify the split calculation by always
//          storing the range with -1
//        - instead of shifting the value, use nb_bits as a shift offset of the
//          value
static jebp_int jebp__read_bool(jebp__bec_reader_t *bec, jebp_ubyte prob,
                                jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
    jebp_int split = 1 + (((bec->range - 1) * prob) >> 8);
    jebp_int split_high = split << 8;
    jebp_int boolval = bec->value >= split_high;
    if (boolval) {
        bec->value -= split_high;
        bec->range -= split;
    } else {
        bec->range = split;
    }

    while (bec->range < 128) {
        bec->value <<= 1;
        bec->range <<= 1;
        bec->nb_bits -= 1;
        if (bec->nb_bits == 0) {
            if (bec->nb_bytes == 0) {
                jebp__error(err, JEBP_ERROR_INVDATA);
                return 0;
            }
            bec->value |= jebp__read_uint8(bec->reader, err);
            bec->nb_bytes -= 1;
            bec->nb_bits = 8;
        }
    }
    return boolval;
}

JEBP__INLINE jebp_int jebp__read_flag(jebp__bec_reader_t *bec,
                                      jebp_error_t *err) {
    return jebp__read_bool(bec, 128, err);
}

static jebp_uint jebp__read_bec_uint(jebp__bec_reader_t *bec, jebp_int size,
                                     jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
    jebp_uint value = 0;
    for (jebp_int i = 0; i < size; i += 1) {
        value = (value << 1) | jebp__read_flag(bec, err);
    }
    return value;
}

static jebp_int jebp__read_bec_int(jebp__bec_reader_t *bec, jebp_int size,
                                   jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
    jebp_int value = jebp__read_bec_uint(bec, size, err);
    return jebp__read_flag(bec, err) ? -value : value;
}

static jebp_int jebp__read_tree(jebp__bec_reader_t *bec, const jebp_byte *tree,
                                const jebp_ubyte *probs, jebp_error_t *err) {
    jebp_int index = 0;
    do {
        const jebp_byte *node = &tree[index];
        index = node[jebp__read_bool(bec, probs[index / 2], err)];
    } while (index > 0);
    return -index;
}

/**
 * Compressed B.E.C. header
 */
#define JEBP__NB_SEGMENTS 4
#define JEBP__NB_QUANT_INDEXES 128
#define JEBP__NB_COEFFS 16
#define JEBP__NB_COEFF_BANDS 8
#define JEBP__NB_TOKEN_COMPLEXITIES 3
#define JEBP__CLAMP_QUANT(q) JEBP__CLAMP(q, 0, JEBP__NB_QUANT_INDEXES - 1)

typedef enum jebp__segment_type_t {
    JEBP__SEGMENT_NONE = -1,
    JEBP__SEGMENT_ZERO,
    JEBP__SEGMENT_ID
} jebp__segment_type_t;

typedef struct jebp__quants_t {
    jebp_short y_dc;
    jebp_short y_ac;
    jebp_short y2_dc;
    jebp_short y2_ac;
    jebp_short uv_dc;
    jebp_short uv_ac;
} jebp__quants_t;

typedef struct jebp__segment_t {
    jebp__quants_t quants;
    jebp_short filter_strength;
} jebp__segment_t;

typedef enum jebp__block_type_t {
    JEBP__BLOCK_Y1, // Y beginning at 1
    JEBP__BLOCK_Y2, // WHT block of DC values
    JEBP__BLOCK_UV,
    JEBP__BLOCK_Y0, // Y beginning at 0
    JEBP__NB_BLOCK_TYPES
} jebp__block_type_t;

typedef enum jebp__token_t {
    JEBP__TOKEN_COEFF0,
    JEBP__TOKEN_COEFF1,
    JEBP__TOKEN_COEFF2,
    JEBP__TOKEN_COEFF3,
    JEBP__TOKEN_COEFF4,
    JEBP__TOKEN_EXTRA1,
    JEBP__TOKEN_EXTRA2,
    JEBP__TOKEN_EXTRA3,
    JEBP__TOKEN_EXTRA4,
    JEBP__TOKEN_EXTRA5,
    JEBP__TOKEN_EXTRA6,
    JEBP__TOKEN_EOB,
    JEBP__NB_TOKENS,
    JEBP__NB_EXTRA_TOKENS = JEBP__TOKEN_EOB - JEBP__TOKEN_EXTRA1
} jebp__token_t;

typedef struct jebp__vp8_header_t {
    jebp_int bec_size;
    jebp__segment_type_t segment_type;
    jebp_int abs_segments;
    jebp__segment_t segments[JEBP__NB_SEGMENTS];
    jebp_ubyte segment_probs[JEBP__NB_PROBS(JEBP__NB_SEGMENTS)];
    jebp_int simple_filter;
    jebp_short filter_strength;
    jebp_short filter_sharpness;
    jebp_ubyte token_probs[JEBP__NB_BLOCK_TYPES][JEBP__NB_COEFF_BANDS]
                          [JEBP__NB_TOKEN_COMPLEXITIES]
                          [JEBP__NB_PROBS(JEBP__NB_TOKENS)];
} jebp__vp8_header_t;

static const jebp_short jebp__dc_quant_table[JEBP__NB_QUANT_INDEXES];
static const jebp_short jebp__ac_quant_table[JEBP__NB_QUANT_INDEXES];
static const jebp_ubyte
    jebp__default_token_probs[JEBP__NB_BLOCK_TYPES][JEBP__NB_COEFF_BANDS]
                             [JEBP__NB_TOKEN_COMPLEXITIES]
                             [JEBP__NB_PROBS(JEBP__NB_TOKENS)];
static const jebp_ubyte
    jebp__update_token_probs[JEBP__NB_BLOCK_TYPES][JEBP__NB_COEFF_BANDS]
                            [JEBP__NB_TOKEN_COMPLEXITIES]
                            [JEBP__NB_PROBS(JEBP__NB_TOKENS)];

static void jebp__init_vp8_header(jebp__vp8_header_t *hdr) {
    JEBP__CLEAR(hdr, sizeof(jebp__vp8_header_t));
    hdr->segment_type = JEBP__SEGMENT_NONE;
    hdr->abs_segments = 1;
    memset(hdr->segment_probs, 255, sizeof(hdr->segment_probs));
    memcpy(hdr->token_probs, jebp__default_token_probs,
           sizeof(hdr->token_probs));
}

static jebp_error_t jebp__read_segment_header(jebp__vp8_header_t *hdr,
                                              jebp__bec_reader_t *bec) {
    jebp_error_t err = JEBP_OK;
    if (!jebp__read_flag(bec, &err)) {
        // no segments
        return err;
    }
    hdr->segment_type = jebp__read_flag(bec, &err);
    if (jebp__read_flag(bec, &err)) {
        // update segment data
        hdr->abs_segments = jebp__read_flag(bec, &err);
        for (jebp_int i = 0; i < JEBP__NB_SEGMENTS; i += 1) {
            if (jebp__read_flag(bec, &err)) {
                hdr->segments[i].quants.y_ac = jebp__read_bec_int(bec, 7, &err);
            }
        }
        for (jebp_int i = 0; i < JEBP__NB_SEGMENTS; i += 1) {
            if (jebp__read_flag(bec, &err)) {
                hdr->segments[i].filter_strength =
                    jebp__read_bec_int(bec, 6, &err);
            }
        }
    }
    if (hdr->segment_type == JEBP__SEGMENT_ID) {
        for (jebp_int i = 0; i < JEBP__NB_PROBS(JEBP__NB_SEGMENTS); i += 1) {
            if (jebp__read_flag(bec, &err)) {
                hdr->segment_probs[i] = jebp__read_bec_uint(bec, 8, &err);
            }
        }
    }
    return err;
}

static jebp_error_t jebp__read_filter_header(jebp__vp8_header_t *hdr,
                                             jebp__bec_reader_t *bec) {
    jebp_error_t err = JEBP_OK;
    hdr->simple_filter = jebp__read_flag(bec, &err);
    hdr->filter_strength = jebp__read_bec_uint(bec, 6, &err);
    hdr->filter_sharpness = jebp__read_bec_uint(bec, 3, &err);
    if (jebp__read_flag(bec, &err)) {
        // TODO: support filter adjustments
        return jebp__error(&err, JEBP_ERROR_NOSUP);
    }
    return err;
}

static void jebp__update_quants(jebp__quants_t *quants,
                                jebp__quants_t *deltas) {
    quants->y_dc =
        jebp__dc_quant_table[JEBP__CLAMP_QUANT(deltas->y_ac + deltas->y_dc)];
    quants->y_ac = jebp__ac_quant_table[JEBP__CLAMP_QUANT(deltas->y_ac)];
    quants->y2_dc =
        jebp__dc_quant_table[JEBP__CLAMP_QUANT(deltas->y_ac + deltas->y2_dc)];
    quants->y2_dc *= 2;
    quants->y2_ac =
        jebp__ac_quant_table[JEBP__CLAMP_QUANT(deltas->y_ac + deltas->y2_ac)];
    quants->y2_ac = JEBP__MAX(quants->y2_ac * 155 / 100, 8);
    quants->uv_dc =
        jebp__dc_quant_table[JEBP__CLAMP_QUANT(deltas->y_ac + deltas->uv_dc)];
    quants->uv_dc = JEBP__MIN(quants->uv_dc, 132);
    quants->uv_ac =
        jebp__ac_quant_table[JEBP__CLAMP_QUANT(deltas->y_ac + deltas->uv_ac)];
}

static jebp_error_t jebp__read_quant_header(jebp__vp8_header_t *hdr,
                                            jebp__bec_reader_t *bec) {
    jebp_error_t err = JEBP_OK;
    jebp__quants_t deltas;
    jebp_int y_ac = jebp__read_bec_uint(bec, 7, &err);
    deltas.y_dc =
        jebp__read_flag(bec, &err) ? jebp__read_bec_int(bec, 4, &err) : 0;
    deltas.y2_dc =
        jebp__read_flag(bec, &err) ? jebp__read_bec_int(bec, 4, &err) : 0;
    deltas.y2_ac =
        jebp__read_flag(bec, &err) ? jebp__read_bec_int(bec, 4, &err) : 0;
    deltas.uv_dc =
        jebp__read_flag(bec, &err) ? jebp__read_bec_int(bec, 4, &err) : 0;
    deltas.uv_ac =
        jebp__read_flag(bec, &err) ? jebp__read_bec_int(bec, 4, &err) : 0;

    if (hdr->segment_type == JEBP__SEGMENT_NONE) {
        deltas.y_ac = y_ac;
        jebp__update_quants(&hdr->segments->quants, &deltas);
        return err;
    }
    if (hdr->abs_segments) {
        y_ac = 0;
    }
    for (jebp_int i = 0; i < JEBP__NB_SEGMENTS; i += 1) {
        jebp__quants_t *quants = &hdr->segments[i].quants;
        deltas.y_ac = y_ac + quants->y_ac;
        jebp__update_quants(quants, &deltas);
    }
    return err;
}

static jebp_error_t jebp__read_token_header(jebp__vp8_header_t *hdr,
                                            jebp__bec_reader_t *bec) {
    jebp_error_t err = JEBP_OK;
    jebp_ubyte *probs = hdr->token_probs[0][0][0];
    const jebp_ubyte *update_probs = jebp__update_token_probs[0][0][0];
    for (size_t i = 0; i < sizeof(jebp__update_token_probs); i += 1) {
        if (jebp__read_bool(bec, update_probs[i], &err)) {
            probs[i] = jebp__read_bec_uint(bec, 8, &err);
        }
    }
    if (jebp__read_flag(bec, &err)) {
        // TODO: support coefficient skipping
        return jebp__error(&err, JEBP_ERROR_NOSUP);
    }
    return err;
}

static jebp_error_t jebp__read_bec_header(jebp__vp8_header_t *hdr,
                                          jebp__bec_reader_t *bec) {
    jebp_error_t err = JEBP_OK;
    if (jebp__read_flag(bec, &err)) {
        // pixel format must be YCbCr
        return jebp__error(&err, JEBP_ERROR_NOSUP);
    }
    jebp__read_flag(bec, &err); // we always clamp pixels
    if (err != JEBP_OK) {
        return err;
    }
    if ((err = jebp__read_segment_header(hdr, bec)) != JEBP_OK) {
        return err;
    }
    if ((err = jebp__read_filter_header(hdr, bec)) != JEBP_OK) {
        return err;
    }
    if (jebp__read_bec_uint(bec, 2, &err) > 0 || err != JEBP_OK) {
        // TODO: support data partitions
        return jebp__error(&err, JEBP_ERROR_NOSUP);
    }
    if ((err = jebp__read_quant_header(hdr, bec)) != JEBP_OK) {
        return err;
    }
    jebp__read_flag(bec, &err); // there is only one frame so probabilities are
                                // never used for later frames
    if (err != JEBP_OK) {
        return err;
    }
    if ((err = jebp__read_token_header(hdr, bec)) != JEBP_OK) {
        return err;
    }
    return JEBP_OK;
}

/**
 * Macroblock header
 */
#define JEBP__BLOCK_BITS 2
#define JEBP__BLOCK_SIZE (1 << JEBP__BLOCK_BITS)                    // 4
#define JEBP__NB_BLOCK_COEFFS (JEBP__BLOCK_SIZE * JEBP__BLOCK_SIZE) // 16
#define JEBP__Y_BITS 2
#define JEBP__Y_SIZE (1 << JEBP__Y_BITS)                     // 4
#define JEBP__NB_Y_BLOCKS (JEBP__Y_SIZE * JEBP__Y_SIZE)      // 16
#define JEBP__Y_PIXEL_BITS (JEBP__Y_BITS + JEBP__BLOCK_BITS) // 4
#define JEBP__Y_PIXEL_SIZE (1 << JEBP__Y_PIXEL_BITS)         // 16
#define JEBP__UV_BITS 1
#define JEBP__UV_SIZE (1 << JEBP__UV_BITS)                     // 2
#define JEBP__NB_UV_BLOCKS (JEBP__UV_SIZE * JEBP__UV_SIZE)     // 4
#define JEBP__UV_PIXEL_BITS (JEBP__UV_BITS + JEBP__BLOCK_BITS) // 3
#define JEBP__UV_PIXEL_SIZE (1 << JEBP__UV_PIXEL_BITS)         // 8

typedef enum jebp__y_flags_t {
    JEBP__B_PRED_MASK = 0x7f,
    JEBP__Y_NONZERO = 0x80
} jebp__y_flags_t;

typedef enum jebp__uv_flags_t {
    JEBP__U_NONZERO = 0x01,
    JEBP__V_NONZERO = 0x02
} jebp__uv_flags_t;

typedef enum jebp__vp8_pred_type_t {
    JEBP__VP8_PRED_DC,   // Predict DC only
    JEBP__VP8_PRED_TM,   // "True-Motion"
    JEBP__VP8_PRED_V,    // Vertical
    JEBP__VP8_PRED_H,    // Horizontal
    JEBP__VP8_PRED_DC_L, // Left-only DC
    JEBP__VP8_PRED_DC_T, // Top-only DC
    JEBP__VP8_PRED_B,    // Per-block prediction
    JEBP__NB_Y_PRED_TYPES,
    JEBP__NB_UV_PRED_TYPES = JEBP__VP8_PRED_B
} jebp__vp8_pred_type_t;

typedef enum jebp__b_pred_type_t {
    JEBP__B_PRED_DC, // Predict DC only
    JEBP__B_PRED_TM, // "True-motion"
    JEBP__B_PRED_VE, // Vertical (S)
    JEBP__B_PRED_HE, // Horizontal (E)
    JEBP__B_PRED_LD, // Left-down (SW)
    JEBP__B_PRED_RD, // Right-down (SE)
    JEBP__B_PRED_VR, // Vertical-right (SSE)
    JEBP__B_PRED_VL, // Vertical-left (SSW)
    JEBP__B_PRED_HD, // Horizontal-down (ESE)
    JEBP__B_PRED_HU, // Horizontal-up (ENE)
    JEBP__NB_B_PRED_TYPES
} jebp__b_pred_type_t;

typedef struct jebp__macro_state_t {
    jebp_ubyte y_flags[JEBP__Y_SIZE];   // jebp__y_flags_t | jebp__b_pred_type_t
    jebp_ubyte uv_flags[JEBP__UV_SIZE]; // jebp__uv_flags_t
    jebp_ubyte y2_flags;                // jebp__y_flags_t
} jebp__macro_state_t;

typedef struct jebp__macro_state_pair_t {
    jebp__macro_state_t *top;
    jebp__macro_state_t *left;
} jebp__macro_state_pair_t;

typedef struct jebp__macro_header_t {
    jebp__vp8_header_t *vp8;
    jebp_int x;
    jebp_int y;
    jebp__segment_t *segment;
    jebp__vp8_pred_type_t y_pred;
    jebp__vp8_pred_type_t uv_pred;
    jebp__b_pred_type_t b_preds[JEBP__NB_Y_BLOCKS];
} jebp__macro_header_t;

static const jebp_byte jebp__segment_tree[JEBP__NB_TREE(JEBP__NB_SEGMENTS)];
static const jebp_byte jebp__y_pred_tree[JEBP__NB_TREE(JEBP__NB_Y_PRED_TYPES)];
static const jebp_ubyte
    jebp__y_pred_probs[JEBP__NB_PROBS(JEBP__NB_Y_PRED_TYPES)];
static const jebp_byte jebp__b_pred_tree[JEBP__NB_TREE(JEBP__NB_B_PRED_TYPES)];
static const jebp_ubyte
    jebp__b_pred_probs[JEBP__NB_B_PRED_TYPES][JEBP__NB_B_PRED_TYPES]
                      [JEBP__NB_PROBS(JEBP__NB_B_PRED_TYPES)];
static const jebp_byte
    jebp__uv_pred_tree[JEBP__NB_TREE(JEBP__NB_UV_PRED_TYPES)];
static const jebp_ubyte
    jebp__uv_pred_probs[JEBP__NB_PROBS(JEBP__NB_UV_PRED_TYPES)];

static jebp_error_t jebp__read_macro_header(jebp__macro_header_t *hdr,
                                            jebp__macro_state_pair_t state,
                                            jebp__bec_reader_t *bec) {
    jebp_error_t err = JEBP_OK;
    jebp_int segment = 0;
    if (hdr->vp8->segment_type == JEBP__SEGMENT_ID) {
        segment = jebp__read_tree(bec, jebp__segment_tree,
                                  hdr->vp8->segment_probs, &err);
    }
    hdr->segment = &hdr->vp8->segments[segment];

    hdr->y_pred =
        jebp__read_tree(bec, jebp__y_pred_tree, jebp__y_pred_probs, &err);
    jebp__b_pred_type_t b_top[JEBP__Y_SIZE];
    jebp__b_pred_type_t b_left[JEBP__Y_SIZE];
    for (jebp_int i = 0; i < JEBP__Y_SIZE; i += 1) {
        if (hdr->y_pred == JEBP__VP8_PRED_B) {
            // We read out the previous subblock predictions from the state now
            // to both make the code cleaner and to potentially improve
            // performance (rather than reading & writing the state for every
            // subblock)
            b_top[i] = state.top->y_flags[i] & JEBP__B_PRED_MASK;
            b_left[i] = state.left->y_flags[i] & JEBP__B_PRED_MASK;
        } else {
            // If we're not decoding B prediction subblocks we instead use this
            // iteration to copy over the fake subblock modes used for the
            // probabilities which will be written back to the state at the end
            b_top[i] = (jebp__b_pred_type_t)hdr->y_pred;
            b_left[i] = (jebp__b_pred_type_t)hdr->y_pred;
        }
    }

    if (hdr->y_pred == JEBP__VP8_PRED_B) {
        for (jebp_int y = 0; y < JEBP__Y_SIZE; y += 1) {
            for (jebp_int x = 0; x < JEBP__Y_SIZE; x += 1) {
                jebp_int i = y * JEBP__Y_SIZE + x;
                hdr->b_preds[i] = jebp__read_tree(
                    bec, jebp__b_pred_tree,
                    jebp__b_pred_probs[b_top[x]][b_left[y]], &err);
                b_top[x] = hdr->b_preds[i];
                b_left[y] = hdr->b_preds[i];
            }
        }
    }

    for (jebp_int i = 0; i < JEBP__Y_SIZE; i += 1) {
        JEBP__SET_MASK(state.top->y_flags[i], JEBP__B_PRED_MASK, b_top[i]);
        JEBP__SET_MASK(state.left->y_flags[i], JEBP__B_PRED_MASK, b_left[i]);
    }
    hdr->uv_pred =
        jebp__read_tree(bec, jebp__uv_pred_tree, jebp__uv_pred_probs, &err);
    return err;
}

/**
 * DCT and WHT inversions
 */
// Utility macros that does 16-bit fixed-point multiplications
// Multiplies against cos(pi/8)*sqrt(2)
#define JEBP__DCT_COS(x) ((x) + (((x)*20091) >> 16))
// Multiplies against sin(pi/8)*sqrt(2)
#define JEBP__DCT_SIN(x) (((x)*35468) >> 16)

#if defined(JEBP__SIMD_NEON)
JEBP__INLINE int16x8_t jebp__neon_getlo_s16x8(int16x8_t v1, int16x8_t v2) {
#ifdef JEBP__SIMD_NEON64
    int64x2_t v_lo =
        vuzp1q_s64(vreinterpretq_s64_s16(v1), vreinterpretq_s64_s16(v2));
    return vreinterpretq_s16_s64(v_lo);
#else  // JEBP__SIMD_NEON64
    return vcombine_s16(vget_low_s16(v1), vget_low_s16(v2));
#endif // JEBP__SIMD_NEON64
}

JEBP__INLINE int16x8_t jebp__neon_gethi_s16x8(int16x8_t v1, int16x8_t v2) {
#ifdef JEBP__SIMD_NEON64
    int64x2_t v_hi =
        vuzp2q_s64(vreinterpretq_s64_s16(v1), vreinterpretq_s64_s16(v2));
    return vreinterpretq_s16_s64(v_hi);
#else  // JEBP__SIMD_NEON64
    return vcombine_s16(vget_high_s16(v1), vget_high_s16(v2));
#endif // JEBP__SIMD_NEON64
}

JEBP__INLINE int16x8_t jebp__neon_dctcos_s16x8(int16x8_t v_dct) {
    int16x8_t v_cos = vqdmulhq_n_s16(v_dct, 20091);
    return vsraq_n_s16(v_dct, v_cos, 1);
}

JEBP__INLINE int16x8_t jebp__neon_dctsin_s16x8(int16x8_t v_dct) {
    return vqdmulhq_n_s16(v_dct, 17734);
}
#endif

static void jebp__invert_dct(jebp_short *dct) {
#if defined(JEBP__SIMD_NEON)
    int16x8_t v_sign = vcombine_s16(vdup_n_s16(1), vdup_n_s16(-1));
    int16x4x4_t v_dct4;
#ifdef JEBP__SIMD_NEON64
    int64x2x2_t v_dct64 = vld2q_s64((int64_t *)dct);
    int16x8_t v_dct0 = vreinterpretq_s16_s64(v_dct64.val[0]);
    int16x8_t v_dct1 = vreinterpretq_s16_s64(v_dct64.val[1]);
#ifndef JEBP__LITTLE_ENDIAN
    v_dct0 = vrev64q_s16(v_dct0);
    v_dct1 = vrev64q_s16(v_dct1);
#endif // JEBP__LITTLE_ENDIAN
#else  // JEBP__SIMD_NEON64
    v_dct4 = vld1_s16_x4(dct);
    int16x8_t v_dct0 = vcombine_s16(v_dct4.val[0], v_dct4.val[2]);
    int16x8_t v_dct1 = vcombine_s16(v_dct4.val[1], v_dct4.val[3]);
#endif // JEBP__SIMD_NEON64
    // Vertical pass
    int16x8_t v_lo = jebp__neon_getlo_s16x8(v_dct0, v_dct0);
    int16x8_t v_hi = jebp__neon_gethi_s16x8(v_dct0, v_dct0);
    int16x8_t v_t01 = vmlaq_s16(v_lo, v_hi, v_sign);
    int16x8_t v_cos = jebp__neon_dctcos_s16x8(v_dct1);
    int16x8_t v_sin = jebp__neon_dctsin_s16x8(v_dct1);
    v_lo = jebp__neon_getlo_s16x8(v_cos, v_sin);
    v_hi = jebp__neon_gethi_s16x8(v_sin, v_cos);
    int16x8_t v_t32 = vmlaq_s16(v_lo, v_hi, v_sign);
    v_dct0 = vaddq_s16(v_t01, v_t32);
    v_dct1 = vsubq_s16(v_t01, v_t32);
    v_dct1 = vextq_s16(v_dct1, v_dct1, 4);
    // Horizontal pass
    int16x8x2_t v_dct = vuzpq_s16(v_dct0, v_dct1);
    int16x8x2_t v_evod = vuzpq_s16(v_dct.val[0], v_dct.val[0]);
    v_t01 = vmlaq_s16(v_evod.val[0], v_evod.val[1], v_sign);
    v_cos = jebp__neon_dctcos_s16x8(v_dct.val[1]);
    v_sin = jebp__neon_dctsin_s16x8(v_dct.val[1]);
#ifdef JEBP__SIMD_NEON64
    int16x8_t v_even = vuzp1q_s16(v_cos, v_sin);
    int16x8_t v_odd = vuzp2q_s16(v_sin, v_cos);
#else  // JEBP__SIMD_NEON64
    v_evod = vuzpq_s16(v_cos, v_sin);
    int16x8_t v_even = v_evod.val[0];
    int16x8_t v_odd = vextq_s16(v_evod.val[1], v_evod.val[1], 4);
#endif // JEBP__SIMD_NEON64
    v_t32 = vmlaq_s16(v_even, v_odd, v_sign);
    v_dct0 = vaddq_s16(v_t01, v_t32);
    v_dct1 = vsubq_s16(v_t01, v_t32);
    // Rounding and store
    v_dct0 = vrshrq_n_s16(v_dct0, 3);
    v_dct1 = vrshrq_n_s16(v_dct1, 3);
    v_dct4.val[0] = vget_low_s16(v_dct0);
    v_dct4.val[1] = vget_high_s16(v_dct0);
    // Saves a vext call by rotating it here
    v_dct4.val[2] = vget_high_s16(v_dct1);
    v_dct4.val[3] = vget_low_s16(v_dct1);
    vst4_s16(dct, v_dct4);
#else
    for (jebp_int i = 0; i < JEBP__BLOCK_SIZE; i += 1) {
        jebp_short *col = &dct[i];
        jebp_int t0 = col[0] + col[8];
        jebp_int t1 = col[0] - col[8];
        jebp_int t2 = JEBP__DCT_SIN(col[4]) - JEBP__DCT_COS(col[12]);
        jebp_int t3 = JEBP__DCT_COS(col[4]) + JEBP__DCT_SIN(col[12]);
        col[0] = t0 + t3;
        col[4] = t1 + t2;
        col[8] = t1 - t2;
        col[12] = t0 - t3;
    }
    for (jebp_int i = 0; i < JEBP__BLOCK_SIZE; i += 1) {
        jebp_short *row = &dct[i * JEBP__BLOCK_SIZE];
        jebp_int t0 = row[0] + row[2];
        jebp_int t1 = row[0] - row[2];
        jebp_int t2 = JEBP__DCT_SIN(row[1]) - JEBP__DCT_COS(row[3]);
        jebp_int t3 = JEBP__DCT_COS(row[1]) + JEBP__DCT_SIN(row[3]);
        row[0] = JEBP__RSHIFT(t0 + t3, 3);
        row[1] = JEBP__RSHIFT(t1 + t2, 3);
        row[2] = JEBP__RSHIFT(t1 - t2, 3);
        row[3] = JEBP__RSHIFT(t0 - t3, 3);
    }
#endif
}

static void jebp__invert_wht(jebp_short *wht) {
#if defined(JEBP__SIMD_NEON)
    int16x8_t v_round = vdupq_n_s16(3);
    int16x8x2_t v_wht = vld1q_s16_x2(wht);
    // Vertical pass
    int16x8_t v_wht0 = v_wht.val[0];
    int16x8_t v_wht1 = vextq_s16(v_wht.val[1], v_wht.val[1], 4);
    int16x8_t v_t01 = vaddq_s16(v_wht0, v_wht1);
    int16x8_t v_t32 = vsubq_s16(v_wht0, v_wht1);
    int16x8_t v_t03 = jebp__neon_getlo_s16x8(v_t01, v_t32);
    int16x8_t v_t12 = jebp__neon_gethi_s16x8(v_t01, v_t32);
    int32x4_t v_wht0_32 = vreinterpretq_s32_s16(vaddq_s16(v_t03, v_t12));
    int32x4_t v_wht1_32 = vreinterpretq_s32_s16(vsubq_s16(v_t03, v_t12));
    // Horizontal pass
    int32x4x2_t v_wht32 = vuzpq_s32(v_wht0_32, v_wht1_32);
    v_wht0 = vreinterpretq_s16_s32(v_wht32.val[0]);
    v_wht1 = vrev32q_s16(vreinterpretq_s16_s32(v_wht32.val[1]));
    v_t01 = vaddq_s16(v_wht0, v_wht1);
    v_t32 = vsubq_s16(v_wht0, v_wht1);
    int16x8x2_t v_tmp = vuzpq_s16(v_t01, v_t32);
    v_wht0 = vaddq_s16(v_tmp.val[0], v_tmp.val[1]);
    v_wht1 = vsubq_s16(v_tmp.val[0], v_tmp.val[1]);
    // Rounding and store
    v_wht0 = vaddq_s16(v_wht0, v_round);
    v_wht1 = vaddq_s16(v_wht1, v_round);
    v_wht0 = vshrq_n_s16(v_wht0, 3);
    v_wht1 = vshrq_n_s16(v_wht1, 3);
    int16x4x4_t v_wht4;
    v_wht4.val[0] = vget_low_s16(v_wht0);
    v_wht4.val[1] = vget_high_s16(v_wht0);
    v_wht4.val[2] = vget_low_s16(v_wht1);
    v_wht4.val[3] = vget_high_s16(v_wht1);
    vst4_s16(wht, v_wht4);
#else
    for (jebp_int i = 0; i < JEBP__BLOCK_SIZE; i += 1) {
        jebp_short *col = &wht[i];
        jebp_int t0 = col[0] + col[12];
        jebp_int t1 = col[4] + col[8];
        jebp_int t2 = col[4] - col[8];
        jebp_int t3 = col[0] - col[12];
        col[0] = t0 + t1;
        col[4] = t2 + t3;
        col[8] = t0 - t1;
        col[12] = t3 - t2;
    }
    for (jebp_int i = 0; i < JEBP__BLOCK_SIZE; i += 1) {
        jebp_short *row = &wht[i * JEBP__BLOCK_SIZE];
        jebp_int t0 = row[0] + row[3];
        jebp_int t1 = row[1] + row[2];
        jebp_int t2 = row[1] - row[2];
        jebp_int t3 = row[0] - row[3];
        // These use a different rounding value and thus can't use RSHIFT
        row[0] = (t0 + t1 + 3) >> 3;
        row[1] = (t2 + t3 + 3) >> 3;
        row[2] = (t0 - t1 + 3) >> 3;
        row[3] = (t3 - t2 + 3) >> 3;
    }
#endif
}

/**
 * VP8 predictions
 */
typedef void (*jebp__vp8_pred_t)(jebp_ubyte *pred, jebp_int stride);
typedef void (*jebp__b_pred_t)(jebp_ubyte *pred, jebp_int stride,
                               jebp_ubyte *tr);

// UV predictions

static void jebp__uv_pred_fill(jebp_ubyte *pred, jebp_int stride,
                               jebp_ubyte value) {
    for (jebp_int y = 0; y < JEBP__UV_PIXEL_SIZE; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        memset(row, value, JEBP__UV_PIXEL_SIZE);
    }
}

static jebp_int jebp__uv_pred_sum_l(jebp_ubyte *pred, jebp_int stride) {
    jebp_int sum = 0;
    for (jebp_int i = 0; i < JEBP__UV_PIXEL_SIZE; i += 1) {
        jebp_ubyte *row = &pred[i * stride];
        sum += row[-1];
    }
    return sum;
}

static jebp_int jebp__uv_pred_sum_t(jebp_ubyte *pred, jebp_int stride) {
    jebp_ubyte *top = &pred[-stride];
#if defined(JEBP__SIMD_NEON)
    uint8x8_t v_top = vld1_u8(top);
#ifdef JEBP__SIMD_NEON64
    return vaddlv_u8(v_top);
#else  // JEBP__SIMD_NEON64
    uint16x4_t v_top4 = vpaddl_u8(v_top);
    uint16x4_t v_top2 = vpadd_u16(v_top4, v_top4);
    uint16x4_t v_top1 = vpadd_u16(v_top2, v_top2);
    return vget_lane_u16(v_top1, 0);
#endif // JEBP__SIMD_NEON64
#else
    jebp_int sum = 0;
    for (jebp_int i = 0; i < JEBP__UV_PIXEL_SIZE; i += 1) {
        sum += top[i];
    }
    return sum;
#endif
}

static void jebp__uv_pred_dc(jebp_ubyte *pred, jebp_int stride) {
    jebp_int sum =
        jebp__uv_pred_sum_t(pred, stride) + jebp__uv_pred_sum_l(pred, stride);
    jebp_ubyte dc = JEBP__RSHIFT(sum, 4);
    jebp__uv_pred_fill(pred, stride, dc);
}

// For handling DC prediction on top and left macroblocks
static void jebp__uv_pred_dc_l(jebp_ubyte *pred, jebp_int stride) {
    jebp_int sum = jebp__uv_pred_sum_l(pred, stride);
    jebp_ubyte dc = JEBP__RSHIFT(sum, 3);
    jebp__uv_pred_fill(pred, stride, dc);
}

static void jebp__uv_pred_dc_t(jebp_ubyte *pred, jebp_int stride) {
    jebp_int sum = jebp__uv_pred_sum_t(pred, stride);
    jebp_ubyte dc = JEBP__RSHIFT(sum, 3);
    jebp__uv_pred_fill(pred, stride, dc);
}

static void jebp__uv_pred_tm(jebp_ubyte *pred, jebp_int stride) {
    jebp_ubyte *top = &pred[-stride];
#if defined(JEBP__SIMD_NEON)
    uint8x8_t v_toplo = vld1_u8(top);
    uint8x16_t v_top = vcombine_u8(v_toplo, v_toplo);
    uint8x16_t v_tl = vld1q_dup_u8(&top[-1]);
    uint8x16_t v_diff = vabdq_u8(v_top, v_tl);
    uint8x16_t v_neg = vcltq_u8(v_top, v_tl);
    for (jebp_int y = 0; y < JEBP__UV_PIXEL_SIZE; y += 2) {
        jebp_ubyte *rowlo = &pred[(y + 0) * stride];
        jebp_ubyte *rowhi = &pred[(y + 1) * stride];
        uint8x16_t v_left =
            vcombine_u8(vld1_dup_u8(&rowlo[-1]), vld1_dup_u8(&rowhi[-1]));
        uint8x16_t v_add = vqaddq_u8(v_left, v_diff);
        uint8x16_t v_sub = vqsubq_u8(v_left, v_diff);
        uint8x16_t v_row = vbslq_u8(v_neg, v_sub, v_add);
        vst1_u8(rowlo, vget_low_u8(v_row));
        vst1_u8(rowhi, vget_high_u8(v_row));
    }
#else
    for (jebp_int y = 0; y < JEBP__UV_PIXEL_SIZE; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        jebp_int diff = row[-1] - top[-1];
        for (jebp_int x = 0; x < JEBP__UV_PIXEL_SIZE; x += 1) {
            row[x] = JEBP__CLAMP_UBYTE(diff + top[x]);
        }
    }
#endif
}

static void jebp__uv_pred_v(jebp_ubyte *pred, jebp_int stride) {
    // This might look dumb but on most compilers this prevents repetive loads
    // TODO: msvc compiling for ARM still struggles with this but eh
    jebp_ubyte top[JEBP__UV_PIXEL_SIZE];
    memcpy(top, &pred[-stride], JEBP__UV_PIXEL_SIZE);
    for (jebp_int y = 0; y < JEBP__UV_PIXEL_SIZE; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        memcpy(row, top, JEBP__UV_PIXEL_SIZE);
    }
}

static void jebp__uv_pred_h(jebp_ubyte *pred, jebp_int stride) {
    for (jebp_int y = 0; y < JEBP__UV_PIXEL_SIZE; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        memset(row, row[-1], JEBP__UV_PIXEL_SIZE);
    }
}

// Y predictions

static void jebp__y_pred_fill(jebp_ubyte *pred, jebp_int stride,
                              jebp_ubyte value) {
    for (jebp_int y = 0; y < JEBP__Y_PIXEL_SIZE; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        memset(row, value, JEBP__Y_PIXEL_SIZE);
    }
}

static jebp_int jebp__y_pred_sum_l(jebp_ubyte *pred, jebp_int stride) {
    jebp_int sum = 0;
    for (jebp_int i = 0; i < JEBP__Y_PIXEL_SIZE; i += 1) {
        jebp_ubyte *row = &pred[i * stride];
        sum += row[-1];
    }
    return sum;
}

static jebp_int jebp__y_pred_sum_t(jebp_ubyte *pred, jebp_int stride) {
    jebp_ubyte *top = &pred[-stride];
#if defined(JEBP__SIMD_NEON)
    uint8x16_t v_top = vld1q_u8(top);
#ifdef JEBP__SIMD_NEON64
    return vaddlvq_u8(v_top);
#else  // JEBP__SIMD_NEON64
    uint16x8_t v_top8 = vaddl_u8(vget_low_u8(v_top), vget_high_u8(v_top));
    uint16x4_t v_top4 = vadd_u16(vget_low_u16(v_top8), vget_high_u16(v_top8));
    uint16x4_t v_top2 = vpadd_u16(v_top4, v_top4);
    uint16x4_t v_top1 = vpadd_u16(v_top2, v_top2);
    return vget_lane_u16(v_top1, 0);
#endif // JEBP__SIMD_NEON64
#else
    jebp_int sum = 0;
    for (jebp_int i = 0; i < JEBP__Y_PIXEL_SIZE; i += 1) {
        sum += top[i];
    }
    return sum;
#endif
}

static void jebp__y_pred_dc(jebp_ubyte *pred, jebp_int stride) {
    jebp_int sum =
        jebp__y_pred_sum_t(pred, stride) + jebp__y_pred_sum_l(pred, stride);
    jebp_ubyte dc = JEBP__RSHIFT(sum, 5);
    jebp__y_pred_fill(pred, stride, dc);
}

static void jebp__y_pred_dc_l(jebp_ubyte *pred, jebp_int stride) {
    jebp_int sum = jebp__y_pred_sum_l(pred, stride);
    jebp_ubyte dc = JEBP__RSHIFT(sum, 4);
    jebp__y_pred_fill(pred, stride, dc);
}

static void jebp__y_pred_dc_t(jebp_ubyte *pred, jebp_int stride) {
    jebp_int sum = jebp__y_pred_sum_t(pred, stride);
    jebp_ubyte dc = JEBP__RSHIFT(sum, 4);
    jebp__y_pred_fill(pred, stride, dc);
}

static void jebp__y_pred_tm(jebp_ubyte *pred, jebp_int stride) {
    jebp_ubyte *top = &pred[-stride];
#if defined(JEBP__SIMD_NEON)
    uint8x16_t v_top = vld1q_u8(top);
    uint8x16_t v_tl = vld1q_dup_u8(&top[-1]);
    uint8x16_t v_diff = vabdq_u8(v_top, v_tl);
    uint8x16_t v_neg = vcltq_u8(v_top, v_tl);
    for (jebp_int y = 0; y < JEBP__Y_PIXEL_SIZE; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        uint8x16_t v_left = vld1q_dup_u8(&row[-1]);
        uint8x16_t v_add = vqaddq_u8(v_left, v_diff);
        uint8x16_t v_sub = vqsubq_u8(v_left, v_diff);
        uint8x16_t v_row = vbslq_u8(v_neg, v_sub, v_add);
        vst1q_u8(row, v_row);
    }
#else
    for (jebp_int y = 0; y < JEBP__Y_PIXEL_SIZE; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        jebp_int diff = row[-1] - top[-1];
        for (jebp_int x = 0; x < JEBP__Y_PIXEL_SIZE; x += 1) {
            row[x] = JEBP__CLAMP_UBYTE(diff + top[x]);
        }
    }
#endif
}

static void jebp__y_pred_v(jebp_ubyte *pred, jebp_int stride) {
    jebp_ubyte top[JEBP__Y_PIXEL_SIZE];
    memcpy(top, &pred[-stride], JEBP__Y_PIXEL_SIZE);
    for (jebp_int y = 0; y < JEBP__Y_PIXEL_SIZE; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        memcpy(row, top, JEBP__Y_PIXEL_SIZE);
    }
}

static void jebp__y_pred_h(jebp_ubyte *pred, jebp_int stride) {
    for (jebp_int y = 0; y < JEBP__Y_PIXEL_SIZE; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        memset(row, row[-1], JEBP__Y_PIXEL_SIZE);
    }
}

// B predictions

static void jebp__b_pred_fill(jebp_ubyte *pred, jebp_int stride,
                              jebp_ubyte value) {
    memset(&pred[0 * stride], value, JEBP__BLOCK_SIZE);
    memset(&pred[1 * stride], value, JEBP__BLOCK_SIZE);
    memset(&pred[2 * stride], value, JEBP__BLOCK_SIZE);
    memset(&pred[3 * stride], value, JEBP__BLOCK_SIZE);
}

static void jebp__b_pred_dc(jebp_ubyte *pred, jebp_int stride, jebp_ubyte *tr) {
    (void)tr;
    jebp_int sum = 0;
    jebp_ubyte *top = &pred[-stride];
    sum += top[0] + top[1] + top[2] + top[3];
    jebp_ubyte *left = &pred[-1];
    sum += left[0 * stride] + left[1 * stride] + left[2 * stride] +
           left[3 * stride];
    jebp_ubyte dc = JEBP__RSHIFT(sum, 3);
    jebp__b_pred_fill(pred, stride, dc);
}

static void jebp__b_pred_tm(jebp_ubyte *pred, jebp_int stride, jebp_ubyte *tr) {
    (void)tr;
    jebp_ubyte *top = &pred[-stride];
#if defined(JEBP__SIMD_NEON)
    uint8x16_t v_top = vreinterpretq_u8_u32(vld1q_dup_u32((uint32_t *)top));
    uint8x16_t v_tl = vld1q_dup_u8(&top[-1]);
    uint8x16_t v_diff = vabdq_u8(v_top, v_tl);
    uint8x16_t v_neg = vcltq_u8(v_top, v_tl);
    uint8x16_t v_left = vdupq_n_u8(0);
    v_left = vld1q_lane_u8(&pred[0 * stride - 1], v_left, 0);
    v_left = vld1q_lane_u8(&pred[1 * stride - 1], v_left, 4);
    v_left = vld1q_lane_u8(&pred[2 * stride - 1], v_left, 8);
    v_left = vld1q_lane_u8(&pred[3 * stride - 1], v_left, 12);
    v_left = vreinterpretq_u8_u32(
        vmulq_n_u32(vreinterpretq_u32_u8(v_left), 0x01010101));
    uint8x16_t v_add = vqaddq_u8(v_left, v_diff);
    uint8x16_t v_sub = vqsubq_u8(v_left, v_diff);
    uint32x4_t v_row = vreinterpretq_u32_u8(vbslq_u8(v_neg, v_sub, v_add));
    vst1q_lane_u32((uint32_t *)&pred[0 * stride], v_row, 0);
    vst1q_lane_u32((uint32_t *)&pred[1 * stride], v_row, 1);
    vst1q_lane_u32((uint32_t *)&pred[2 * stride], v_row, 2);
    vst1q_lane_u32((uint32_t *)&pred[3 * stride], v_row, 3);
#else
    for (jebp_int y = 0; y < JEBP__BLOCK_SIZE; y += 1) {
        jebp_ubyte *row = &pred[y * stride];
        jebp_int diff = row[-1] - top[-1];
        row[0] = JEBP__CLAMP_UBYTE(diff + top[0]);
        row[1] = JEBP__CLAMP_UBYTE(diff + top[1]);
        row[2] = JEBP__CLAMP_UBYTE(diff + top[2]);
        row[3] = JEBP__CLAMP_UBYTE(diff + top[3]);
    }
#endif
}

static void jebp__b_pred_ve(jebp_ubyte *pred, jebp_int stride, jebp_ubyte *tr) {
    jebp_ubyte *top = &pred[-stride];
    jebp_ubyte avg[4];
    avg[0] = JEBP__RAVG3(top[-1], top[0], top[1]);
    avg[1] = JEBP__RAVG3(top[0], top[1], top[2]);
    avg[2] = JEBP__RAVG3(top[1], top[2], top[3]);
    avg[3] = JEBP__RAVG3(top[2], top[3], tr[0]);
    memcpy(&pred[0 * stride], avg, JEBP__BLOCK_SIZE);
    memcpy(&pred[1 * stride], avg, JEBP__BLOCK_SIZE);
    memcpy(&pred[2 * stride], avg, JEBP__BLOCK_SIZE);
    memcpy(&pred[3 * stride], avg, JEBP__BLOCK_SIZE);
}

static void jebp__b_pred_he(jebp_ubyte *pred, jebp_int stride, jebp_ubyte *tr) {
    (void)tr;
    jebp_ubyte *top = &pred[-stride];
    jebp_ubyte *r0 = &pred[0 * stride];
    jebp_ubyte *r1 = &pred[1 * stride];
    jebp_ubyte *r2 = &pred[2 * stride];
    jebp_ubyte *r3 = &pred[3 * stride];
    memset(r0, JEBP__RAVG3(top[-1], r0[-1], r1[-1]), JEBP__BLOCK_SIZE);
    memset(r1, JEBP__RAVG3(r0[-1], r1[-1], r2[-1]), JEBP__BLOCK_SIZE);
    memset(r2, JEBP__RAVG3(r1[-1], r2[-1], r3[-1]), JEBP__BLOCK_SIZE);
    memset(r3, JEBP__RAVG3(r2[-1], r3[-1], r3[-1]), JEBP__BLOCK_SIZE);
}

static void jebp__b_pred_ld(jebp_ubyte *pred, jebp_int stride, jebp_ubyte *tr) {
    jebp_ubyte *top = &pred[-stride];
    jebp_ubyte *r0 = &pred[0 * stride];
    jebp_ubyte *r1 = &pred[1 * stride];
    jebp_ubyte *r2 = &pred[2 * stride];
    jebp_ubyte *r3 = &pred[3 * stride];
    r0[0] = JEBP__RAVG3(top[0], top[1], top[2]);
    r0[1] = r1[0] = JEBP__RAVG3(top[1], top[2], top[3]);
    r0[2] = r1[1] = r2[0] = JEBP__RAVG3(top[2], top[3], tr[0]);
    r0[3] = r1[2] = r2[1] = r3[0] = JEBP__RAVG3(top[3], tr[0], tr[1]);
    r1[3] = r2[2] = r3[1] = JEBP__RAVG3(tr[0], tr[1], tr[2]);
    r2[3] = r3[2] = JEBP__RAVG3(tr[1], tr[2], tr[3]);
    r3[3] = JEBP__RAVG3(tr[2], tr[3], tr[3]);
}

static void jebp__b_pred_rd(jebp_ubyte *pred, jebp_int stride, jebp_ubyte *tr) {
    (void)tr;
    jebp_ubyte *top = &pred[-stride];
    jebp_ubyte *r0 = &pred[0 * stride];
    jebp_ubyte *r1 = &pred[1 * stride];
    jebp_ubyte *r2 = &pred[2 * stride];
    jebp_ubyte *r3 = &pred[3 * stride];
    r3[0] = JEBP__RAVG3(r3[-1], r2[-1], r1[-1]);
    r2[0] = r3[1] = JEBP__RAVG3(r2[-1], r1[-1], r0[-1]);
    r1[0] = r2[1] = r3[2] = JEBP__RAVG3(r1[-1], r0[-1], top[-1]);
    r0[0] = r1[1] = r2[2] = r3[3] = JEBP__RAVG3(r0[-1], top[-1], top[0]);
    r0[1] = r1[2] = r2[3] = JEBP__RAVG3(top[-1], top[0], top[1]);
    r0[2] = r1[3] = JEBP__RAVG3(top[0], top[1], top[2]);
    r0[3] = JEBP__RAVG3(top[1], top[2], top[3]);
}

static void jebp__b_pred_vr(jebp_ubyte *pred, jebp_int stride, jebp_ubyte *tr) {
    (void)tr;
    jebp_ubyte *top = &pred[-stride];
    jebp_ubyte *r0 = &pred[0 * stride];
    jebp_ubyte *r1 = &pred[1 * stride];
    jebp_ubyte *r2 = &pred[2 * stride];
    jebp_ubyte *r3 = &pred[3 * stride];
    r3[0] = JEBP__RAVG3(r2[-1], r1[-1], r0[-1]);
    r2[0] = JEBP__RAVG3(r1[-1], r0[-1], top[-1]);
    r1[0] = r3[1] = JEBP__RAVG3(r0[-1], top[-1], top[0]);
    r0[0] = r2[1] = JEBP__RAVG(top[-1], top[0]);
    r1[1] = r3[2] = JEBP__RAVG3(top[-1], top[0], top[1]);
    r0[1] = r2[2] = JEBP__RAVG(top[0], top[1]);
    r1[2] = r3[3] = JEBP__RAVG3(top[0], top[1], top[2]);
    r0[2] = r2[3] = JEBP__RAVG(top[1], top[2]);
    r1[3] = JEBP__RAVG3(top[1], top[2], top[3]);
    r0[3] = JEBP__RAVG(top[2], top[3]);
}

static void jebp__b_pred_vl(jebp_ubyte *pred, jebp_int stride, jebp_ubyte *tr) {
    jebp_ubyte *top = &pred[-stride];
    jebp_ubyte *r0 = &pred[0 * stride];
    jebp_ubyte *r1 = &pred[1 * stride];
    jebp_ubyte *r2 = &pred[2 * stride];
    jebp_ubyte *r3 = &pred[3 * stride];
    r0[0] = JEBP__RAVG(top[0], top[1]);
    r1[0] = JEBP__RAVG3(top[0], top[1], top[2]);
    r0[1] = r2[0] = JEBP__RAVG(top[1], top[2]);
    r1[1] = r3[0] = JEBP__RAVG3(top[1], top[2], top[3]);
    r0[2] = r2[1] = JEBP__RAVG(top[2], top[3]);
    r1[2] = r3[1] = JEBP__RAVG3(top[2], top[3], tr[0]);
    r0[3] = r2[2] = JEBP__RAVG(top[3], tr[0]);
    r1[3] = r3[2] = JEBP__RAVG3(top[3], tr[0], tr[1]);
    // These last two do not follow the same pattern
    r2[3] = JEBP__RAVG3(tr[0], tr[1], tr[2]);
    r3[3] = JEBP__RAVG3(tr[1], tr[2], tr[3]);
}

static void jebp__b_pred_hd(jebp_ubyte *pred, jebp_int stride, jebp_ubyte *tr) {
    (void)tr;
    jebp_ubyte *top = &pred[-stride];
    jebp_ubyte *r0 = &pred[0 * stride];
    jebp_ubyte *r1 = &pred[1 * stride];
    jebp_ubyte *r2 = &pred[2 * stride];
    jebp_ubyte *r3 = &pred[3 * stride];
    r3[0] = JEBP__RAVG(r3[-1], r2[-1]);
    r3[1] = JEBP__RAVG3(r3[-1], r2[-1], r1[-1]);
    r2[0] = r3[2] = JEBP__RAVG(r2[-1], r1[-1]);
    r2[1] = r3[3] = JEBP__RAVG3(r2[-1], r1[-1], r0[-1]);
    r1[0] = r2[2] = JEBP__RAVG(r1[-1], r0[-1]);
    r1[1] = r2[3] = JEBP__RAVG3(r1[-1], r0[-1], top[-1]);
    r0[0] = r1[2] = JEBP__RAVG(r0[-1], top[-1]);
    r0[1] = r1[3] = JEBP__RAVG3(r0[-1], top[-1], top[0]);
    r0[2] = JEBP__RAVG3(top[-1], top[0], top[1]);
    r0[3] = JEBP__RAVG3(top[0], top[1], top[2]);
}

static void jebp__b_pred_hu(jebp_ubyte *pred, jebp_int stride, jebp_ubyte *tr) {
    (void)tr;
    jebp_ubyte *r0 = &pred[0 * stride];
    jebp_ubyte *r1 = &pred[1 * stride];
    jebp_ubyte *r2 = &pred[2 * stride];
    jebp_ubyte *r3 = &pred[3 * stride];
    r0[0] = JEBP__RAVG(r0[-1], r1[-1]);
    r0[1] = JEBP__RAVG3(r0[-1], r1[-1], r2[-1]);
    r1[0] = r0[2] = JEBP__RAVG(r1[-1], r2[-1]);
    r1[1] = r0[3] = JEBP__RAVG3(r1[-1], r2[-1], r3[-1]);
    r2[0] = r1[2] = JEBP__RAVG(r2[-1], r3[-1]);
    r2[1] = r1[3] = JEBP__RAVG3(r2[-1], r3[-1], r3[-1]);
    // The rest cannot be predicted well
    r2[2] = r2[3] = r3[0] = r3[1] = r3[2] = r3[3] = r3[-1];
}

static const jebp__vp8_pred_t jebp__uv_preds[JEBP__NB_UV_PRED_TYPES] = {
    jebp__uv_pred_dc, jebp__uv_pred_tm,   jebp__uv_pred_v,
    jebp__uv_pred_h,  jebp__uv_pred_dc_l, jebp__uv_pred_dc_t};

// Using 'nb. UV pred types' since we don't include B-pred in this list
static const jebp__vp8_pred_t jebp__y_preds[JEBP__NB_UV_PRED_TYPES] = {
    jebp__y_pred_dc, jebp__y_pred_tm,   jebp__y_pred_v,
    jebp__y_pred_h,  jebp__y_pred_dc_l, jebp__y_pred_dc_t};

static const jebp__b_pred_t jebp__b_preds[JEBP__NB_B_PRED_TYPES] = {
    jebp__b_pred_dc, jebp__b_pred_tm, jebp__b_pred_ve, jebp__b_pred_he,
    jebp__b_pred_ld, jebp__b_pred_rd, jebp__b_pred_vr, jebp__b_pred_vl,
    jebp__b_pred_hd, jebp__b_pred_hu};

/**
 * Macroblock data
 */
#define JEBP__MAX_TOKEN_EXTRA 11
#define JEBP__GET_Y_NONZERO(state, index)                                      \
    (((state)->y_flags[index] & JEBP__Y_NONZERO) != 0)
#define JEBP__GET_U_NONZERO(state, index)                                      \
    (((state)->uv_flags[index] & JEBP__U_NONZERO) != 0)
#define JEBP__GET_V_NONZERO(state, index)                                      \
    (((state)->uv_flags[index] & JEBP__V_NONZERO) != 0)
#define JEBP__GET_Y2_NONZERO(state) (((state)->y2_flags & JEBP__Y_NONZERO) != 0)

typedef struct jebp__token_extra_t {
    jebp_byte offset;
    jebp_ubyte probs[JEBP__MAX_TOKEN_EXTRA + 1];
} jebp__token_extra_t;

static const jebp_byte jebp__coeff_bands[JEBP__NB_BLOCK_COEFFS];
static const jebp_byte jebp__coeff_order[JEBP__NB_BLOCK_COEFFS];
static const jebp_byte jebp__token_tree[JEBP__NB_TREE(JEBP__NB_TOKENS - 1)];
static const jebp__token_extra_t jebp__token_extra[JEBP__NB_EXTRA_TOKENS];

static jebp__vp8_pred_type_t jebp__vp8_pred_type(jebp__macro_header_t *hdr,
                                                 jebp__vp8_pred_type_t pred) {
    if (pred == JEBP__VP8_PRED_DC) {
        if (hdr->x > 0 && hdr->y == 0) {
            return JEBP__VP8_PRED_DC_L;
        } else if (hdr->x == 0 && hdr->y > 0) {
            return JEBP__VP8_PRED_DC_T;
        }
    }
    return pred;
}

JEBP__INLINE jebp_short jebp__read_token_extrabits(jebp__token_t token,
                                                   jebp__bec_reader_t *bec,
                                                   jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
    const jebp__token_extra_t *extra =
        &jebp__token_extra[token - JEBP__TOKEN_EXTRA1];
    jebp_short value = 0;
    for (const jebp_ubyte *prob = extra->probs; *prob != 0; prob += 1) {
        value = (value << 1) | jebp__read_bool(bec, *prob, err);
    }
    return value + extra->offset;
}

// Returns non-zero if it contains atleast 1 non-zero token
static jebp_int jebp__read_dct(jebp__macro_header_t *hdr, jebp_short *dct,
                               jebp__block_type_t type, jebp_int complex,
                               jebp__bec_reader_t *bec, jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
    jebp_int coeff = type == JEBP__BLOCK_Y1 ? 1 : 0;
    jebp__quants_t *quants = &hdr->segment->quants;
    // We can treat the quants structure as an array of shorts
    // TODO: maybe it should be an array of shorts??
    jebp_short *dcac;
    switch (type) {
    case JEBP__BLOCK_Y2:
        dcac = &quants->y2_dc;
        break;
    case JEBP__BLOCK_UV:
        dcac = &quants->uv_dc;
        break;
    default:
        dcac = &quants->y_dc;
        break;
    }
    // The initial quantizer is DC if starting at 0, or AC for Y1 blocks
    jebp_short quant = dcac[coeff];

    jebp_ubyte(*token_probs)[JEBP__NB_TOKEN_COMPLEXITIES]
                            [JEBP__NB_PROBS(JEBP__NB_TOKENS)] =
                                hdr->vp8->token_probs[type];
    jebp_ubyte *probs = token_probs[jebp__coeff_bands[coeff]][complex];
    if (!jebp__read_bool(bec, probs[0], err)) {
        // First token is EOB, making sure not to clear the first one if the
        // type is Y1
        JEBP__CLEAR(&dct[coeff],
                    (JEBP__NB_BLOCK_COEFFS - coeff) * sizeof(jebp_short));
        return 0;
    }

    for (;;) {
        jebp__token_t token =
            jebp__read_tree(bec, jebp__token_tree, &probs[1], err);
        if (token == JEBP__TOKEN_COEFF0) {
            // If the token is 0, there is no negative flag, the next complexity
            // is 0, and we skip the EOB reading.
            dct[jebp__coeff_order[coeff]] = 0;
            coeff += 1;
            if (coeff >= JEBP__NB_BLOCK_COEFFS) {
                break;
            }
            quant = dcac[1];
            probs = token_probs[jebp__coeff_bands[coeff]][0];
        } else {
            jebp_short value;
            complex = 2;
            if (token == JEBP__TOKEN_COEFF1) {
                // 1 has a complexity of 1
                value = 1;
                complex = 1;
            } else if (token < JEBP__TOKEN_EXTRA1) {
                value = token - JEBP__TOKEN_COEFF0;
            } else {
                value = jebp__read_token_extrabits(token, bec, err);
            }
            if (jebp__read_flag(bec, err)) {
                // Negative value
                value = -value;
            }
            value *= quant;
            dct[jebp__coeff_order[coeff]] = value;

            coeff += 1;
            if (coeff >= JEBP__NB_BLOCK_COEFFS) {
                break;
            }
            quant = dcac[1];
            probs = token_probs[jebp__coeff_bands[coeff]][complex];
            if (!jebp__read_bool(bec, probs[0], err)) {
                // EOB token
                break;
            }
        }
    }

    // Fill the rest after an EOB with 0
    for (; coeff < JEBP__NB_BLOCK_COEFFS; coeff += 1) {
        dct[jebp__coeff_order[coeff]] = 0;
    }
    return 1;
}

// TODO: invert and add DCT at the same time
static void jebp__sum_pred_dct(jebp_ubyte *pred, jebp_int stride,
                               jebp_short *dct) {
#if defined(JEBP__SIMD_NEON)
    uint16x8x2_t v_dct = vld1q_u16_x2((uint16_t *)dct);
    uint32x2_t v_pred32 = vcreate_u32(0);
    for (jebp_int y = 0; y < JEBP__BLOCK_SIZE; y += 2) {
        uint32_t *rowlo = (uint32_t *)&pred[(y + 0) * stride];
        uint32_t *rowhi = (uint32_t *)&pred[(y + 1) * stride];
        v_pred32 = vld1_lane_u32(rowlo, v_pred32, 0);
        v_pred32 = vld1_lane_u32(rowhi, v_pred32, 1);
        uint16x8_t v_pred16 =
            vaddw_u8(v_dct.val[y / 2], vreinterpret_u8_u32(v_pred32));
        uint8x8_t v_pred8 = vqmovun_s16(vreinterpretq_s16_u16(v_pred16));
        v_pred32 = vreinterpret_u8_u32(v_pred8);
        vst1_lane_u32(rowlo, v_pred32, 0);
        vst1_lane_u32(rowhi, v_pred32, 1);
    }
#else
    for (jebp_int i = 0; i < JEBP__BLOCK_SIZE; i += 1) {
        pred[0] = JEBP__CLAMP_UBYTE(pred[0] + dct[0]);
        pred[1] = JEBP__CLAMP_UBYTE(pred[1] + dct[1]);
        pred[2] = JEBP__CLAMP_UBYTE(pred[2] + dct[2]);
        pred[3] = JEBP__CLAMP_UBYTE(pred[3] + dct[3]);
        pred += stride;
        dct += JEBP__BLOCK_SIZE;
    }
#endif
}

static jebp_error_t jebp__read_macro_data(jebp__macro_header_t *hdr,
                                          jebp__macro_state_pair_t state,
                                          jebp__yuv_image_t *image,
                                          jebp__bec_reader_t *bec) {
    jebp_error_t err = JEBP_OK;
    JEBP__ALIGN_TYPE(jebp_short dct[JEBP__NB_BLOCK_COEFFS], JEBP__SIMD_ALIGN);
    JEBP__ALIGN_TYPE(jebp_short wht[JEBP__NB_BLOCK_COEFFS], JEBP__SIMD_ALIGN);
    jebp__block_type_t y_type = JEBP__BLOCK_Y0;
    jebp_ubyte *image_y =
        &image->y[(hdr->y * image->stride + hdr->x) * JEBP__Y_PIXEL_SIZE];

    // TODO: optimize 16x DCT inversion/add for non-B predictions
    if (hdr->y_pred != JEBP__VP8_PRED_B) {
        y_type = JEBP__BLOCK_Y1;
        jebp__y_preds[jebp__vp8_pred_type(hdr, hdr->y_pred)](image_y,
                                                             image->stride);

        jebp_int complex =
            JEBP__GET_Y2_NONZERO(state.top) + JEBP__GET_Y2_NONZERO(state.left);
        jebp_int nonzero =
            jebp__read_dct(hdr, wht, JEBP__BLOCK_Y2, complex, bec, &err);
        JEBP__SET_BIT(state.top->y2_flags, JEBP__Y_NONZERO, nonzero);
        JEBP__SET_BIT(state.left->y2_flags, JEBP__Y_NONZERO, nonzero);
        jebp__invert_wht(wht);
    }

    jebp_int macro_width = image->width / JEBP__Y_PIXEL_SIZE;
    for (jebp_int y = 0; y < JEBP__Y_SIZE; y += 1) {
        jebp_int row = y * image->stride;
        for (jebp_int x = 0; x < JEBP__Y_SIZE; x += 1) {
            jebp_int i = y * JEBP__Y_SIZE + x;
            jebp_ubyte *pred = &image_y[(row + x) * JEBP__BLOCK_SIZE];
            if (hdr->y_pred == JEBP__VP8_PRED_B) {
                jebp_ubyte *tr;
                jebp_ubyte tr_copy[JEBP__BLOCK_SIZE];
                if (x < JEBP__Y_SIZE - 1) {
                    // 0th, 1st and 2nd blocks can just reference the top-right
                    // portion
                    tr = &pred[JEBP__BLOCK_SIZE - image->stride];
                } else if (hdr->x < macro_width - 1) {
                    // Blocks on the right edge share TR with the top-right
                    // block
                    tr = &image_y[JEBP__Y_PIXEL_SIZE - image->stride];
                } else {
                    // Otherwise we duplicate the right-most pixel
                    memset(tr_copy,
                           image_y[JEBP__Y_PIXEL_SIZE - 1 - image->stride],
                           JEBP__BLOCK_SIZE);
                    tr = tr_copy;
                }
                jebp__b_preds[hdr->b_preds[i]](pred, image->stride, tr);
            } else {
                dct[0] = wht[i];
            }

            jebp_int complex = JEBP__GET_Y_NONZERO(state.top, x) +
                               JEBP__GET_Y_NONZERO(state.left, y);
            jebp_int nonzero =
                jebp__read_dct(hdr, dct, y_type, complex, bec, &err);
            JEBP__SET_BIT(state.top->y_flags[x], JEBP__Y_NONZERO, nonzero);
            JEBP__SET_BIT(state.left->y_flags[y], JEBP__Y_NONZERO, nonzero);
            jebp__invert_dct(dct);
            jebp__sum_pred_dct(pred, image->stride, dct);
        }
    }

    jebp__vp8_pred_t uv_pred =
        jebp__uv_preds[jebp__vp8_pred_type(hdr, hdr->uv_pred)];
    jebp_int uv_offset =
        (hdr->y * image->uv_stride + hdr->x) * JEBP__UV_PIXEL_SIZE;
    jebp_ubyte *image_u = &image->u[uv_offset];
    uv_pred(image_u, image->uv_stride);
    jebp_ubyte *image_v = &image->v[uv_offset];
    uv_pred(image_v, image->uv_stride);

    // TODO: optimize 4x DCT inversion/add for UV predictions
    for (jebp_int y = 0; y < JEBP__UV_SIZE; y += 1) {
        jebp_int row = y * image->uv_stride;
        for (jebp_int x = 0; x < JEBP__UV_SIZE; x += 1) {
            jebp_ubyte *pred = &image_u[(row + x) * JEBP__BLOCK_SIZE];
            jebp_int complex = JEBP__GET_U_NONZERO(state.top, x) +
                               JEBP__GET_U_NONZERO(state.left, y);
            jebp_int nonzero =
                jebp__read_dct(hdr, dct, JEBP__BLOCK_UV, complex, bec, &err);
            JEBP__SET_BIT(state.top->uv_flags[x], JEBP__U_NONZERO, nonzero);
            JEBP__SET_BIT(state.left->uv_flags[y], JEBP__U_NONZERO, nonzero);
            jebp__invert_dct(dct);
            jebp__sum_pred_dct(pred, image->uv_stride, dct);
        }
    }
    for (jebp_int y = 0; y < JEBP__UV_SIZE; y += 1) {
        jebp_int row = y * image->uv_stride;
        for (jebp_int x = 0; x < JEBP__UV_SIZE; x += 1) {
            jebp_ubyte *pred = &image_v[(row + x) * JEBP__BLOCK_SIZE];
            jebp_int complex = JEBP__GET_V_NONZERO(state.top, x) +
                               JEBP__GET_V_NONZERO(state.left, y);
            jebp_int nonzero =
                jebp__read_dct(hdr, dct, JEBP__BLOCK_UV, complex, bec, &err);
            JEBP__SET_BIT(state.top->uv_flags[x], JEBP__V_NONZERO, nonzero);
            JEBP__SET_BIT(state.left->uv_flags[y], JEBP__V_NONZERO, nonzero);
            jebp__invert_dct(dct);
            jebp__sum_pred_dct(pred, image->uv_stride, dct);
        }
    }
    return err;
}

/**
 * VP8 lossy codec
 */
#define JEBP__VP8_TAG 0x20385056
#define JEBP__VP8_MAGIC 0x2a019d

static jebp_error_t jebp__read_vp8_header(jebp__vp8_header_t *hdr,
                                          jebp_image_t *image,
                                          jebp__reader_t *reader,
                                          jebp__chunk_t *chunk) {
    jebp_error_t err = JEBP_OK;
    if (chunk->size < 10) {
        return JEBP_ERROR_INVDATA_HEADER;
    }
    chunk->size -= 10;
    jebp_int frame = jebp__read_uint24(reader, &err);
    if (jebp__read_uint24(reader, &err) != JEBP__VP8_MAGIC) {
        // check magic before everything else, despite being 3 bytes in
        return jebp__error(&err, JEBP_ERROR_INVDATA_HEADER);
    }
    if (frame & 0x1) {
        // frame must be a key-frame
        return jebp__error(&err, JEBP_ERROR_INVDATA);
    }
    if ((frame & 0xe) > 6) {
        // version must be 3 or less (shifted left by 1)
        return jebp__error(&err, JEBP_ERROR_NOSUP);
    }
    if (!(frame & 0x10)) {
        // frame must be displayed
        return jebp__error(&err, JEBP_ERROR_INVDATA);
    }
    hdr->bec_size = frame >> 5;
    if ((jebp_uint)hdr->bec_size > chunk->size) {
        return jebp__error(&err, JEBP_ERROR_INVDATA);
    }
    chunk->size -= hdr->bec_size;
    image->width = jebp__read_uint16(reader, &err);
    image->height = jebp__read_uint16(reader, &err);
    if ((image->width & 0xc000) || (image->height & 0xc000)) {
        // TODO: support frame upscaling
        return jebp__error(&err, JEBP_ERROR_NOSUP);
    }
    return err;
}

static jebp_error_t jebp__read_vp8_size(jebp_image_t *image,
                                        jebp__reader_t *reader,
                                        jebp__chunk_t *chunk) {
    jebp__vp8_header_t hdr;
    jebp__init_vp8_header(&hdr);
    return jebp__read_vp8_header(&hdr, image, reader, chunk);
}

static jebp_error_t jebp__read_vp8(jebp_image_t *image, jebp__reader_t *reader,
                                   jebp__chunk_t *chunk) {
    jebp_error_t err;
    jebp__vp8_header_t hdr;
    jebp__init_vp8_header(&hdr);
    if ((err = jebp__read_vp8_header(&hdr, image, reader, chunk)) != JEBP_OK) {
        return err;
    }

    jebp__reader_t map;
    jebp__bec_reader_t hdr_bec;
    if ((err = jebp__map_reader(reader, &map, hdr.bec_size)) != JEBP_OK) {
        return err;
    }
    if ((err = jebp__init_bec_reader(&hdr_bec, &map, hdr.bec_size)) !=
        JEBP_OK) {
        jebp__unmap_reader(&map);
        return err;
    }
    if ((err = jebp__read_bec_header(&hdr, &hdr_bec)) != JEBP_OK) {
        jebp__unmap_reader(&map);
        return err;
    }
    jebp__bec_reader_t data_bec;
    if ((err = jebp__init_bec_reader(&data_bec, reader, chunk->size)) !=
        JEBP_OK) {
        jebp__unmap_reader(&map);
        return err;
    }

    jebp_int macro_width = JEBP__CSHIFT(image->width, JEBP__Y_PIXEL_BITS);
    jebp_int macro_height = JEBP__CSHIFT(image->height, JEBP__Y_PIXEL_BITS);
    jebp__yuv_image_t yuv_image;
    yuv_image.width = macro_width * JEBP__Y_PIXEL_SIZE;
    yuv_image.height = macro_height * JEBP__Y_PIXEL_SIZE;
    if ((err = jebp__alloc_yuv_image(&yuv_image)) != JEBP_OK) {
        jebp__unmap_reader(&map);
        return err;
    }

    size_t top_size = macro_width * sizeof(jebp__macro_state_t);
    jebp__macro_state_t *top = JEBP_ALLOC(top_size);
    if (top == NULL) {
        jebp__free_yuv_image(&yuv_image);
        jebp__unmap_reader(&map);
        return JEBP_ERROR_NOMEM;
    }
    JEBP__CLEAR(top, top_size);
    jebp__macro_state_t left;
    jebp__macro_header_t macro_hdr;
    macro_hdr.vp8 = &hdr;

    for (jebp_int y = 0; y < macro_height; y += 1) {
        JEBP__CLEAR(&left, sizeof(jebp__macro_state_t));
        for (jebp_int x = 0; x < macro_width; x += 1) {
            macro_hdr.x = x;
            macro_hdr.y = y;
            jebp__macro_state_pair_t state = {.top = &top[x], .left = &left};
            if ((err = jebp__read_macro_header(&macro_hdr, state, &hdr_bec)) !=
                JEBP_OK) {
                break;
            }
            if ((err = jebp__read_macro_data(&macro_hdr, state, &yuv_image,
                                             &data_bec)) != JEBP_OK) {
                break;
            }
        }
        if (err != JEBP_OK) {
            break;
        }
    }

    JEBP_FREE(top);
    jebp__unmap_reader(&map);
    if (err != JEBP_OK) {
        jebp__free_yuv_image(&yuv_image);
        return err;
    }

    if ((err = jebp__alloc_image(image)) != JEBP_OK) {
        jebp__free_yuv_image(&yuv_image);
        return err;
    }
    err = jebp__convert_yuv_image(image, &yuv_image);
    jebp__free_yuv_image(&yuv_image);
    if (err != JEBP_OK) {
        jebp_free_image(image);
        return err;
    }
    return JEBP_OK;
}
#endif // JEBP_NO_VP8

/**
 * Bit reader
 */
#ifndef JEBP_NO_VP8L
typedef struct jebp__bit_reader_t {
    jebp__reader_t *reader;
    size_t nb_bytes;
    jebp_int nb_bits;
    jebp_uint bits;
} jebp__bit_reader_t;

static void jepb__init_bit_reader(jebp__bit_reader_t *bits,
                                  jebp__reader_t *reader, size_t size) {
    bits->reader = reader;
    bits->nb_bytes = size;
    bits->nb_bits = 0;
    bits->bits = 0;
}

// buffer/peek/skip should be used together to optimize bit-reading
static jebp_error_t jebp__buffer_bits(jebp__bit_reader_t *bits, jebp_int size) {
    jebp_error_t err = JEBP_OK;
    while (bits->nb_bits < size && bits->nb_bytes > 0) {
        bits->bits |= jebp__read_uint8(bits->reader, &err) << bits->nb_bits;
        bits->nb_bits += 8;
        bits->nb_bytes -= 1;
    }
    return err;
}

JEBP__INLINE jebp_int jepb__peek_bits(jebp__bit_reader_t *bits, jebp_int size) {
    return bits->bits & ((1 << size) - 1);
}

JEBP__INLINE jebp_error_t jebp__skip_bits(jebp__bit_reader_t *bits,
                                          jebp_int size) {
    if (size > bits->nb_bits) {
        return JEBP_ERROR_INVDATA;
    }
    bits->nb_bits -= size;
    bits->bits >>= size;
    return JEBP_OK;
}

static jebp_uint jebp__read_bits(jebp__bit_reader_t *bits, jebp_int size,
                                 jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
    if ((*err = jebp__buffer_bits(bits, size)) != JEBP_OK) {
        return 0;
    }
    jebp_uint value = jepb__peek_bits(bits, size);
    if ((*err = jebp__skip_bits(bits, size)) != JEBP_OK) {
        return 0;
    }
    return value;
}

/**
 * Huffman coding
 */
#define JEBP__MAX_HUFFMAN_LENGTH 15
#define JEBP__MAX_PRIMARY_LENGTH 8
#define JEBP__MAX_SECONDARY_LENGTH                                             \
    (JEBP__MAX_HUFFMAN_LENGTH - JEBP__MAX_PRIMARY_LENGTH)
#define JEBP__NB_PRIMARY_HUFFMANS (1 << JEBP__MAX_PRIMARY_LENGTH)
#define JEBP__NO_HUFFMAN_SYMBOL 0xffff

#define JEBP__NB_META_SYMBOLS 19
#define JEBP__NB_COLOR_SYMBOLS 256
#define JEBP__NB_LENGTH_SYMBOLS 24
#define JEBP__NB_DIST_SYMBOLS 40
#define JEBP__NB_MAIN_SYMBOLS (JEBP__NB_COLOR_SYMBOLS + JEBP__NB_LENGTH_SYMBOLS)

// The huffman decoding is done in one or two steps, both using a lookup table.
// These tables are called the "primary" table and "secondary" tables. First
// 8-bits are peeked from the stream to index the primary table. If the symbol
// is in this table (indicated by length <= 8) then the symbol from that is used
// and the length is used to skip that many bits. Codes which are smaller than
// 8-bits are represented by filling the table such that any index with a prefix
// of the given code will have the same entry. If the symbol requires more bits
// (indiciated by length > 8) then the symbol is used as an offset pointing to
// the secondary table which has an index size of (length - 8) bits.
typedef struct jebp__huffman_t {
    // <= 8: length is the number of bits actually used, and symbol is the
    //       decoded symbol or `JEBP__NO_HUFFMAN_SYMBOL` for an invalid code.
    // >  8: length is the maximum number of bits for any code with this prefix,
    //       and symbol is the offset in the array to the secondary table.
    jebp_short length;
    jebp_ushort symbol;
} jebp__huffman_t;

typedef struct jebp__huffman_group_t {
    jebp__huffman_t *main;
    jebp__huffman_t *red;
    jebp__huffman_t *blue;
    jebp__huffman_t *alpha;
    jebp__huffman_t *dist;
} jebp__huffman_group_t;

static const jebp_byte jebp__meta_length_order[JEBP__NB_META_SYMBOLS];

// Reverse increment, returns truthy on overflow
JEBP__INLINE jebp_int jebp__increment_code(jebp_int *code, jebp_int length) {
    jebp_int inc = 1 << (length - 1);
    while (*code & inc) {
        inc >>= 1;
    }
    if (inc == 0) {
        return 1;
    }
    *code = (*code & (inc - 1)) + inc;
    return 0;
}

// This function is a bit confusing so I have attempted to document it well
static jebp_error_t jebp__alloc_huffman(jebp__huffman_t **huffmans,
                                        jebp_int nb_lengths,
                                        const jebp_byte *lengths) {
    // Stack allocate the primary table and set it all to invalid values
    jebp__huffman_t primary[JEBP__NB_PRIMARY_HUFFMANS];
    for (jebp_int i = 0; i < JEBP__NB_PRIMARY_HUFFMANS; i += 1) {
        primary[i].symbol = JEBP__NO_HUFFMAN_SYMBOL;
    }

    // Fill in the 8-bit codes in the primary table
    jebp_int len = 1;
    jebp_int code = 0;
    jebp_int overflow = 0;
    jebp_ushort symbol = JEBP__NO_HUFFMAN_SYMBOL;
    jebp_int nb_symbols = 0;
    for (; len <= JEBP__MAX_PRIMARY_LENGTH; len += 1) {
        for (jebp_int i = 0; i < nb_lengths; i += 1) {
            if (lengths[i] != len) {
                continue;
            }
            if (overflow) {
                // Fail now if the last increment overflowed
                return JEBP_ERROR_INVDATA;
            }
            for (jebp_int c = code; c < JEBP__NB_PRIMARY_HUFFMANS;
                 c += 1 << len) {
                primary[c].length = len;
                primary[c].symbol = i;
            }
            overflow = jebp__increment_code(&code, len);
            symbol = i;
            nb_symbols += 1;
        }
    }

    // Fill in the secondary table lengths in the primary table
    jebp_int secondary_code = code;
    for (; len <= JEBP__MAX_HUFFMAN_LENGTH; len += 1) {
        for (jebp_int i = 0; i < nb_lengths; i += 1) {
            if (lengths[i] != len) {
                continue;
            }
            if (overflow) {
                return JEBP_ERROR_INVDATA;
            }
            jebp_int prefix = code & (JEBP__NB_PRIMARY_HUFFMANS - 1);
            primary[prefix].length = len;
            overflow = jebp__increment_code(&code, len);
            symbol = i;
            nb_symbols += 1;
        }
    }

    // Calculate the total no. of huffman entries and fill in the secondary
    // table offsets
    jebp_int nb_huffmans = JEBP__NB_PRIMARY_HUFFMANS;
    for (jebp_int i = 0; i < JEBP__NB_PRIMARY_HUFFMANS; i += 1) {
        if (nb_symbols <= 1) {
            // Special case: if there is only one symbol, use this iteration to
            //               instead fill the primary table with 0-length
            //               entries
            primary[i].length = 0;
            primary[i].symbol = symbol;
            continue;
        }
        jebp_int suffix_length = primary[i].length - JEBP__MAX_PRIMARY_LENGTH;
        if (suffix_length > 0) {
            primary[i].symbol = nb_huffmans;
            nb_huffmans += 1 << suffix_length;
        }
    }

    // Allocate, copy over the primary table, and assign the rest to invalid
    // values
    *huffmans = JEBP_ALLOC(nb_huffmans * sizeof(jebp__huffman_t));
    if (*huffmans == NULL) {
        return JEBP_ERROR_NOMEM;
    }
    memcpy(*huffmans, primary, sizeof(primary));
    if (nb_huffmans == JEBP__NB_PRIMARY_HUFFMANS) {
        // Special case: we can stop here if we don't have to fill any secondary
        //               tables
        return JEBP_OK;
    }
    for (jebp_int i = JEBP__NB_PRIMARY_HUFFMANS; i < nb_huffmans; i += 1) {
        (*huffmans)[i].symbol = JEBP__NO_HUFFMAN_SYMBOL;
    }

    // Fill in the secondary tables
    len = JEBP__MAX_PRIMARY_LENGTH + 1;
    code = secondary_code;
    for (; len <= JEBP__MAX_HUFFMAN_LENGTH; len += 1) {
        for (jebp_int i = 0; i < nb_lengths; i += 1) {
            if (lengths[i] != len) {
                continue;
            }
            jebp_int prefix = code & (JEBP__NB_PRIMARY_HUFFMANS - 1);
            jebp_int nb_secondary_huffmans = 1 << primary[prefix].length;
            jebp__huffman_t *secondary = *huffmans + primary[prefix].symbol;
            for (jebp_int c = code; c < nb_secondary_huffmans; c += 1 << len) {
                secondary[c >> JEBP__MAX_PRIMARY_LENGTH].length = len;
                secondary[c >> JEBP__MAX_PRIMARY_LENGTH].symbol = i;
            }
            jebp__increment_code(&code, len);
        }
    }
    return JEBP_OK;
}

static jebp_int jebp__read_symbol(jebp__huffman_t *huffmans,
                                  jebp__bit_reader_t *bits, jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 0;
    }
    if ((*err = jebp__buffer_bits(bits, JEBP__MAX_HUFFMAN_LENGTH)) != JEBP_OK) {
        return 0;
    }
    jebp_int code = jepb__peek_bits(bits, JEBP__MAX_PRIMARY_LENGTH);
    if (huffmans[code].symbol == JEBP__NO_HUFFMAN_SYMBOL) {
        *err = JEBP_ERROR_INVDATA;
        return 0;
    }
    jebp_int length = huffmans[code].length;
    jebp_int skip = JEBP__MIN(length, JEBP__MAX_PRIMARY_LENGTH);
    if ((*err = jebp__skip_bits(bits, skip)) != JEBP_OK) {
        return 0;
    }
    if (skip == length) {
        return huffmans[code].symbol;
    }

    huffmans += huffmans[code].symbol;
    code = jepb__peek_bits(bits, length - skip);
    if (huffmans[code].symbol == JEBP__NO_HUFFMAN_SYMBOL) {
        *err = JEBP_ERROR_INVDATA;
        return 0;
    }
    if ((*err = jebp__skip_bits(bits, huffmans[code].length - skip)) !=
        JEBP_OK) {
        return 0;
    }
    return huffmans[code].symbol;
}

static jebp_error_t jebp__read_huffman(jebp__huffman_t **huffmans,
                                       jebp__bit_reader_t *bits,
                                       jebp_int nb_lengths,
                                       jebp_byte *lengths) {
    // This part of the spec is INCREDIBLY wrong and partly missing
    jebp_error_t err = JEBP_OK;
    JEBP__CLEAR(lengths, nb_lengths);

    if (jebp__read_bits(bits, 1, &err)) {
        // simple length storage with only 1 (first) or 2 (second) symbols, both
        // with a length of 1
        jebp_int has_second = jebp__read_bits(bits, 1, &err);
        jebp_int first_bits = jebp__read_bits(bits, 1, &err) ? 8 : 1;
        jebp_int first = jebp__read_bits(bits, first_bits, &err);
        if (first >= nb_lengths) {
            return jebp__error(&err, JEBP_ERROR_INVDATA);
        }
        lengths[first] = 1;
        if (has_second) {
            jebp_int second = jebp__read_bits(bits, 8, &err);
            if (second >= nb_lengths) {
                return jebp__error(&err, JEBP_ERROR_INVDATA);
            }
            lengths[second] = 1;
        }

    } else {
        jebp_byte meta_lengths[JEBP__NB_META_SYMBOLS] = {0};
        jebp_int nb_meta_lengths = jebp__read_bits(bits, 4, &err) + 4;
        for (jebp_int i = 0; i < nb_meta_lengths; i += 1) {
            meta_lengths[jebp__meta_length_order[i]] =
                jebp__read_bits(bits, 3, &err);
        }
        if (err != JEBP_OK) {
            return err;
        }
        jebp__huffman_t *meta_huffmans;
        if ((err = jebp__alloc_huffman(&meta_huffmans, JEBP__NB_META_SYMBOLS,
                                       meta_lengths)) != JEBP_OK) {
            return err;
        }

        jebp_int nb_meta_symbols = nb_lengths;
        if (jebp__read_bits(bits, 1, &err)) {
            // limit codes
            jebp_int symbols_bits = jebp__read_bits(bits, 3, &err) * 2 + 2;
            nb_meta_symbols = jebp__read_bits(bits, symbols_bits, &err) + 2;
        }

        jebp_int prev_length = 8;
        for (jebp_int i = 0; i < nb_lengths && nb_meta_symbols > 0;
             nb_meta_symbols -= 1) {
            jebp_int symbol = jebp__read_symbol(meta_huffmans, bits, &err);
            jebp_int length;
            jebp_int repeat;
            switch (symbol) {
            case 16:
                length = prev_length;
                repeat = jebp__read_bits(bits, 2, &err) + 3;
                break;
            case 17:
                length = 0;
                repeat = jebp__read_bits(bits, 3, &err) + 3;
                break;
            case 18:
                length = 0;
                repeat = jebp__read_bits(bits, 7, &err) + 11;
                break;
            default:
                prev_length = symbol;
                /* fallthrough */
            case 0:
                // We don't ever repeat 0 values.
                lengths[i++] = symbol;
                continue;
            }
            if (i + repeat > nb_lengths) {
                jebp__error(&err, JEBP_ERROR_INVDATA);
                break;
            }
            for (jebp_int j = 0; j < repeat; j += 1) {
                lengths[i++] = length;
            }
        }
        JEBP_FREE(meta_huffmans);
    }

    if (err != JEBP_OK) {
        return err;
    }
    return jebp__alloc_huffman(huffmans, nb_lengths, lengths);
}

static jebp_error_t jebp__read_huffman_group(jebp__huffman_group_t *group,
                                             jebp__bit_reader_t *bits,
                                             jebp_int nb_main_symbols,
                                             jebp_byte *lengths) {
    jebp_error_t err;
    if ((err = jebp__read_huffman(&group->main, bits, nb_main_symbols,
                                  lengths)) != JEBP_OK) {
        return err;
    }
    if ((err = jebp__read_huffman(&group->red, bits, JEBP__NB_COLOR_SYMBOLS,
                                  lengths)) != JEBP_OK) {
        return err;
    }
    if ((err = jebp__read_huffman(&group->blue, bits, JEBP__NB_COLOR_SYMBOLS,
                                  lengths)) != JEBP_OK) {
        return err;
    }
    if ((err = jebp__read_huffman(&group->alpha, bits, JEBP__NB_COLOR_SYMBOLS,
                                  lengths)) != JEBP_OK) {
        return err;
    }
    if ((err = jebp__read_huffman(&group->dist, bits, JEBP__NB_DIST_SYMBOLS,
                                  lengths)) != JEBP_OK) {
        return err;
    }
    return JEBP_OK;
}

static void jebp__free_huffman_group(jebp__huffman_group_t *group) {
    JEBP_FREE(group->main);
    JEBP_FREE(group->red);
    JEBP_FREE(group->blue);
    JEBP_FREE(group->alpha);
    JEBP_FREE(group->dist);
}

/**
 * Color cache
 */
typedef struct jebp__colcache_t {
    jebp_int bits;
    jebp_color_t *colors;
} jebp__colcache_t;

static jebp_error_t jebp__read_colcache(jebp__colcache_t *colcache,
                                        jebp__bit_reader_t *bits) {
    jebp_error_t err = JEBP_OK;
    if (!jebp__read_bits(bits, 1, &err)) {
        // no color cache
        colcache->bits = 0;
        return err;
    }
    colcache->bits = jebp__read_bits(bits, 4, &err);
    if (err != JEBP_OK || colcache->bits < 1 || colcache->bits > 11) {
        return jebp__error(&err, JEBP_ERROR_INVDATA);
    }

    size_t colcache_size = ((size_t)1 << colcache->bits) * sizeof(jebp_color_t);
    colcache->colors = JEBP_ALLOC(colcache_size);
    if (colcache->colors == NULL) {
        return JEBP_ERROR_NOMEM;
    }
    JEBP__CLEAR(colcache->colors, colcache_size);
    return JEBP_OK;
}

static void jebp__free_colcache(jebp__colcache_t *colcache) {
    if (colcache->bits > 0) {
        JEBP_FREE(colcache->colors);
    }
}

static void jebp__colcache_insert(jebp__colcache_t *colcache,
                                  jebp_color_t *color) {
    if (colcache->bits == 0) {
        return;
    }
#if defined(JEBP__LITTLE_ENDIAN) && defined(JEBP__SWAP32)
    jebp_uint hash = *(jebp_uint *)color; // ABGR due to little-endian
    hash = JEBP__SWAP32(hash);            // RGBA
    hash = (hash >> 8) | (hash << 24);    // ARGB
#else
    jebp_uint hash = ((jebp_uint)color->a << 24) | ((jebp_uint)color->r << 16) |
                     ((jebp_uint)color->g << 8) | (jebp_uint)color->b;
#endif
    hash = (0x1e35a7bd * hash) >> (32 - colcache->bits);
    colcache->colors[hash] = *color;
}

/**
 * VP8L image
 */
#define JEBP__NB_VP8L_OFFSETS 120

typedef struct jebp__subimage_t {
    jebp_int width;
    jebp_int height;
    jebp_color_t *pixels;
    jebp_int block_bits;
} jebp__subimage_t;

static const jebp_byte jebp__vp8l_offsets[JEBP__NB_VP8L_OFFSETS][2];

JEBP__INLINE jebp_int jebp__read_vp8l_extrabits(jebp__bit_reader_t *bits,
                                                jebp_int symbol,
                                                jebp_error_t *err) {
    if (*err != JEBP_OK) {
        return 1;
    }
    if (symbol < 4) {
        return symbol + 1;
    }
    jebp_int extrabits = symbol / 2 - 1;
    symbol = ((symbol % 2 + 2) << extrabits) + 1;
    return symbol + jebp__read_bits(bits, extrabits, err);
}

static jebp_error_t jebp__read_vp8l_image(jebp_image_t *image,
                                          jebp__bit_reader_t *bits,
                                          jebp__colcache_t *colcache,
                                          jebp__subimage_t *huffman_image) {
    jebp_error_t err;
    jebp_int nb_groups = 1;
    jebp__huffman_group_t *groups = &(jebp__huffman_group_t){0};
    if (huffman_image != NULL) {
        for (jebp_int i = 0; i < huffman_image->width * huffman_image->height;
             i += 1) {
            jebp_color_t *huffman = &huffman_image->pixels[i];
            if (huffman->r != 0) {
                // Currently only 256 huffman groups are supported
                return JEBP_ERROR_NOSUP;
            }
            nb_groups = JEBP__MAX(nb_groups, huffman->g + 1);
            huffman += 1;
        }
        if (nb_groups > 1) {
            groups = JEBP_ALLOC(nb_groups * sizeof(jebp__huffman_group_t));
            if (groups == NULL) {
                return JEBP_ERROR_NOMEM;
            }
        }
    }

    jebp_int nb_main_symbols = JEBP__NB_MAIN_SYMBOLS;
    if (colcache->bits > 0) {
        nb_main_symbols += 1 << colcache->bits;
    }
    jebp_byte *lengths = JEBP_ALLOC(nb_main_symbols);
    if (lengths == NULL) {
        err = JEBP_ERROR_NOMEM;
        goto free_groups;
    }
    jebp_int nb_read_groups = 0;
    for (; nb_read_groups < nb_groups; nb_read_groups += 1) {
        if ((err = jebp__read_huffman_group(&groups[nb_read_groups], bits,
                                            nb_main_symbols, lengths)) !=
            JEBP_OK) {
            break;
        }
    }
    JEBP_FREE(lengths);
    if (err != JEBP_OK) {
        goto free_read_groups;
    }
    if ((err = jebp__alloc_image(image)) != JEBP_OK) {
        goto free_read_groups;
    }

    jebp_color_t *pixel = image->pixels;
    jebp_color_t *end = pixel + image->width * image->height;
    jebp_int x = 0;
    for (jebp_int y = 0; y < image->height;) {
        jebp_color_t *huffman_row = NULL;
        if (huffman_image != NULL) {
            huffman_row =
                &huffman_image->pixels[(y >> huffman_image->block_bits) *
                                       huffman_image->width];
        }
        do {
            jebp__huffman_group_t *group;
            if (huffman_image == NULL) {
                group = groups;
            } else {
                jebp_color_t *huffman =
                    &huffman_row[x >> huffman_image->block_bits];
                group = &groups[huffman->g];
            }

            jebp_int main = jebp__read_symbol(group->main, bits, &err);
            if (main < JEBP__NB_COLOR_SYMBOLS) {
                pixel->g = main;
                pixel->r = jebp__read_symbol(group->red, bits, &err);
                pixel->b = jebp__read_symbol(group->blue, bits, &err);
                pixel->a = jebp__read_symbol(group->alpha, bits, &err);
                jebp__colcache_insert(colcache, pixel++);
                x += 1;
            } else if (main >= JEBP__NB_MAIN_SYMBOLS) {
                *(pixel++) = colcache->colors[main - JEBP__NB_MAIN_SYMBOLS];
                x += 1;
            } else {
                jebp_int length = jebp__read_vp8l_extrabits(
                    bits, main - JEBP__NB_COLOR_SYMBOLS, &err);
                jebp_int dist = jebp__read_symbol(group->dist, bits, &err);
                dist = jebp__read_vp8l_extrabits(bits, dist, &err);
                if (dist > JEBP__NB_VP8L_OFFSETS) {
                    dist -= JEBP__NB_VP8L_OFFSETS;
                } else {
                    const jebp_byte *offset = jebp__vp8l_offsets[dist - 1];
                    dist = offset[1] * image->width + offset[0];
                    dist = JEBP__MAX(dist, 1);
                }
                jebp_color_t *repeat = pixel - dist;
                if (repeat < image->pixels || pixel + length > end) {
                    jebp__error(&err, JEBP_ERROR_INVDATA);
                    break;
                }
                for (jebp_int i = 0; i < length; i += 1) {
                    jebp__colcache_insert(colcache, repeat);
                    *(pixel++) = *(repeat++);
                }
                x += length;
            }
        } while (x < image->width);
        y += x / image->width;
        x %= image->width;
    }

    if (err != JEBP_OK) {
        jebp_free_image(image);
    }
free_read_groups:
    for (nb_read_groups -= 1; nb_read_groups >= 0; nb_read_groups -= 1) {
        jebp__free_huffman_group(&groups[nb_read_groups]);
    }
free_groups:
    if (nb_groups > 1) {
        JEBP_FREE(groups);
    }
    return err;
}

static jebp_error_t jebp__read_subimage(jebp__subimage_t *subimage,
                                        jebp__bit_reader_t *bits,
                                        jebp_image_t *image) {
    jebp_error_t err = JEBP_OK;
    subimage->block_bits = jebp__read_bits(bits, 3, &err) + 2;
    subimage->width = JEBP__CSHIFT(image->width, subimage->block_bits);
    subimage->height = JEBP__CSHIFT(image->height, subimage->block_bits);
    if (err != JEBP_OK) {
        return err;
    }
    jebp__colcache_t colcache;
    if ((err = jebp__read_colcache(&colcache, bits)) != JEBP_OK) {
        return err;
    }
    err =
        jebp__read_vp8l_image((jebp_image_t *)subimage, bits, &colcache, NULL);
    jebp__free_colcache(&colcache);
    return err;
}

/**
 * VP8L predictions
 */
#define JEBP__NB_VP8L_PRED_TYPES 14

// I don't like the way it formats this
// clang-format off
#define JEBP__UNROLL4(var, body) \
    { var = 0; body } \
    { var = 1; body } \
    { var = 2; body } \
    { var = 3; body }
// clang-format on

typedef void (*jebp__vp8l_pred_t)(jebp_color_t *pixel, jebp_color_t *top,
                                  jebp_int width);

#ifdef JEBP__SIMD_SSE2
typedef struct jebp__m128x4i {
    __m128i v[4];
} jebp__m128x4i;

JEBP__INLINE __m128i jebp__sse_move_px1(__m128i v_dst, __m128i v_src) {
    __m128 v_dstf = _mm_castsi128_ps(v_dst);
    __m128 v_srcf = _mm_castsi128_ps(v_src);
    __m128 v_movf = _mm_move_ss(v_dstf, v_srcf);
    return _mm_castps_si128(v_movf);
}

JEBP__INLINE __m128i jebp__sse_avg_u8x16(__m128i v1, __m128i v2) {
    __m128i v_one = _mm_set1_epi8(1);
    __m128i v_avg = _mm_avg_epu8(v1, v2);
    // SSE2 `avg` rounds up, we have to check if a round-up occured (one of the
    // low bits was set but the other wasn't) and subtract 1 if so
    __m128i v_err = _mm_xor_si128(v1, v2);
    v_err = _mm_and_si128(v_err, v_one);
    return _mm_sub_epi8(v_avg, v_err);
}

JEBP__INLINE __m128i jebp__sse_avg2_u8x16(__m128i v1, __m128i v2, __m128i v3) {
    __m128i v_one = _mm_set1_epi8(1);
    // We can further optimise two avg calls but noting that the error will
    // propogate
    __m128i v_avg1 = _mm_avg_epu8(v1, v2);
    __m128i v_err1 = _mm_xor_si128(v1, v2);
    __m128i v_avg2 = _mm_avg_epu8(v_avg1, v3);
    __m128i v_err2 = _mm_xor_si128(v_avg1, v3);
    v_err2 = _mm_or_si128(v_err1, v_err2);
    v_err2 = _mm_and_si128(v_err2, v_one);
    return _mm_sub_epi8(v_avg2, v_err2);
}

JEBP__INLINE __m128i jebp__sse_flatten_px4(jebp__m128x4i v_pixel4) {
    __m128i v_pixello = jebp__sse_move_px1(v_pixel4.v[1], v_pixel4.v[0]);
    __m128i v_pixel3 = _mm_bsrli_si128(v_pixel4.v[3], 4);
    __m128i v_pixelhi = _mm_unpackhi_epi32(v_pixel4.v[2], v_pixel3);
    return _mm_unpacklo_epi64(v_pixello, v_pixelhi);
}

// Bit-select and accumulate, used by prediction filters 11-13
JEBP__INLINE __m128i jebp__sse_bsela_u8x16(__m128i v_acc, __m128i v_mask,
                                           __m128i v1, __m128i v0) {
    // This is faster than using and/andnot/or since SSE only supports two
    // operands so prefers chaining outputs
    __m128i v_sel = _mm_xor_si128(v0, v1);
    v_sel = _mm_and_si128(v_sel, v_mask);
    v_sel = _mm_xor_si128(v_sel, v0);
    return _mm_add_epi8(v_acc, v_sel);
}
#endif // JEBP__SIMD_SSE2

#ifdef JEBP__SIMD_NEON
JEBP__INLINE uint8x16_t jebp__neon_load_px1(jebp_color_t *pixel) {
    uint8x16_t v_pixel = vreinterpretq_u8_u32(vld1q_dup_u32((uint32_t *)pixel));
#ifndef JEBP__LITTLE_ENDIAN
    v_pixel = vrev32q_u8(v_pixel);
#endif // JEBP__LITTLE_ENDIAN
    return v_pixel;
}

JEBP__INLINE uint8x16_t jebp__neon_flatten_px4(uint8x16x4_t v_pixel4) {
#ifdef JEBP__SIMD_NEON64
    uint8x16_t v_table = vcombine_u8(vcreate_u8(0x1716151403020100),
                                     vcreate_u8(0x3f3e3d3c2b2a2928));
    return vqtbl4q_u8(v_pixel4, v_table);
#else  // JEBP__SIMD_NEON64
    uint8x16_t v_mask = vreinterpretq_u8_u64(vdupq_n_u64(0xffffffff));
    uint8x16_t v_even = vcombine_u8(vget_low_u8(v_pixel4.val[0]),
                                    vget_high_u8(v_pixel4.val[2]));
    uint8x16_t v_odd = vcombine_u8(vget_low_u8(v_pixel4.val[1]),
                                   vget_high_u8(v_pixel4.val[3]));
    return vbslq_u8(v_mask, v_even, v_odd);
#endif // JEBP__SIMD_NEON64
}

JEBP__INLINE uint32x4_t jebp__neon_sad_px4(uint8x16_t v_pix1,
                                           uint8x16_t v_pix2) {
    uint8x16_t v_diff8 = vabdq_u8(v_pix1, v_pix2);
    uint16x8_t v_diff16 = vpaddlq_u8(v_diff8);
    return vpaddlq_u16(v_diff16);
}
#endif // JEBP__SIMD_NEON

JEBP__INLINE void jebp__vp8l_pred_black(jebp_color_t *pixel, jebp_int width) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    __m128i v_black = _mm_set1_epi32((int)0xff000000);
    for (; x + 4 <= width; x += 4) {
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        v_pixel = _mm_add_epi8(v_pixel, v_black);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
    }
#elif defined(JEBP__SIMD_NEON)
    uint8x8_t v_black = vdup_n_u8(0xff);
    for (; x + 8 <= width; x += 8) {
        uint8x8x4_t v_pixel = vld4_u8((uint8_t *)&pixel[x]);
        v_pixel.val[3] = vadd_u8(v_pixel.val[3], v_black);
        vst4_u8((uint8_t *)&pixel[x], v_pixel);
    }
#endif
    for (; x < width; x += 1) {
        pixel[x].a += 0xff;
    }
}

static void jebp__vp8l_pred0(jebp_color_t *pixel, jebp_color_t *top,
                             jebp_int width) {
    (void)top;
    jebp__vp8l_pred_black(pixel, width);
}

JEBP__INLINE void jebp__vp8l_pred_left(jebp_color_t *pixel, jebp_int width) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    __m128i v_left;
    if (width >= 4) {
        v_left = _mm_cvtsi32_si128(*(int *)&pixel[-1]);
    }
    for (; x + 4 <= width; x += 4) {
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        v_pixel = _mm_add_epi8(v_pixel, v_left);
        v_left = _mm_bslli_si128(v_pixel, 4);
        v_pixel = _mm_add_epi8(v_pixel, v_left);
        v_left = _mm_bslli_si128(v_pixel, 8);
        v_pixel = _mm_add_epi8(v_pixel, v_left);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
        v_left = _mm_bsrli_si128(v_pixel, 12);
    }
#elif defined(JEBP__SIMD_NEON)
    uint8x16_t v_zero = vdupq_n_u8(0);
    uint8x16_t v_left;
    if (width >= 4) {
        v_left = jebp__neon_load_px1(&pixel[-1]);
        v_left = vextq_u8(v_left, v_zero, 12);
    }
    for (; x + 4 <= width; x += 4) {
        uint8x16_t v_pixel = vld1q_u8((uint8_t *)&pixel[x]);
        v_pixel = vaddq_u8(v_pixel, v_left);
        v_left = vextq_u8(v_zero, v_pixel, 12);
        v_pixel = vaddq_u8(v_pixel, v_left);
        v_left = vextq_u8(v_zero, v_pixel, 8);
        v_pixel = vaddq_u8(v_pixel, v_left);
        vst1q_u8((uint8_t *)&pixel[x], v_pixel);
        v_left = vextq_u8(v_pixel, v_zero, 12);
    }
#endif
    for (; x < width; x += 1) {
        pixel[x].r += pixel[x - 1].r;
        pixel[x].g += pixel[x - 1].g;
        pixel[x].b += pixel[x - 1].b;
        pixel[x].a += pixel[x - 1].a;
    }
}

static void jebp__vp8l_pred1(jebp_color_t *pixel, jebp_color_t *top,
                             jebp_int width) {
    (void)top;
    jebp__vp8l_pred_left(pixel, width);
}

JEBP__INLINE void jebp__vp8l_pred_top(jebp_color_t *pixel, jebp_color_t *top,
                                      jebp_int width) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    for (; x + 4 <= width; x += 4) {
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        __m128i v_top = _mm_loadu_si128((__m128i *)&top[x]);
        v_pixel = _mm_add_epi8(v_pixel, v_top);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
    }
#elif defined(JEBP__SIMD_NEON)
    for (; x + 4 <= width; x += 4) {
        uint8x16_t v_pixel = vld1q_u8((uint8_t *)&pixel[x]);
        uint8x16_t v_top = vld1q_u8((uint8_t *)&top[x]);
        v_pixel = vaddq_u8(v_pixel, v_top);
        vst1q_u8((uint8_t *)&pixel[x], v_pixel);
    }
#endif
    for (; x < width; x += 1) {
        pixel[x].r += top[x].r;
        pixel[x].g += top[x].g;
        pixel[x].b += top[x].b;
        pixel[x].a += top[x].a;
    }
}

static void jebp__vp8l_pred2(jebp_color_t *pixel, jebp_color_t *top,
                             jebp_int width) {
    jebp__vp8l_pred_top(pixel, top, width);
}

static void jebp__vp8l_pred3(jebp_color_t *pixel, jebp_color_t *top,
                             jebp_int width) {
    jebp__vp8l_pred_top(pixel, &top[1], width);
}

static void jebp__vp8l_pred4(jebp_color_t *pixel, jebp_color_t *top,
                             jebp_int width) {
    jebp__vp8l_pred_top(pixel, &top[-1], width);
}

static void jebp__vp8l_pred5(jebp_color_t *pixel, jebp_color_t *top,
                             jebp_int width) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    __m128i v_left;
    __m128i v_top;
    if (width >= 4) {
        v_left = _mm_cvtsi32_si128(*(int *)&pixel[-1]);
        v_top = _mm_loadu_si128((__m128i *)top);
    }
    for (; x + 4 <= width; x += 4) {
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        __m128i v_next = _mm_loadu_si128((__m128i *)&top[x + 4]);
        __m128i v_tr = jebp__sse_move_px1(v_top, v_next);
        v_tr = _mm_shuffle_epi32(v_tr, _MM_SHUFFLE(0, 3, 2, 1));
        jebp__m128x4i v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            __m128i v_avg = jebp__sse_avg2_u8x16(v_left, v_tr, v_top);
            v_pixel4.v[i] = _mm_add_epi8(v_pixel, v_avg);
            v_left = _mm_shuffle_epi32(v_pixel4.v[i], _MM_SHUFFLE(2, 1, 0, 3));
        })
        v_pixel = jebp__sse_flatten_px4(v_pixel4);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
        v_top = v_next;
    }
#elif defined(JEBP__SIMD_NEON)
    uint8x16_t v_left;
    uint8x16_t v_top;
    if (width >= 4) {
        v_left = jebp__neon_load_px1(&pixel[-1]);
        v_top = vld1q_u8((uint8_t *)top);
    }
    for (; x + 4 <= width; x += 4) {
        uint8x16_t v_pixel = vld1q_u8((uint8_t *)&pixel[x]);
        uint8x16_t v_next = vld1q_u8((uint8_t *)&top[x + 4]);
        uint8x16_t v_tr = vextq_u8(v_top, v_next, 4);
        uint8x16x4_t v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            uint8x16_t v_avg = vhaddq_u8(v_left, v_tr);
            v_avg = vhaddq_u8(v_avg, v_top);
            v_pixel4.val[i] = vaddq_u8(v_pixel, v_avg);
            v_left = vextq_u8(v_pixel4.val[i], v_pixel4.val[i], 12);
        })
        v_pixel = jebp__neon_flatten_px4(v_pixel4);
        vst1q_u8((uint8_t *)&pixel[x], v_pixel);
        v_top = v_next;
    }
#endif
    for (; x < width; x += 1) {
        pixel[x].r +=
            JEBP__FAVG(JEBP__FAVG(pixel[x - 1].r, top[x + 1].r), top[x].r);
        pixel[x].g +=
            JEBP__FAVG(JEBP__FAVG(pixel[x - 1].g, top[x + 1].g), top[x].g);
        pixel[x].b +=
            JEBP__FAVG(JEBP__FAVG(pixel[x - 1].b, top[x + 1].b), top[x].b);
        pixel[x].a +=
            JEBP__FAVG(JEBP__FAVG(pixel[x - 1].a, top[x + 1].a), top[x].a);
    }
}

JEBP__INLINE void jebp__vp8l_pred_avgtl(jebp_color_t *pixel, jebp_color_t *top,
                                        jebp_int width) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    __m128i v_left;
    if (width >= 4) {
        v_left = _mm_cvtsi32_si128(*(int *)&pixel[-1]);
    }
    for (; x + 4 <= width; x += 4) {
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        __m128i v_top = _mm_loadu_si128((__m128i *)&top[x]);
        jebp__m128x4i v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            __m128i v_avg = jebp__sse_avg_u8x16(v_left, v_top);
            v_pixel4.v[i] = _mm_add_epi8(v_pixel, v_avg);
            v_left = _mm_shuffle_epi32(v_pixel4.v[i], _MM_SHUFFLE(2, 1, 0, 3));
        })
        v_pixel = jebp__sse_flatten_px4(v_pixel4);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
    }
#elif defined(JEBP__SIMD_NEON)
    uint8x16_t v_left;
    if (width >= 4) {
        v_left = jebp__neon_load_px1(&pixel[-1]);
    }
    for (; x + 4 <= width; x += 4) {
        uint8x16_t v_pixel = vld1q_u8((uint8_t *)&pixel[x]);
        uint8x16_t v_top = vld1q_u8((uint8_t *)&top[x]);
        uint8x16x4_t v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            uint8x16_t v_avg = vhaddq_u8(v_left, v_top);
            v_pixel4.val[i] = vaddq_u8(v_pixel, v_avg);
            v_left = vextq_u8(v_pixel4.val[i], v_pixel4.val[i], 12);
        })
        v_pixel = jebp__neon_flatten_px4(v_pixel4);
        vst1q_u8((uint8_t *)&pixel[x], v_pixel);
    }
#endif
    for (; x < width; x += 1) {
        pixel[x].r += JEBP__FAVG(pixel[x - 1].r, top[x].r);
        pixel[x].g += JEBP__FAVG(pixel[x - 1].g, top[x].g);
        pixel[x].b += JEBP__FAVG(pixel[x - 1].b, top[x].b);
        pixel[x].a += JEBP__FAVG(pixel[x - 1].a, top[x].a);
    }
}

static void jebp__vp8l_pred6(jebp_color_t *pixel, jebp_color_t *top,
                             jebp_int width) {
    jebp__vp8l_pred_avgtl(pixel, &top[-1], width);
}

static void jebp__vp8l_pred7(jebp_color_t *pixel, jebp_color_t *top,
                             jebp_int width) {
    jebp__vp8l_pred_avgtl(pixel, top, width);
}

JEBP__INLINE void jebp__vp8l_pred_avgtr(jebp_color_t *pixel, jebp_color_t *top,
                                        jebp_int width) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    __m128i v_top;
    if (width >= 4) {
        v_top = _mm_loadu_si128((__m128i *)top);
    }
    for (; x + 4 <= width; x += 4) {
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        __m128i v_next = _mm_loadu_si128((__m128i *)&top[x + 4]);
        __m128i v_tr = jebp__sse_move_px1(v_top, v_next);
        v_tr = _mm_shuffle_epi32(v_tr, _MM_SHUFFLE(0, 3, 2, 1));
        v_tr = jebp__sse_avg_u8x16(v_top, v_tr);
        v_pixel = _mm_add_epi8(v_pixel, v_tr);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
        v_top = v_next;
    }
#elif defined(JEBP__SIMD_NEON)
    uint8x16_t v_top;
    if (width >= 4) {
        v_top = vld1q_u8((uint8_t *)top);
    }
    for (; x + 4 <= width; x += 4) {
        uint8x16_t v_pixel = vld1q_u8((uint8_t *)&pixel[x]);
        uint8x16_t v_next = vld1q_u8((uint8_t *)&top[x + 4]);
        uint8x16_t v_tr = vextq_u8(v_top, v_next, 4);
        v_tr = vhaddq_u8(v_top, v_tr);
        v_pixel = vaddq_u8(v_pixel, v_tr);
        vst1q_u8((uint8_t *)&pixel[x], v_pixel);
        v_top = v_next;
    }
#endif
    for (; x < width; x += 1) {
        pixel[x].r += JEBP__FAVG(top[x].r, top[x + 1].r);
        pixel[x].g += JEBP__FAVG(top[x].g, top[x + 1].g);
        pixel[x].b += JEBP__FAVG(top[x].b, top[x + 1].b);
        pixel[x].a += JEBP__FAVG(top[x].a, top[x + 1].a);
    }
}

static void jebp__vp8l_pred8(jebp_color_t *pixel, jebp_color_t *top,
                             jebp_int width) {
    jebp__vp8l_pred_avgtr(pixel, &top[-1], width);
}

static void jebp__vp8l_pred9(jebp_color_t *pixel, jebp_color_t *top,
                             jebp_int width) {
    jebp__vp8l_pred_avgtr(pixel, top, width);
}

static void jebp__vp8l_pred10(jebp_color_t *pixel, jebp_color_t *top,
                              jebp_int width) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    __m128i v_left;
    __m128i v_tl;
    __m128i v_top;
    if (width >= 4) {
        v_left = _mm_cvtsi32_si128(*(int *)&pixel[-1]);
        v_tl = _mm_cvtsi32_si128(*(int *)&top[-1]);
        v_top = _mm_loadu_si128((__m128i *)top);
    }
    for (; x + 4 <= width; x += 4) {
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        __m128i v_next = _mm_loadu_si128((__m128i *)&top[x + 4]);
        __m128i v_rot = _mm_shuffle_epi32(v_top, _MM_SHUFFLE(2, 1, 0, 3));
        v_tl = jebp__sse_move_px1(v_rot, v_tl);
        __m128i v_tr = jebp__sse_move_px1(v_top, v_next);
        v_tr = _mm_shuffle_epi32(v_tr, _MM_SHUFFLE(0, 3, 2, 1));
        v_tr = jebp__sse_avg_u8x16(v_top, v_tr);
        jebp__m128x4i v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            __m128i v_avg = jebp__sse_avg2_u8x16(v_left, v_tl, v_tr);
            v_pixel4.v[i] = _mm_add_epi8(v_pixel, v_avg);
            v_left = _mm_shuffle_epi32(v_pixel4.v[i], _MM_SHUFFLE(2, 1, 0, 3));
        })
        v_pixel = jebp__sse_flatten_px4(v_pixel4);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
        v_tl = v_rot;
        v_top = v_next;
    }
#elif defined(JEBP__SIMD_NEON)
    uint8x16_t v_left;
    uint8x16_t v_tl;
    uint8x16_t v_top;
    if (width >= 4) {
        v_left = jebp__neon_load_px1(&pixel[-1]);
        v_tl = jebp__neon_load_px1(&top[-1]);
        v_top = vld1q_u8((uint8_t *)top);
    }
    for (; x + 4 <= width; x += 4) {
        uint8x16_t v_pixel = vld1q_u8((uint8_t *)&pixel[x]);
        uint8x16_t v_next = vld1q_u8((uint8_t *)&top[x + 4]);
        v_tl = vextq_u8(v_tl, v_top, 12);
        uint8x16_t v_tr = vextq_u8(v_top, v_next, 4);
        v_tr = vhaddq_u8(v_top, v_tr);
        uint8x16x4_t v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            uint8x16_t v_avg = vhaddq_u8(v_left, v_tl);
            v_avg = vhaddq_u8(v_avg, v_tr);
            v_pixel4.val[i] = vaddq_u8(v_pixel, v_avg);
            v_left = vextq_u8(v_pixel4.val[i], v_pixel4.val[i], 12);
        })
        v_pixel = jebp__neon_flatten_px4(v_pixel4);
        vst1q_u8((uint8_t *)&pixel[x], v_pixel);
        v_tl = v_top;
        v_top = v_next;
    }
#endif
    for (; x < width; x += 1) {
        pixel[x].r += JEBP__FAVG(JEBP__FAVG(pixel[x - 1].r, top[x - 1].r),
                                 JEBP__FAVG(top[x].r, top[x + 1].r));
        pixel[x].g += JEBP__FAVG(JEBP__FAVG(pixel[x - 1].g, top[x - 1].g),
                                 JEBP__FAVG(top[x].g, top[x + 1].g));
        pixel[x].b += JEBP__FAVG(JEBP__FAVG(pixel[x - 1].b, top[x - 1].b),
                                 JEBP__FAVG(top[x].b, top[x + 1].b));
        pixel[x].a += JEBP__FAVG(JEBP__FAVG(pixel[x - 1].a, top[x - 1].a),
                                 JEBP__FAVG(top[x].a, top[x + 1].a));
    }
}

JEBP__INLINE jebp_int jebp__vp8l_pred_dist(jebp_color_t *pix1,
                                           jebp_color_t *pix2) {
    return JEBP__ABS(pix1->r - pix2->r) + JEBP__ABS(pix1->g - pix2->g) +
           JEBP__ABS(pix1->b - pix2->b) + JEBP__ABS(pix1->a - pix2->a);
}

static void jebp__vp8l_pred11(jebp_color_t *pixel, jebp_color_t *top,
                              jebp_int width) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    __m128i v_left;
    __m128i v_tl;
    if (width >= 4) {
        v_left = _mm_cvtsi32_si128(*(int *)&pixel[-1]);
        v_tl = _mm_cvtsi32_si128(*(int *)&top[-1]);
    }
    for (; x + 4 <= width; x += 4) {
        __m128i v_ldist, v_tdist, v_cmp, v_pixello, v_pixelhi;
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        __m128i v_top = _mm_loadu_si128((__m128i *)&top[x]);
        __m128i v_rot = _mm_shuffle_epi32(v_top, _MM_SHUFFLE(2, 1, 0, 3));
        v_tl = jebp__sse_move_px1(v_rot, v_tl);
        // Pixel 0
        // This does double the SAD result but if both distances are doubled the
        // comparison should still be the same
        __m128i v_tllo = _mm_unpacklo_epi32(v_tl, v_tl);
        __m128i v_toplo = _mm_unpacklo_epi32(v_top, v_top);
        v_ldist = _mm_sad_epu8(v_tllo, v_toplo);
        v_tdist = _mm_unpacklo_epi32(v_left, v_left);
        v_tdist = _mm_sad_epu8(v_tllo, v_tdist);
        v_cmp = _mm_cmplt_epi32(v_ldist, v_tdist);
        v_pixello = jebp__sse_bsela_u8x16(v_pixel, v_cmp, v_left, v_top);
        v_left = _mm_bslli_si128(v_pixello, 4);
        // Pixel 1
        v_tdist = _mm_unpacklo_epi32(v_left, v_left);
        v_tdist = _mm_sad_epu8(v_tllo, v_tdist);
        v_cmp = _mm_cmplt_epi32(v_ldist, v_tdist);
        v_cmp = _mm_bsrli_si128(v_cmp, 4);
        v_pixello = jebp__sse_bsela_u8x16(v_pixel, v_cmp, v_left, v_top);
        v_pixello = _mm_unpacklo_epi32(v_left, v_pixello);
        v_left = _mm_bsrli_si128(v_pixello, 4);
        // Pixel 2
        __m128i v_tlhi = _mm_shuffle_epi32(v_tl, _MM_SHUFFLE(2, 2, 3, 3));
        __m128i v_tophi = _mm_shuffle_epi32(v_top, _MM_SHUFFLE(2, 2, 3, 3));
        v_ldist = _mm_sad_epu8(v_tlhi, v_tophi);
        v_tdist = _mm_shuffle_epi32(v_left, _MM_SHUFFLE(2, 2, 3, 3));
        v_tdist = _mm_sad_epu8(v_tlhi, v_tdist);
        v_cmp = _mm_cmplt_epi32(v_ldist, v_tdist);
        v_pixelhi = jebp__sse_bsela_u8x16(v_pixel, v_cmp, v_left, v_top);
        v_left = _mm_bslli_si128(v_pixelhi, 4);
        // Pixel 3
        v_tdist = _mm_shuffle_epi32(v_left, _MM_SHUFFLE(2, 2, 3, 3));
        v_tdist = _mm_sad_epu8(v_tlhi, v_tdist);
        v_cmp = _mm_cmplt_epi32(v_ldist, v_tdist);
        v_cmp = _mm_bslli_si128(v_cmp, 12);
        v_pixelhi = jebp__sse_bsela_u8x16(v_pixel, v_cmp, v_left, v_top);
        v_pixelhi = _mm_unpackhi_epi32(v_left, v_pixelhi);
        v_left = _mm_bsrli_si128(v_pixelhi, 12);
        v_pixel = _mm_unpackhi_epi64(v_pixello, v_pixelhi);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
        v_tl = v_rot;
    }
#elif defined(JEBP__SIMD_NEON)
    uint8x16_t v_left;
    uint8x16_t v_tl;
    if (width >= 4) {
        v_left = jebp__neon_load_px1(&pixel[-1]);
        v_tl = jebp__neon_load_px1(&top[-1]);
    }
    for (; x + 4 <= width; x += 4) {
        uint8x16_t v_pixel = vld1q_u8((uint8_t *)&pixel[x]);
        uint8x16_t v_top = vld1q_u8((uint8_t *)&top[x]);
        v_tl = vextq_u8(v_tl, v_top, 12);
        uint32x4_t v_ldist = jebp__neon_sad_px4(v_tl, v_top);
        uint8x16x4_t v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            uint32x4_t v_tdist = jebp__neon_sad_px4(v_tl, v_left);
            uint32x4_t v_cmp = vcltq_u32(v_ldist, v_tdist);
            uint8x16_t v_pred = vbslq_u8((uint8x16_t)v_cmp, v_left, v_top);
            v_pixel4.val[i] = vaddq_u8(v_pixel, v_pred);
            v_left = vextq_u8(v_pixel4.val[i], v_pixel4.val[i], 12);
        })
        v_pixel = jebp__neon_flatten_px4(v_pixel4);
        vst1q_u8((uint8_t *)&pixel[x], v_pixel);
        v_tl = v_top;
    }
#endif
    for (; x < width; x += 1) {
        jebp_int ldist = jebp__vp8l_pred_dist(&top[x - 1], &top[x]);
        jebp_int tdist = jebp__vp8l_pred_dist(&top[x - 1], &pixel[x - 1]);
        if (ldist < tdist) {
            jebp__vp8l_pred_left(&pixel[x], 1);
        } else {
            jebp__vp8l_pred_top(&pixel[x], &top[x], 1);
        }
    }
}

static void jebp__vp8l_pred12(jebp_color_t *pixel, jebp_color_t *top,
                              jebp_int width) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    __m128i v_left;
    __m128i v_tl;
    if (width >= 4) {
        v_left = _mm_cvtsi32_si128(*(int *)&pixel[-1]);
        v_tl = _mm_cvtsi32_si128(*(int *)&top[-1]);
    }
    for (; x + 4 <= width; x += 4) {
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        __m128i v_top = _mm_loadu_si128((__m128i *)&top[x]);
        __m128i v_rot = _mm_shuffle_epi32(v_top, _MM_SHUFFLE(2, 1, 0, 3));
        v_tl = jebp__sse_move_px1(v_rot, v_tl);
        __m128i v_max = _mm_max_epu8(v_top, v_tl);
        __m128i v_min = _mm_min_epu8(v_top, v_tl);
        __m128i v_diff = _mm_sub_epi8(v_max, v_min);
        __m128i v_pos = _mm_cmpeq_epi8(v_max, v_top);
        jebp__m128x4i v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            __m128i v_add = _mm_adds_epu8(v_left, v_diff);
            __m128i v_sub = _mm_subs_epu8(v_left, v_diff);
            v_pixel4.v[i] = jebp__sse_bsela_u8x16(v_pixel, v_pos, v_add, v_sub);
            v_left = _mm_shuffle_epi32(v_pixel4.v[i], _MM_SHUFFLE(2, 1, 0, 3));
        })
        v_pixel = jebp__sse_flatten_px4(v_pixel4);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
        v_tl = v_rot;
    }
#elif defined(JEBP__SIMD_NEON)
    uint8x16_t v_left;
    uint8x16_t v_tl;
    if (width >= 4) {
        v_left = jebp__neon_load_px1(&pixel[-1]);
        v_tl = jebp__neon_load_px1(&top[-1]);
    }
    for (; x + 4 <= width; x += 4) {
        uint8x16_t v_pixel = vld1q_u8((uint8_t *)&pixel[x]);
        uint8x16_t v_top = vld1q_u8((uint8_t *)&top[x]);
        v_tl = vextq_u8(v_tl, v_top, 12);
        uint8x16_t v_diff = vabdq_u8(v_top, v_tl);
        uint8x16_t v_neg = vcltq_u8(v_top, v_tl);
        uint8x16x4_t v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            uint8x16_t v_add = vqaddq_u8(v_left, v_diff);
            uint8x16_t v_sub = vqsubq_u8(v_left, v_diff);
            uint8x16_t v_pred = vbslq_u8(v_neg, v_sub, v_add);
            v_pixel4.val[i] = vaddq_u8(v_pixel, v_pred);
            v_left = vextq_u8(v_pixel4.val[i], v_pixel4.val[i], 12);
        })
        v_pixel = jebp__neon_flatten_px4(v_pixel4);
        vst1q_u8((uint8_t *)&pixel[x], v_pixel);
        v_tl = v_top;
    }
#endif
    for (; x < width; x += 1) {
        pixel[x].r +=
            JEBP__CLAMP_UBYTE(pixel[x - 1].r + top[x].r - top[x - 1].r);
        pixel[x].g +=
            JEBP__CLAMP_UBYTE(pixel[x - 1].g + top[x].g - top[x - 1].g);
        pixel[x].b +=
            JEBP__CLAMP_UBYTE(pixel[x - 1].b + top[x].b - top[x - 1].b);
        pixel[x].a +=
            JEBP__CLAMP_UBYTE(pixel[x - 1].a + top[x].a - top[x - 1].a);
    }
}

static void jebp__vp8l_pred13(jebp_color_t *pixel, jebp_color_t *top,
                              jebp_int width) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    __m128i v_mask = _mm_set1_epi8(0x7f);
    __m128i v_left;
    __m128i v_tl;
    if (width >= 4) {
        v_left = _mm_cvtsi32_si128(*(int *)&pixel[-1]);
        v_tl = _mm_cvtsi32_si128(*(int *)&top[-1]);
    }
    for (; x + 4 <= width; x += 4) {
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        __m128i v_top = _mm_loadu_si128((__m128i *)&top[x]);
        __m128i v_rot = _mm_shuffle_epi32(v_top, _MM_SHUFFLE(2, 1, 0, 3));
        v_tl = jebp__sse_move_px1(v_rot, v_tl);
        jebp__m128x4i v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            __m128i v_avg = jebp__sse_avg_u8x16(v_left, v_top);
            __m128i v_max = _mm_max_epu8(v_avg, v_tl);
            __m128i v_min = _mm_min_epu8(v_avg, v_tl);
            __m128i v_diff = _mm_sub_epi8(v_max, v_min);
            v_diff = _mm_srli_epi16(v_diff, 1);
            v_diff = _mm_and_si128(v_diff, v_mask);
            __m128i v_pos = _mm_cmpeq_epi8(v_max, v_avg);
            __m128i v_add = _mm_adds_epu8(v_avg, v_diff);
            __m128i v_sub = _mm_subs_epu8(v_avg, v_diff);
            v_pixel4.v[i] = jebp__sse_bsela_u8x16(v_pixel, v_pos, v_add, v_sub);
            v_left = _mm_shuffle_epi32(v_pixel4.v[i], _MM_SHUFFLE(2, 1, 0, 3));
        })
        v_pixel = jebp__sse_flatten_px4(v_pixel4);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
        v_tl = v_rot;
    }
#elif defined(JEBP__SIMD_NEON)
    uint8x16_t v_left;
    uint8x16_t v_tl;
    if (width >= 4) {
        v_left = jebp__neon_load_px1(&pixel[-1]);
        v_tl = jebp__neon_load_px1(&top[-1]);
    }
    for (; x + 4 <= width; x += 4) {
        uint8x16_t v_pixel = vld1q_u8((uint8_t *)&pixel[x]);
        uint8x16_t v_top = vld1q_u8((uint8_t *)&top[x]);
        v_tl = vextq_u8(v_tl, v_top, 12);
        uint8x16x4_t v_pixel4;
        JEBP__UNROLL4(jebp_int i, {
            uint8x16_t v_avg = vhaddq_u8(v_left, v_top);
            uint8x16_t v_diff = vabdq_u8(v_avg, v_tl);
            v_diff = vshrq_n_u8(v_diff, 1);
            uint8x16_t v_neg = vcltq_u8(v_avg, v_tl);
            uint8x16_t v_add = vqaddq_u8(v_avg, v_diff);
            uint8x16_t v_sub = vqsubq_u8(v_avg, v_diff);
            uint8x16_t v_pred = vbslq_u8(v_neg, v_sub, v_add);
            v_pixel4.val[i] = vaddq_u8(v_pixel, v_pred);
            v_left = vextq_u8(v_pixel4.val[i], v_pixel4.val[i], 12);
        })
        v_pixel = jebp__neon_flatten_px4(v_pixel4);
        vst1q_u8((uint8_t *)&pixel[x], v_pixel);
        v_tl = v_top;
    }
#endif
    for (; x < width; x += 1) {
        jebp_color_t avg = {JEBP__FAVG(pixel[x - 1].r, top[x].r),
                            JEBP__FAVG(pixel[x - 1].g, top[x].g),
                            JEBP__FAVG(pixel[x - 1].b, top[x].b),
                            JEBP__FAVG(pixel[x - 1].a, top[x].a)};
        pixel[x].r += JEBP__CLAMP_UBYTE(avg.r + (avg.r - top[x - 1].r) / 2);
        pixel[x].g += JEBP__CLAMP_UBYTE(avg.g + (avg.g - top[x - 1].g) / 2);
        pixel[x].b += JEBP__CLAMP_UBYTE(avg.b + (avg.b - top[x - 1].b) / 2);
        pixel[x].a += JEBP__CLAMP_UBYTE(avg.a + (avg.a - top[x - 1].a) / 2);
    }
}

static const jebp__vp8l_pred_t jebp__vp8l_preds[JEBP__NB_VP8L_PRED_TYPES] = {
    jebp__vp8l_pred0,  jebp__vp8l_pred1, jebp__vp8l_pred2,  jebp__vp8l_pred3,
    jebp__vp8l_pred4,  jebp__vp8l_pred5, jebp__vp8l_pred6,  jebp__vp8l_pred7,
    jebp__vp8l_pred8,  jebp__vp8l_pred9, jebp__vp8l_pred10, jebp__vp8l_pred11,
    jebp__vp8l_pred12, jebp__vp8l_pred13};

/**
 * VP8L transforms
 */
typedef enum jebp__transform_type_t {
    JEBP__TRANSFORM_PREDICT,
    JEBP__TRANSFORM_COLOR,
    JEBP__TRANSFORM_GREEN,
    JEBP__TRANSFORM_PALETTE,
    JEBP__NB_TRANSFORMS
} jebp__transform_type_t;

typedef struct jebp__transform_t {
    jebp__transform_type_t type;
    jebp__subimage_t image;
} jebp__transform_t;

static jebp_error_t jebp__read_transform(jebp__transform_t *transform,
                                         jebp__bit_reader_t *bits,
                                         jebp_image_t *image) {
    jebp_error_t err = JEBP_OK;
    transform->type = jebp__read_bits(bits, 2, &err);
    if (err != JEBP_OK) {
        return err;
    }
    if (transform->type == JEBP__TRANSFORM_PALETTE) {
        // TODO: support palette images
        return JEBP_ERROR_NOSUP_PALETTE;
    } else if (transform->type != JEBP__TRANSFORM_GREEN) {
        err = jebp__read_subimage(&transform->image, bits, image);
    }
    return err;
}

static void jebp__free_transform(jebp__transform_t *transform) {
    if (transform->type != JEBP__TRANSFORM_GREEN) {
        jebp_free_image((jebp_image_t *)&transform->image);
    }
}

JEBP__INLINE jebp_error_t jebp__apply_predict_row(jebp_color_t *pixel,
                                                  jebp_color_t *top,
                                                  jebp_int width,
                                                  jebp_color_t *predict_pixel) {
    if (predict_pixel->g >= JEBP__NB_VP8L_PRED_TYPES) {
        return JEBP_ERROR_INVDATA;
    }
    jebp__vp8l_preds[predict_pixel->g](pixel, top, width);
    return JEBP_OK;
}

JEBP__INLINE jebp_error_t jebp__apply_predict_transform(
    jebp_image_t *image, jebp__subimage_t *predict_image) {
    jebp_error_t err;
    jebp_color_t *pixel = image->pixels;
    jebp_color_t *top = pixel;
    jebp_int predict_width = predict_image->width - 1;
    jebp_int block_size = 1 << predict_image->block_bits;
    jebp_int end_size =
        image->width - (predict_width << predict_image->block_bits);
    if (predict_width == 0) {
        // Special case: if there is only one block the first block which is
        //               shortened by one pixel (due to the left prediction)
        //               needs to be `end_size` and the proper end block then
        //               needs to be skipped.
        block_size = end_size;
        end_size = 0;
    }
    // Use opaque-black prediction for the top-left pixel
    jebp__vp8l_pred_black(pixel, 1);
    // Use left prediction for the top row
    jebp__vp8l_pred_left(pixel + 1, image->width - 1);
    pixel += image->width;
    for (jebp_int y = 1; y < image->height; y += 1) {
        jebp_color_t *predict_row =
            &predict_image->pixels[(y >> predict_image->block_bits) *
                                   predict_image->width];
        // Use top prediction for the left column
        jebp__vp8l_pred_top(pixel, top, 1);
        // Finish the rest of the first block
        if ((err = jebp__apply_predict_row(pixel + 1, top + 1, block_size - 1,
                                           predict_row)) != JEBP_OK) {
            return err;
        }
        pixel += block_size;
        top += block_size;
        for (jebp_int x = 1; x < predict_width; x += 1) {
            if ((err = jebp__apply_predict_row(pixel, top, block_size,
                                               &predict_row[x])) != JEBP_OK) {
                return err;
            }
            pixel += block_size;
            top += block_size;
        }
        jebp__apply_predict_row(pixel, top, end_size,
                                &predict_row[predict_width]);
        pixel += end_size;
        top += end_size;
    }
    return JEBP_OK;
}

JEBP__INLINE void jebp__apply_color_row(jebp_color_t *pixel, jebp_int width,
                                        jebp_color_t *color_pixel) {
    jebp_int x = 0;
#if defined(JEBP__SIMD_SSE2)
    jebp_ushort color_r = ((jebp_short)(color_pixel->r << 8) >> 5);
    jebp_ushort color_g = ((jebp_short)(color_pixel->g << 8) >> 5);
    jebp_ushort color_b = ((jebp_short)(color_pixel->b << 8) >> 5);
    __m128i v_color_bg = _mm_set1_epi32(color_b | ((jebp_uint)color_g << 16));
    __m128i v_color_r = _mm_set1_epi32(color_r);
    __m128i v_masklo = _mm_set1_epi16((short)0x00ff);
    __m128i v_maskhi = _mm_set1_epi16((short)0xff00);
    for (; x + 4 <= width; x += 4) {
        __m128i v_pixel = _mm_loadu_si128((__m128i *)&pixel[x]);
        __m128i v_green = _mm_and_si128(v_pixel, v_maskhi);
        v_green = _mm_shufflelo_epi16(v_green, _MM_SHUFFLE(2, 2, 0, 0));
        v_green = _mm_shufflehi_epi16(v_green, _MM_SHUFFLE(2, 2, 0, 0));
        __m128i v_bg = _mm_mulhi_epi16(v_green, v_color_bg);
        v_bg = _mm_and_si128(v_bg, v_masklo);
        v_pixel = _mm_add_epi8(v_pixel, v_bg);
        __m128i v_red = _mm_slli_epi16(v_pixel, 8);
        v_red = _mm_mulhi_epi16(v_red, v_color_r);
        v_red = _mm_and_si128(v_red, v_masklo);
        v_red = _mm_slli_epi32(v_red, 16);
        v_pixel = _mm_add_epi8(v_pixel, v_red);
        _mm_storeu_si128((__m128i *)&pixel[x], v_pixel);
    }
#elif defined(JEBP__SIMD_NEON)
    int8x8x3_t v_color_pixel = vld3_dup_s8((jebp_byte *)color_pixel);
    for (; x + 8 <= width; x += 8) {
        int16x8_t v_mul;
        int8x8_t v_shr;
        int8x8x4_t v_pixel = vld4_s8((jebp_byte *)&pixel[x]);
        v_mul = vmull_s8(v_pixel.val[1], v_color_pixel.val[2]);
        v_shr = vshrn_n_s16(v_mul, 5);
        v_pixel.val[0] = vadd_s8(v_pixel.val[0], v_shr);
        v_mul = vmull_s8(v_pixel.val[1], v_color_pixel.val[1]);
        v_shr = vshrn_n_s16(v_mul, 5);
        v_pixel.val[2] = vadd_s8(v_pixel.val[2], v_shr);
        v_mul = vmull_s8(v_pixel.val[0], v_color_pixel.val[0]);
        v_shr = vshrn_n_s16(v_mul, 5);
        v_pixel.val[2] = vadd_s8(v_pixel.val[2], v_shr);
        vst4_s8((jebp_byte *)&pixel[x], v_pixel);
    }
#endif
    for (; x < width; x += 1) {
        pixel[x].r += ((jebp_byte)pixel[x].g * (jebp_byte)color_pixel->b) >> 5;
        pixel[x].b += ((jebp_byte)pixel[x].g * (jebp_byte)color_pixel->g) >> 5;
        pixel[x].b += ((jebp_byte)pixel[x].r * (jebp_byte)color_pixel->r) >> 5;
    }
}

JEBP__INLINE jebp_error_t jebp__apply_color_transform(
    jebp_image_t *image, jebp__subimage_t *color_image) {
    jebp_color_t *pixel = image->pixels;
    jebp_int color_width = color_image->width - 1;
    jebp_int block_size = 1 << color_image->block_bits;
    jebp_int end_size = image->width - (color_width << color_image->block_bits);
    for (jebp_int y = 0; y < image->height; y += 1) {
        jebp_color_t *color_row =
            &color_image
                 ->pixels[(y >> color_image->block_bits) * color_image->width];
        for (jebp_int x = 0; x < color_width; x += 1) {
            jebp__apply_color_row(pixel, block_size, &color_row[x]);
            pixel += block_size;
        }
        jebp__apply_color_row(pixel, end_size, &color_row[color_width]);
        pixel += end_size;
    }
    return JEBP_OK;
}

JEBP__INLINE jebp_error_t jebp__apply_green_transform(jebp_image_t *image) {
    jebp_int size = image->width * image->height;
    jebp_int i = 0;
#if defined(JEBP__SIMD_SSE2)
    for (; i + 4 <= size; i += 4) {
        __m128i *pixel = (__m128i *)&image->pixels[i];
        __m128i v_pixel = _mm_loadu_si128(pixel);
        __m128i v_green = _mm_srli_epi16(v_pixel, 8);
        v_green = _mm_shufflelo_epi16(v_green, _MM_SHUFFLE(2, 2, 0, 0));
        v_green = _mm_shufflehi_epi16(v_green, _MM_SHUFFLE(2, 2, 0, 0));
        v_pixel = _mm_add_epi8(v_pixel, v_green);
        _mm_storeu_si128(pixel, v_pixel);
    }
#elif defined(JEBP__SIMD_NEON)
    for (; i + 16 <= size; i += 16) {
        jebp_ubyte *pixel = (jebp_ubyte *)&image->pixels[i];
        uint8x16x4_t v_pixel = vld4q_u8(pixel);
        v_pixel.val[0] = vaddq_u8(v_pixel.val[0], v_pixel.val[1]);
        v_pixel.val[2] = vaddq_u8(v_pixel.val[2], v_pixel.val[1]);
        vst4q_u8(pixel, v_pixel);
    }
#endif
    for (; i < size; i += 1) {
        jebp_color_t *pixel = &image->pixels[i];
        pixel->r += pixel->g;
        pixel->b += pixel->g;
    }
    return JEBP_OK;
}

static jebp_error_t jebp__apply_transform(jebp__transform_t *transform,
                                          jebp_image_t *image) {
    switch (transform->type) {
    case JEBP__TRANSFORM_PREDICT:
        return jebp__apply_predict_transform(image, &transform->image);
    case JEBP__TRANSFORM_COLOR:
        return jebp__apply_color_transform(image, &transform->image);
    case JEBP__TRANSFORM_GREEN:
        return jebp__apply_green_transform(image);
    default:
        return JEBP_ERROR_NOSUP;
    }
}

/**
 * VP8L lossless codec
 */
#define JEBP__VP8L_TAG 0x4c385056
#define JEBP__VP8L_MAGIC 0x2f

static jebp_error_t jebp__read_vp8l_header(jebp_image_t *image,
                                           jebp__reader_t *reader,
                                           jebp__bit_reader_t *bits,
                                           jebp__chunk_t *chunk) {
    jebp_error_t err = JEBP_OK;
    if (chunk->size < 5) {
        return JEBP_ERROR_INVDATA_HEADER;
    }
    if (jebp__read_uint8(reader, &err) != JEBP__VP8L_MAGIC) {
        return jebp__error(&err, JEBP_ERROR_INVDATA_HEADER);
    }
    jepb__init_bit_reader(bits, reader, chunk->size - 1);
    image->width = jebp__read_bits(bits, 14, &err) + 1;
    image->height = jebp__read_bits(bits, 14, &err) + 1;
    jebp__read_bits(bits, 1, &err); // alpha does not impact decoding
    if (jebp__read_bits(bits, 3, &err) != 0) {
        // version must be 0
        return jebp__error(&err, JEBP_ERROR_NOSUP);
    }
    return err;
}

static jebp_error_t jebp__read_vp8l_size(jebp_image_t *image,
                                         jebp__reader_t *reader,
                                         jebp__chunk_t *chunk) {
    jebp__bit_reader_t bits;
    return jebp__read_vp8l_header(image, reader, &bits, chunk);
}

static jebp_error_t jebp__read_vp8l_nohead(jebp_image_t *image,
                                           jebp__bit_reader_t *bits) {
    jebp_error_t err = JEBP_OK;
    jebp__transform_t transforms[4];
    jebp_int nb_transforms = 0;
    for (; nb_transforms <= JEBP__NB_TRANSFORMS; nb_transforms += 1) {
        if (!jebp__read_bits(bits, 1, &err)) {
            // no more transforms to read
            break;
        }
        if (err != JEBP_OK || nb_transforms == JEBP__NB_TRANSFORMS) {
            // too many transforms
            jebp__error(&err, JEBP_ERROR_INVDATA);
            goto free_transforms;
        }
        if ((err = jebp__read_transform(&transforms[nb_transforms], bits,
                                        image)) != JEBP_OK) {
            goto free_transforms;
        }
    }
    if (err != JEBP_OK) {
        goto free_transforms;
    }

    jebp__colcache_t colcache;
    if ((err = jebp__read_colcache(&colcache, bits)) != JEBP_OK) {
        goto free_transforms;
    }
    jebp__subimage_t *huffman_image = &(jebp__subimage_t){0};
    if (!jebp__read_bits(bits, 1, &err)) {
        // there is no huffman image
        huffman_image = NULL;
    }
    if (err != JEBP_OK) {
        jebp__free_colcache(&colcache);
        goto free_transforms;
    }
    if (huffman_image != NULL) {
        if ((err = jebp__read_subimage(huffman_image, bits, image)) !=
            JEBP_OK) {
            jebp__free_colcache(&colcache);
            goto free_transforms;
        }
    }
    err = jebp__read_vp8l_image(image, bits, &colcache, huffman_image);
    jebp__free_colcache(&colcache);
    jebp_free_image((jebp_image_t *)huffman_image);

free_transforms:
    for (nb_transforms -= 1; nb_transforms >= 0; nb_transforms -= 1) {
        if (err == JEBP_OK) {
            err = jebp__apply_transform(&transforms[nb_transforms], image);
        }
        jebp__free_transform(&transforms[nb_transforms]);
    }
    return err;
}

static jebp_error_t jebp__read_vp8l(jebp_image_t *image, jebp__reader_t *reader,
                                    jebp__chunk_t *chunk) {
    jebp_error_t err;
    jebp__bit_reader_t bits;
    if ((err = jebp__read_vp8l_header(image, reader, &bits, chunk)) !=
        JEBP_OK) {
        return err;
    }
    if ((err = jebp__read_vp8l_nohead(image, &bits)) != JEBP_OK) {
        return err;
    }
    return JEBP_OK;
}
#endif // JEBP_NO_VP8L

/**
 * Public API
 */
static const char *const jebp__error_strings[JEBP_NB_ERRORS];

const char *jebp_error_string(jebp_error_t err) {
    if (err < 0 || err >= JEBP_NB_ERRORS) {
        err = JEBP_ERROR_UNKNOWN;
    }
    return jebp__error_strings[err];
}

void jebp_free_image(jebp_image_t *image) {
    if (image != NULL) {
        JEBP_FREE(image->pixels);
        JEBP__CLEAR(image, sizeof(jebp_image_t));
    }
}

static jebp_error_t jebp__read_size(jebp_image_t *image,
                                    jebp__reader_t *reader) {
    jebp_error_t err;
    jebp__riff_reader_t riff;
    JEBP__CLEAR(image, sizeof(jebp_image_t));
    if ((err = jebp__read_riff_header(&riff, reader)) != JEBP_OK) {
        return err;
    }
    jebp__chunk_t chunk;
    if ((err = jebp__read_riff_chunk(&riff, &chunk)) != JEBP_OK) {
        return err;
    }

    switch (chunk.tag) {
#ifndef JEBP_NO_VP8
    case JEBP__VP8_TAG:
        return jebp__read_vp8_size(image, reader, &chunk);
#endif // JEBP_NO_VP8
#ifndef JEBP_NO_VP8L
    case JEBP__VP8L_TAG:
        return jebp__read_vp8l_size(image, reader, &chunk);
#endif // JEBP_NO_VP8L
    default:
        return JEBP_ERROR_NOSUP_CODEC;
    }
}

jebp_error_t jebp_decode_size(jebp_image_t *image, size_t size,
                              const void *data) {
    if (image == NULL || data == NULL) {
        return JEBP_ERROR_INVAL;
    }
    jebp__reader_t reader;
    jebp__init_memory(&reader, size, data);
    return jebp__read_size(image, &reader);
}

static jebp_error_t jebp__read(jebp_image_t *image, jebp__reader_t *reader) {
    jebp_error_t err;
    jebp__riff_reader_t riff;
    JEBP__CLEAR(image, sizeof(jebp_image_t));
    if ((err = jebp__read_riff_header(&riff, reader)) != JEBP_OK) {
        return err;
    }
    jebp__chunk_t chunk;
    if ((err = jebp__read_riff_chunk(&riff, &chunk)) != JEBP_OK) {
        return err;
    }

    switch (chunk.tag) {
#ifndef JEBP_NO_VP8
    case JEBP__VP8_TAG:
        return jebp__read_vp8(image, reader, &chunk);
#endif // JEBP_NO_VP8
#ifndef JEBP_NO_VP8L
    case JEBP__VP8L_TAG:
        return jebp__read_vp8l(image, reader, &chunk);
#endif // JEBP_NO_VP8L
    default:
        return JEBP_ERROR_NOSUP_CODEC;
    }
}

jebp_error_t jebp_decode(jebp_image_t *image, size_t size, const void *data) {
    if (image == NULL || data == NULL) {
        return JEBP_ERROR_INVAL;
    }
    jebp__reader_t reader;
    jebp__init_memory(&reader, size, data);
    return jebp__read(image, &reader);
}

#ifndef JEBP_NO_STDIO
jebp_error_t jebp_read_size(jebp_image_t *image, const char *path) {
    jebp_error_t err;
    if (image == NULL || path == NULL) {
        return JEBP_ERROR_INVAL;
    }
    jebp__reader_t reader;
    if ((err = jebp__open_file(&reader, path)) != JEBP_OK) {
        return err;
    }
    err = jebp__read_size(image, &reader);
    jebp__close_file(&reader);
    return err;
}

jebp_error_t jebp_read(jebp_image_t *image, const char *path) {
    jebp_error_t err;
    if (image == NULL || path == NULL) {
        return JEBP_ERROR_INVAL;
    }
    jebp__reader_t reader;
    if ((err = jebp__open_file(&reader, path)) != JEBP_OK) {
        return err;
    }
    err = jebp__read(image, &reader);
    jebp__close_file(&reader);
    return err;
}
#endif // JEBP_NO_STDIO

/**
 * Lookup tables
 */
// These are moved to the end of the file since some of them are very large and
// putting them in the middle of the code would disrupt the flow of reading.
// Especially since in most situations the values in these tables are
// unimportant to the developer.

#ifndef JEBP_NO_VP8
// Lookup table mapping quantizer indices to DC values
static const jebp_short jebp__dc_quant_table[JEBP__NB_QUANT_INDEXES] = {
    4,   5,   6,   7,   8,   9,   10,  10,  11,  12,  13,  14,  15,  16,  17,
    17,  18,  19,  20,  20,  21,  21,  22,  22,  23,  23,  24,  25,  25,  26,
    27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  37,  38,  39,  40,
    41,  42,  43,  44,  45,  46,  46,  47,  48,  49,  50,  51,  52,  53,  54,
    55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,
    70,  71,  72,  73,  74,  75,  76,  76,  77,  78,  79,  80,  81,  82,  83,
    84,  85,  86,  87,  88,  89,  91,  93,  95,  96,  98,  100, 101, 102, 104,
    106, 108, 110, 112, 114, 116, 118, 122, 124, 126, 128, 130, 132, 134, 136,
    138, 140, 143, 145, 148, 151, 154, 157};

// Lookup table mapping quantizer indices to AC values
static const jebp_short jebp__ac_quant_table[JEBP__NB_QUANT_INDEXES] = {
    4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,
    19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,
    34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  60,  62,  64,  66,  68,
    70,  72,  74,  76,  78,  80,  82,  84,  86,  88,  90,  92,  94,  96,  98,
    100, 102, 104, 106, 108, 110, 112, 114, 116, 119, 122, 125, 128, 131, 134,
    137, 140, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170, 173, 177, 181,
    185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 234, 239, 245,
    249, 254, 259, 264, 269, 274, 279, 284};

// Default token probabilities
static const jebp_ubyte jebp__default_token_probs
    [JEBP__NB_BLOCK_TYPES][JEBP__NB_COEFF_BANDS][JEBP__NB_TOKEN_COMPLEXITIES]
    [JEBP__NB_PROBS(JEBP__NB_TOKENS)] = {
        {{{128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
          {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
          {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128}},
         {{253, 136, 254, 255, 228, 219, 128, 128, 128, 128, 128},
          {189, 129, 242, 255, 227, 213, 255, 219, 128, 128, 128},
          {106, 126, 227, 252, 214, 209, 255, 255, 128, 128, 128}},
         {{1, 98, 248, 255, 236, 226, 255, 255, 128, 128, 128},
          {181, 133, 238, 254, 221, 234, 255, 154, 128, 128, 128},
          {78, 134, 202, 247, 198, 180, 255, 219, 128, 128, 128}},
         {{1, 185, 249, 255, 243, 255, 128, 128, 128, 128, 128},
          {184, 150, 247, 255, 236, 224, 128, 128, 128, 128, 128},
          {77, 110, 216, 255, 236, 230, 128, 128, 128, 128, 128}},
         {{1, 101, 251, 255, 241, 255, 128, 128, 128, 128, 128},
          {170, 139, 241, 252, 236, 209, 255, 255, 128, 128, 128},
          {37, 116, 196, 243, 228, 255, 255, 255, 128, 128, 128}},
         {{1, 204, 254, 255, 245, 255, 128, 128, 128, 128, 128},
          {207, 160, 250, 255, 238, 128, 128, 128, 128, 128, 128},
          {102, 103, 231, 255, 211, 171, 128, 128, 128, 128, 128}},
         {{1, 152, 252, 255, 240, 255, 128, 128, 128, 128, 128},
          {177, 135, 243, 255, 234, 225, 128, 128, 128, 128, 128},
          {80, 129, 211, 255, 194, 224, 128, 128, 128, 128, 128}},
         {{1, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
          {246, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
          {255, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128}}},
        {{{198, 35, 237, 223, 193, 187, 162, 160, 145, 155, 62},
          {131, 45, 198, 221, 172, 176, 220, 157, 252, 221, 1},
          {68, 47, 146, 208, 149, 167, 221, 162, 255, 223, 128}},
         {{1, 149, 241, 255, 221, 224, 255, 255, 128, 128, 128},
          {184, 141, 234, 253, 222, 220, 255, 199, 128, 128, 128},
          {81, 99, 181, 242, 176, 190, 249, 202, 255, 255, 128}},
         {{1, 129, 232, 253, 214, 197, 242, 196, 255, 255, 128},
          {99, 121, 210, 250, 201, 198, 255, 202, 128, 128, 128},
          {23, 91, 163, 242, 170, 187, 247, 210, 255, 255, 128}},
         {{1, 200, 246, 255, 234, 255, 128, 128, 128, 128, 128},
          {109, 178, 241, 255, 231, 245, 255, 255, 128, 128, 128},
          {44, 130, 201, 253, 205, 192, 255, 255, 128, 128, 128}},
         {{1, 132, 239, 251, 219, 209, 255, 165, 128, 128, 128},
          {94, 136, 225, 251, 218, 190, 255, 255, 128, 128, 128},
          {22, 100, 174, 245, 186, 161, 255, 199, 128, 128, 128}},
         {{1, 182, 249, 255, 232, 235, 128, 128, 128, 128, 128},
          {124, 143, 241, 255, 227, 234, 128, 128, 128, 128, 128},
          {35, 77, 181, 251, 193, 211, 255, 205, 128, 128, 128}},
         {{1, 157, 247, 255, 236, 231, 255, 255, 128, 128, 128},
          {121, 141, 235, 255, 225, 227, 255, 255, 128, 128, 128},
          {45, 99, 188, 251, 195, 217, 255, 224, 128, 128, 128}},
         {{1, 1, 251, 255, 213, 255, 128, 128, 128, 128, 128},
          {203, 1, 248, 255, 255, 128, 128, 128, 128, 128, 128},
          {137, 1, 177, 255, 224, 255, 128, 128, 128, 128, 128}}},
        {{{253, 9, 248, 251, 207, 208, 255, 192, 128, 128, 128},
          {175, 13, 224, 243, 193, 185, 249, 198, 255, 255, 128},
          {73, 17, 171, 221, 161, 179, 236, 167, 255, 234, 128}},
         {{1, 95, 247, 253, 212, 183, 255, 255, 128, 128, 128},
          {239, 90, 244, 250, 211, 209, 255, 255, 128, 128, 128},
          {155, 77, 195, 248, 188, 195, 255, 255, 128, 128, 128}},
         {{1, 24, 239, 251, 218, 219, 255, 205, 128, 128, 128},
          {201, 51, 219, 255, 196, 186, 128, 128, 128, 128, 128},
          {69, 46, 190, 239, 201, 218, 255, 228, 128, 128, 128}},
         {{1, 191, 251, 255, 255, 128, 128, 128, 128, 128, 128},
          {223, 165, 249, 255, 213, 255, 128, 128, 128, 128, 128},
          {141, 124, 248, 255, 255, 128, 128, 128, 128, 128, 128}},
         {{1, 16, 248, 255, 255, 128, 128, 128, 128, 128, 128},
          {190, 36, 230, 255, 236, 255, 128, 128, 128, 128, 128},
          {149, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128}},
         {{1, 226, 255, 128, 128, 128, 128, 128, 128, 128, 128},
          {247, 192, 255, 128, 128, 128, 128, 128, 128, 128, 128},
          {240, 128, 255, 128, 128, 128, 128, 128, 128, 128, 128}},
         {{1, 134, 252, 255, 255, 128, 128, 128, 128, 128, 128},
          {213, 62, 250, 255, 255, 128, 128, 128, 128, 128, 128},
          {55, 93, 255, 128, 128, 128, 128, 128, 128, 128, 128}},
         {{128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
          {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
          {128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128}}},
        {{{202, 24, 213, 235, 186, 191, 220, 160, 240, 175, 255},
          {126, 38, 182, 232, 169, 184, 228, 174, 255, 187, 128},
          {61, 46, 138, 219, 151, 178, 240, 170, 255, 216, 128}},
         {{1, 112, 230, 250, 199, 191, 247, 159, 255, 255, 128},
          {166, 109, 228, 252, 211, 215, 255, 174, 128, 128, 128},
          {39, 77, 162, 232, 172, 180, 245, 178, 255, 255, 128}},
         {{1, 52, 220, 246, 198, 199, 249, 220, 255, 255, 128},
          {124, 74, 191, 243, 183, 193, 250, 221, 255, 255, 128},
          {24, 71, 130, 219, 154, 170, 243, 182, 255, 255, 128}},
         {{1, 182, 225, 249, 219, 240, 255, 224, 128, 128, 128},
          {149, 150, 226, 252, 216, 205, 255, 171, 128, 128, 128},
          {28, 108, 170, 242, 183, 194, 254, 223, 255, 255, 128}},
         {{1, 81, 230, 252, 204, 203, 255, 192, 128, 128, 128},
          {123, 102, 209, 247, 188, 196, 255, 233, 128, 128, 128},
          {20, 95, 153, 243, 164, 173, 255, 203, 128, 128, 128}},
         {{1, 222, 248, 255, 216, 213, 128, 128, 128, 128, 128},
          {168, 175, 246, 252, 235, 205, 255, 255, 128, 128, 128},
          {47, 116, 215, 255, 211, 212, 255, 255, 128, 128, 128}},
         {{1, 121, 236, 253, 212, 214, 255, 255, 128, 128, 128},
          {141, 84, 213, 252, 201, 202, 255, 219, 128, 128, 128},
          {42, 80, 160, 240, 162, 185, 255, 205, 128, 128, 128}},
         {{1, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
          {244, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
          {238, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128}}}};

// Probabilities to update specific token
static const jebp_ubyte jebp__update_token_probs
    [JEBP__NB_BLOCK_TYPES][JEBP__NB_COEFF_BANDS][JEBP__NB_TOKEN_COMPLEXITIES]
    [JEBP__NB_PROBS(JEBP__NB_TOKENS)] = {
        {{{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{176, 246, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {223, 241, 252, 255, 255, 255, 255, 255, 255, 255, 255},
          {249, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 244, 252, 255, 255, 255, 255, 255, 255, 255, 255},
          {234, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 246, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {239, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {254, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 248, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {251, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {251, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {254, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 254, 253, 255, 254, 255, 255, 255, 255, 255, 255},
          {250, 255, 254, 255, 254, 255, 255, 255, 255, 255, 255},
          {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}}},
        {{{217, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {225, 252, 241, 253, 255, 255, 254, 255, 255, 255, 255},
          {234, 250, 241, 250, 253, 255, 253, 254, 255, 255, 255}},
         {{255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {223, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {238, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 248, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {249, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {247, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 254, 253, 255, 255, 255, 255, 255, 255, 255, 255},
          {250, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}}},
        {{{186, 251, 250, 255, 255, 255, 255, 255, 255, 255, 255},
          {234, 251, 244, 254, 255, 255, 255, 255, 255, 255, 255},
          {251, 251, 243, 253, 254, 255, 254, 255, 255, 255, 255}},
         {{255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {236, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {251, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255}},
         {{255, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}}},
        {{{248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {250, 254, 252, 254, 255, 255, 255, 255, 255, 255, 255},
          {248, 254, 249, 253, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255},
          {246, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255},
          {252, 254, 251, 254, 254, 255, 255, 255, 255, 255, 255}},
         {{255, 254, 252, 255, 255, 255, 255, 255, 255, 255, 255},
          {248, 254, 253, 255, 255, 255, 255, 255, 255, 255, 255},
          {253, 255, 254, 254, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {245, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {253, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 251, 253, 255, 255, 255, 255, 255, 255, 255, 255},
          {252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {249, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 255, 253, 255, 255, 255, 255, 255, 255, 255, 255},
          {250, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}},
         {{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
          {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}}}};

// The decoding tree for the segment ID
static const jebp_byte jebp__segment_tree[JEBP__NB_TREE(JEBP__NB_SEGMENTS)] = {
    2, 4, -0, -1, -2, -3};

// The decoding tree for the Y prediction mode
static const jebp_byte jebp__y_pred_tree[JEBP__NB_TREE(JEBP__NB_Y_PRED_TYPES)] =
    {-JEBP__VP8_PRED_B,
     2,
     4,
     6,
     -JEBP__VP8_PRED_DC,
     -JEBP__VP8_PRED_V,
     -JEBP__VP8_PRED_H,
     -JEBP__VP8_PRED_TM};

// The fixed probabilities for the Y prediction mode
static const jebp_ubyte jebp__y_pred_probs[JEBP__NB_PROBS(
    JEBP__NB_Y_PRED_TYPES)] = {145, 156, 163, 128};

// The decoding tree for the Y subblock modes (when the prediction mode is B)
static const jebp_byte jebp__b_pred_tree[JEBP__NB_TREE(JEBP__NB_B_PRED_TYPES)] =
    {-JEBP__B_PRED_DC,
     2,
     -JEBP__B_PRED_TM,
     4,
     -JEBP__B_PRED_VE,
     6,
     8,
     12,
     -JEBP__B_PRED_HE,
     10,
     -JEBP__B_PRED_RD,
     -JEBP__B_PRED_VR,
     -JEBP__B_PRED_LD,
     14,
     -JEBP__B_PRED_VL,
     16,
     -JEBP__B_PRED_HD,
     -JEBP__B_PRED_HU};

// The fixed probabilities for the Y subblock modes based on nearby subblock
// modes
static const jebp_ubyte
    jebp__b_pred_probs[JEBP__NB_B_PRED_TYPES][JEBP__NB_B_PRED_TYPES]
                      [JEBP__NB_PROBS(JEBP__NB_B_PRED_TYPES)] = {
                          {{231, 120, 48, 89, 115, 113, 120, 152, 112},
                           {152, 179, 64, 126, 170, 118, 46, 70, 95},
                           {175, 69, 143, 80, 85, 82, 72, 155, 103},
                           {56, 58, 10, 171, 218, 189, 17, 13, 152},
                           {144, 71, 10, 38, 171, 213, 144, 34, 26},
                           {114, 26, 17, 163, 44, 195, 21, 10, 173},
                           {121, 24, 80, 195, 26, 62, 44, 64, 85},
                           {170, 46, 55, 19, 136, 160, 33, 206, 71},
                           {63, 20, 8, 114, 114, 208, 12, 9, 226},
                           {81, 40, 11, 96, 182, 84, 29, 16, 36}},
                          {{134, 183, 89, 137, 98, 101, 106, 165, 148},
                           {72, 187, 100, 130, 157, 111, 32, 75, 80},
                           {66, 102, 167, 99, 74, 62, 40, 234, 128},
                           {41, 53, 9, 178, 241, 141, 26, 8, 107},
                           {104, 79, 12, 27, 217, 255, 87, 17, 7},
                           {74, 43, 26, 146, 73, 166, 49, 23, 157},
                           {65, 38, 105, 160, 51, 52, 31, 115, 128},
                           {87, 68, 71, 44, 114, 51, 15, 186, 23},
                           {47, 41, 14, 110, 182, 183, 21, 17, 194},
                           {66, 45, 25, 102, 197, 189, 23, 18, 22}},
                          {{88, 88, 147, 150, 42, 46, 45, 196, 205},
                           {43, 97, 183, 117, 85, 38, 35, 179, 61},
                           {39, 53, 200, 87, 26, 21, 43, 232, 171},
                           {56, 34, 51, 104, 114, 102, 29, 93, 77},
                           {107, 54, 32, 26, 51, 1, 81, 43, 31},
                           {39, 28, 85, 171, 58, 165, 90, 98, 64},
                           {34, 22, 116, 206, 23, 34, 43, 166, 73},
                           {68, 25, 106, 22, 64, 171, 36, 225, 114},
                           {34, 19, 21, 102, 132, 188, 16, 76, 124},
                           {62, 18, 78, 95, 85, 57, 50, 48, 51}},
                          {{193, 101, 35, 159, 215, 111, 89, 46, 111},
                           {60, 148, 31, 172, 219, 228, 21, 18, 111},
                           {112, 113, 77, 85, 179, 255, 38, 120, 114},
                           {40, 42, 1, 196, 245, 209, 10, 25, 109},
                           {100, 80, 8, 43, 154, 1, 51, 26, 71},
                           {88, 43, 29, 140, 166, 213, 37, 43, 154},
                           {61, 63, 30, 155, 67, 45, 68, 1, 209},
                           {142, 78, 78, 16, 255, 128, 34, 197, 171},
                           {41, 40, 5, 102, 211, 183, 4, 1, 221},
                           {51, 50, 17, 168, 209, 192, 23, 25, 82}},
                          {{125, 98, 42, 88, 104, 85, 117, 175, 82},
                           {95, 84, 53, 89, 128, 100, 113, 101, 45},
                           {75, 79, 123, 47, 51, 128, 81, 171, 1},
                           {57, 17, 5, 71, 102, 57, 53, 41, 49},
                           {115, 21, 2, 10, 102, 255, 166, 23, 6},
                           {38, 33, 13, 121, 57, 73, 26, 1, 85},
                           {41, 10, 67, 138, 77, 110, 90, 47, 114},
                           {101, 29, 16, 10, 85, 128, 101, 196, 26},
                           {57, 18, 10, 102, 102, 213, 34, 20, 43},
                           {117, 20, 15, 36, 163, 128, 68, 1, 26}},
                          {{138, 31, 36, 171, 27, 166, 38, 44, 229},
                           {67, 87, 58, 169, 82, 115, 26, 59, 179},
                           {63, 59, 90, 180, 59, 166, 93, 73, 154},
                           {40, 40, 21, 116, 143, 209, 34, 39, 175},
                           {57, 46, 22, 24, 128, 1, 54, 17, 37},
                           {47, 15, 16, 183, 34, 223, 49, 45, 183},
                           {46, 17, 33, 183, 6, 98, 15, 32, 183},
                           {65, 32, 73, 115, 28, 128, 23, 128, 205},
                           {40, 3, 9, 115, 51, 192, 18, 6, 223},
                           {87, 37, 9, 115, 59, 77, 64, 21, 47}},
                          {{104, 55, 44, 218, 9, 54, 53, 130, 226},
                           {64, 90, 70, 205, 40, 41, 23, 26, 57},
                           {54, 57, 112, 184, 5, 41, 38, 166, 213},
                           {30, 34, 26, 133, 152, 116, 10, 32, 134},
                           {75, 32, 12, 51, 192, 255, 160, 43, 51},
                           {39, 19, 53, 221, 26, 114, 32, 73, 255},
                           {31, 9, 65, 234, 2, 15, 1, 118, 73},
                           {88, 31, 35, 67, 102, 85, 55, 186, 85},
                           {56, 21, 23, 111, 59, 205, 45, 37, 192},
                           {55, 38, 70, 124, 73, 102, 1, 34, 98}},
                          {{102, 61, 71, 37, 34, 53, 31, 243, 192},
                           {69, 60, 71, 38, 73, 119, 28, 222, 37},
                           {68, 45, 128, 34, 1, 47, 11, 245, 171},
                           {62, 17, 19, 70, 146, 85, 55, 62, 70},
                           {75, 15, 9, 9, 64, 255, 184, 119, 16},
                           {37, 43, 37, 154, 100, 163, 85, 160, 1},
                           {63, 9, 92, 136, 28, 64, 32, 201, 85},
                           {86, 6, 28, 5, 64, 255, 25, 248, 1},
                           {56, 8, 17, 132, 137, 255, 55, 116, 128},
                           {58, 15, 20, 82, 135, 57, 26, 121, 40}},
                          {{164, 50, 31, 137, 154, 133, 25, 35, 218},
                           {51, 103, 44, 131, 131, 123, 31, 6, 158},
                           {86, 40, 64, 135, 148, 224, 45, 183, 128},
                           {22, 26, 17, 131, 240, 154, 14, 1, 209},
                           {83, 12, 13, 54, 192, 255, 68, 47, 28},
                           {45, 16, 21, 91, 64, 222, 7, 1, 197},
                           {56, 21, 39, 155, 60, 138, 23, 102, 213},
                           {85, 26, 85, 85, 128, 128, 32, 146, 171},
                           {18, 11, 7, 63, 144, 171, 4, 4, 246},
                           {35, 27, 10, 146, 174, 171, 12, 26, 128}},
                          {{190, 80, 35, 99, 180, 80, 126, 54, 45},
                           {85, 126, 47, 87, 176, 51, 41, 20, 32},
                           {101, 75, 128, 139, 118, 146, 116, 128, 85},
                           {56, 41, 15, 176, 236, 85, 37, 9, 62},
                           {146, 36, 19, 30, 171, 255, 97, 27, 20},
                           {71, 30, 17, 119, 118, 255, 17, 18, 138},
                           {101, 38, 60, 138, 55, 70, 43, 26, 142},
                           {138, 45, 61, 62, 219, 1, 81, 188, 64},
                           {32, 41, 20, 117, 151, 142, 20, 21, 163},
                           {112, 19, 12, 61, 195, 128, 48, 4, 24}}};

// The decoding tree for the UV prediction mode
static const jebp_byte
    jebp__uv_pred_tree[JEBP__NB_TREE(JEBP__NB_UV_PRED_TYPES)] = {
        -JEBP__VP8_PRED_DC, 2, -JEBP__VP8_PRED_V, 4, -JEBP__VP8_PRED_H,
        -JEBP__VP8_PRED_TM};

// The fixed probabilities for the UV prediction mode
static const jebp_ubyte jebp__uv_pred_probs[JEBP__NB_PROBS(
    JEBP__NB_UV_PRED_TYPES)] = {142, 114, 183};

// Which bands each coefficient goes into for token complexities
static const jebp_byte jebp__coeff_bands[JEBP__NB_BLOCK_COEFFS] = {
    0, 1, 2, 3, 6, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7};

// The zig-zag order of the coefficients
//   [0]= 0  [1]= 1  [5]= 2  [6]= 3
//   [2]= 4  [4]= 5  [7]= 6 [12]= 7
//   [3]= 8  [8]= 9 [11]=10 [13]=11
//   [9]=12 [10]=13 [14]=14 [15]=15
static const jebp_byte jebp__coeff_order[JEBP__NB_BLOCK_COEFFS] = {
    0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15};

// The fixed tree for token decoding, using the probabilities defined in the
// header. This doesn't include the EOB branch at the start since that may be
// skipped.
static const jebp_byte jebp__token_tree[JEBP__NB_TREE(JEBP__NB_TOKENS - 1)] = {
    -JEBP__TOKEN_COEFF0,
    2,
    -JEBP__TOKEN_COEFF1,
    4,
    6,
    10,
    -JEBP__TOKEN_COEFF2,
    8,
    -JEBP__TOKEN_COEFF3,
    -JEBP__TOKEN_COEFF4,
    12,
    14,
    -JEBP__TOKEN_EXTRA1,
    -JEBP__TOKEN_EXTRA2,
    16,
    18,
    -JEBP__TOKEN_EXTRA3,
    -JEBP__TOKEN_EXTRA4,
    -JEBP__TOKEN_EXTRA5,
    -JEBP__TOKEN_EXTRA6};

static const jebp__token_extra_t jebp__token_extra[JEBP__NB_EXTRA_TOKENS] = {
    {5, {159, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {7, {165, 145, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {11, {173, 148, 140, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    {19, {176, 155, 140, 135, 0, 0, 0, 0, 0, 0, 0, 0}},
    {35, {180, 157, 141, 134, 130, 0, 0, 0, 0, 0, 0, 0}},
    {67, {254, 254, 243, 230, 196, 177, 153, 140, 133, 130, 129, 0}},
};
#endif // JEBP_NO_VP8

#ifndef JEBP_NO_VP8L
// The order that meta lengths are read
static const jebp_byte jebp__meta_length_order[JEBP__NB_META_SYMBOLS] = {
    17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

// {X, Y} offsets from the pixel when decoding short distance codes
static const jebp_byte jebp__vp8l_offsets[JEBP__NB_VP8L_OFFSETS][2] = {
    {0, 1},  {1, 0},  {1, 1},  {-1, 1}, {0, 2},  {2, 0},  {1, 2},  {-1, 2},
    {2, 1},  {-2, 1}, {2, 2},  {-2, 2}, {0, 3},  {3, 0},  {1, 3},  {-1, 3},
    {3, 1},  {-3, 1}, {2, 3},  {-2, 3}, {3, 2},  {-3, 2}, {0, 4},  {4, 0},
    {1, 4},  {-1, 4}, {4, 1},  {-4, 1}, {3, 3},  {-3, 3}, {2, 4},  {-2, 4},
    {4, 2},  {-4, 2}, {0, 5},  {3, 4},  {-3, 4}, {4, 3},  {-4, 3}, {5, 0},
    {1, 5},  {-1, 5}, {5, 1},  {-5, 1}, {2, 5},  {-2, 5}, {5, 2},  {-5, 2},
    {4, 4},  {-4, 4}, {3, 5},  {-3, 5}, {5, 3},  {-5, 3}, {0, 6},  {6, 0},
    {1, 6},  {-1, 6}, {6, 1},  {-6, 1}, {2, 6},  {-2, 6}, {6, 2},  {-6, 2},
    {4, 5},  {-4, 5}, {5, 4},  {-5, 4}, {3, 6},  {-3, 6}, {6, 3},  {-6, 3},
    {0, 7},  {7, 0},  {1, 7},  {-1, 7}, {5, 5},  {-5, 5}, {7, 1},  {-7, 1},
    {4, 6},  {-4, 6}, {6, 4},  {-6, 4}, {2, 7},  {-2, 7}, {7, 2},  {-7, 2},
    {3, 7},  {-3, 7}, {7, 3},  {-7, 3}, {5, 6},  {-5, 6}, {6, 5},  {-6, 5},
    {8, 0},  {4, 7},  {-4, 7}, {7, 4},  {-7, 4}, {8, 1},  {8, 2},  {6, 6},
    {-6, 6}, {8, 3},  {5, 7},  {-5, 7}, {7, 5},  {-7, 5}, {8, 4},  {6, 7},
    {-6, 7}, {7, 6},  {-7, 6}, {8, 5},  {7, 7},  {-7, 7}, {8, 6},  {8, 7}};
#endif // JEBP_NO_VP8L

// Error strings to return from jebp_error_string
static const char *const jebp__error_strings[JEBP_NB_ERRORS] = {
    "Ok",
    "Invalid value or argument",
    "Invalid data or corrupted file",
    "Invalid WebP header or corrupted file",
    "End of file",
    "Feature not supported",
    "Codec not supported",
    "Color-indexing or palettes are not supported",
    "Not enough memory",
    "I/O error",
    "Unknown error"};
#endif // JEBP_IMPLEMENTATION
