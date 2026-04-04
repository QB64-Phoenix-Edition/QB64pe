#pragma once

#include "glut-emu.h"

// Called to potentially setup GLUT before starting the program.
void libqb_glut_presetup();

// Starts the "main thread", including handling all the GLUT setup.
void libqb_start_main_thread();

// Used to support _ScreenShow, which can start the GLUT thread after the
// program is started
void libqb_start_glut_thread();

// Indicates whether GLUT is currently running (and thus whether we're able to
// do any GLUT-related stuff
bool libqb_is_glut_up();

// Called to properly exit the program. Necessary because GLUT requires a
// special care to not seg-fault when exiting the program.
void libqb_exit(int);

// Convenience macros, exists a function depending on the state of GLUT
#define NEEDS_GLUT(error_result)                                                                                                                               \
    do {                                                                                                                                                       \
        if (!libqb_is_glut_up()) {                                                                                                                             \
            error(5);                                                                                                                                          \
            return error_result;                                                                                                                               \
        }                                                                                                                                                      \
    } while (0)

#define OPTIONAL_GLUT(result)                                                                                                                                  \
    do {                                                                                                                                                       \
        if (!libqb_is_glut_up())                                                                                                                               \
            return result;                                                                                                                                     \
    } while (0)
