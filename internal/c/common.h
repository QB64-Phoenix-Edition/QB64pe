// Fill out dependency macros
#if !defined(DEPENDENCY_NO_SOCKETS) && !defined(DEPENDENCY_SOCKETS)
#    define DEPENDENCY_SOCKETS
#endif

#if !defined(DEPENDENCY_NO_PRINTER) && !defined(DEPENDENCY_PRINTER)
#    define DEPENDENCY_PRINTER
#endif

#if !defined(DEPENDENCY_NO_ICON) && !defined(DEPENDENCY_ICON)
#    define DEPENDENCY_ICON
#endif

#if !defined(DEPENDENCY_NO_SCREENIMAGE) && !defined(DEPENDENCY_SCREENIMAGE)
#    define DEPENDENCY_SCREENIMAGE
#endif

#ifndef INC_COMMON_CPP
#    define INC_COMMON_CPP
#    include "os.h"

#    define QB64_GL1
#    define QB64_GLUT

#    ifdef DEPENDENCY_CONSOLE_ONLY
#        undef QB64_GLUT
#    else
#        define QB64_GUI
#    endif

// core
#    ifdef QB64_GUI
#        ifdef QB64_GLUT
#            include <GL/glew.h>
#            ifdef QB64_MACOSX
// note: MacOSX uses Apple's GLUT not FreeGLUT
#                include <GLUT/glut.h>
#                include <OpenGL/gl.h>
#                include <OpenGL/glext.h>
#                include <OpenGL/glu.h>
#            else
#                define CORE_FREEGLUT
#                include <GL/freeglut.h>
#                include <GL/glext.h>
#            endif
#        endif
#    endif

#    ifdef QB64_WINDOWS

#        ifndef QB64_GUI
#            undef int64 // definition of int64 from os.h conflicts with a definition within windows.h, temporarily undefine then redefine
#            include <windows.h>
#            define int64 __int64
#        endif

#        include <shfolder.h>

#        include <float.h>
#        include <winbase.h>

#    endif

// common includes
#    include <cmath>
#    include <errno.h>
#    include <fcntl.h>
#    include <fstream>
#    include <inttypes.h>
#    include <iostream>
#    include <limits.h>
#    include <stdint.h>
#    include <stdio.h>
#    include <string.h>
#    include <time.h>

// OS/compiler specific includes
#    ifdef QB64_WINDOWS
#        include <direct.h>
#        ifdef DEPENDENCY_PRINTER
#            include <winspool.h>
#        endif
#        include <csignal>
#        include <process.h> //required for multi-threading
#        if defined DEPENDENCY_AUDIO_MINIAUDIO || defined QB64_GUI
#            include <mmsystem.h>
#        endif

#    else

#        include <dlfcn.h>
#        include <pthread.h>
#        include <stdlib.h>
#        include <sys/stat.h>
#        include <sys/types.h>
#        include <sys/wait.h>
#        include <unistd.h>

#    endif

#    include "error_handle.h"
#    include "qbs.h"

struct img_struct {
    void *lock_offset;
    int64 lock_id;
    uint8 valid;   // 0,1 0=invalid
    uint8 text;    // if set, surface is a text surface
    uint8 console; // dummy surface to absorb unimplemented console functionality
    uint16 width, height;
    uint8 bytes_per_pixel;  // 1,2,4
    uint8 bits_per_pixel;   // 1,2,4,8,16(text),32
    uint32 mask;            // 1,3,0xF,0xFF,0xFFFF,0xFFFFFFFF
    uint16 compatible_mode; // 0,1,2,7,8,9,10,11,12,13,32,256
    uint32 color, background_color, draw_color;
    uint32 font;               // 8,14,16,?
    int16 top_row, bottom_row; // VIEW PRINT settings, unique (as in QB) to each "page"
    int16 cursor_x, cursor_y;  // unique (as in QB) to each "page"
    uint8 cursor_show, cursor_firstvalue, cursor_lastvalue;
    union {
        uint8 *offset;
        uint32 *offset32;
    };
    uint32 flags;
    uint32 *pal;
    int32 transparent_color; //-1 means no color is transparent
    uint8 alpha_disabled;
    uint8 holding_cursor;
    uint8 print_mode;
    // BEGIN apm ('active page migration')
    // everything between apm points is migrated during active page changes
    // note: apm data is only relevant to graphics modes
    uint8 apm_p1;
    int32 view_x1, view_y1, view_x2, view_y2;
    int32 view_offset_x, view_offset_y;
    float x, y;
    uint8 clipping_or_scaling;
    float scaling_x, scaling_y, scaling_offset_x, scaling_offset_y;
    float window_x1, window_y1, window_x2, window_y2;
    double draw_ta;
    double draw_scale;
    uint8 apm_p2;
    // END apm
};
// img_struct flags
#    define IMG_FREEPAL 1 // free palette data before freeing image
#    define IMG_SCREEN 2  // img is linked to other screen pages
#    define IMG_FREEMEM 4 // if set, it means memory must be freed

// QB64 internal variable type flags (internally referenced by some C functions)
#    define ISSTRING 1073741824
#    define ISFLOAT 536870912
#    define ISUNSIGNED 268435456
#    define ISPOINTER 134217728
#    define ISFIXEDLENGTH 67108864 // only set for strings with pointer flag
#    define ISINCONVENTIONALMEMORY 33554432
#    define ISOFFSETINBITS 16777216

struct ontimer_struct {
    uint8 allocated;
    uint32 id;        // the event ID to trigger (0=no event)
    int64 pass;       // the value to pass to the triggered event (only applicable to ON ... CALL ...(x)
    uint8 active;     // 0=OFF, 1=ON, 2=STOP
    uint8 state;      // 0=untriggered,1=triggered
    double seconds;   // how many seconds between events
    double last_time; // the last time this event was triggered
};

struct onkey_struct {
    uint32 id;                // the event ID to trigger (0=no event)
    int64 pass;               // the value to pass to the triggered event (only applicable to ON ... CALL ...(x)
    uint8 active;             // 0=OFF, 1=ON, 2=STOP
    uint8 state;              // 0=untriggered,1=triggered,2=in progress(TIMER only),2+=multiple events queued(KEY only)
    uint32 keycode;           // 32-bit code, same as what _KEYHIT returns
    uint32 keycode_alternate; // an alternate keycode which may also trigger event
    uint8 key_scancode;
    uint8 key_flags;
    // flags:
    // 0 No keyboard flag, 1-3 Either Shift key, 4 Ctrl key, 8 Alt key,32 NumLock key,64 Caps Lock key, 128 Extended keys on a 101-key keyboard
    // To specify multiple shift states, add the values together. For example, a value of 12 specifies that the user-defined key is used in combination with the
    // Ctrl and Alt keys.
    qbs *text;
};

struct byte_element_struct {
    uint64 offset;
    int32 length;
};

#    include "mem.h"

#endif // INC_COMMON_CPP
