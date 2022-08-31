
#include "libqb-common.h"

/* common types (not quite an include guard, but allows an including
 * file to not have these included.
 *
 * Should this be adapted to check for each type before defining?
 */
#ifndef QB64_OS_H_NO_TYPES
#    ifdef QB64_WINDOWS
#        define uint64 unsigned __int64
#        define uint32 unsigned __int32
#        define uint16 unsigned __int16
#        define uint8 unsigned __int8
#        define int64 __int64
#        define int32 __int32
#        define int16 __int16
#        define int8 __int8
#    else
#        define int64 int64_t
#        define int32 int32_t
#        define int16 int16_t
#        define int8 int8_t
#        define uint64 uint64_t
#        define uint32 uint32_t
#        define uint16 uint16_t
#        define uint8 uint8_t
#    endif

#    define ptrszint intptr_t
#    define uptrszint uintptr_t

#    ifdef QB64_64
#        define ptrsz 8
#    else
#        define ptrsz 4
#    endif
#endif
