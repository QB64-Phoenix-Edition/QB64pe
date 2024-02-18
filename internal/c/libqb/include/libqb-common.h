#ifndef INCLUDE_LIBQB_LIBQB_COMMON_H
#define INCLUDE_LIBQB_LIBQB_COMMON_H
// Should be included at the top of every .cpp file

/* Provide some OS/compiler macros.
 * QB64_WINDOWS: Is this a Windows system?
 * QB64_LINUX: Is this a Linux system?
 * QB64_MACOSX: Is this MacOSX, or MacOS or whatever Apple calls it now?
 * QB64_UNIX: Is this a Unix-flavoured system?
 *
 * QB64_BACKSLASH_FILESYSTEM: Does this system use \ for file paths (as opposed to /)?
 * QB64_MICROSOFT: Are we compiling with Visual Studio?
 * QB64_GCC: Are we compiling with gcc?
 * QB64_MINGW: Are we compiling with MinGW, specifically? (Set in addition to QB64_GCC)
 *
 * QB64_32: A 32bit system (the default)
 * QB64_64: A 64bit system (assumes all Macs are 64 bit)
 */
#ifdef WIN32
#    define QB64_WINDOWS
#    ifndef _WIN32_WINNT
// This supports Windows Vista and up
#        define _WIN32_WINNT 0x0600
#        define WINVER 0x0600
#    endif

#    define QB64_BACKSLASH_FILESYSTEM
#    ifdef _MSC_VER
// Do we even support non-mingw compilers on Windows?
#        define QB64_MICROSOFT
#    else
#        define QB64_GCC
#        define QB64_MINGW
#    endif
#elif defined(__APPLE__)
#    define QB64_MACOSX
#    define QB64_UNIX
#    define QB64_GCC
#elif defined(__linux__)
#    define QB64_LINUX
#    define QB64_UNIX
#    define QB64_GCC
#else
#    error "Unknown system; refusing to build. Edit os.h if needed"
#endif

#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__) || defined(QB64_MACOSX) || defined(__aarch64__)
#    define QB64_64
#else
#    define QB64_32
#endif

#if !defined(i386) && !defined(__x86_64__)
#    define QB64_NOT_X86
#    if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64)
#        define QB64_ARM
#    endif
#endif

#define QB_FALSE 0
#define QB_TRUE -1

#endif
