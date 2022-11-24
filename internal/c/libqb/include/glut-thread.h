#ifndef INCLUDE_LIBQB_GLUT_THREAD_H
#define INCLUDE_LIBQB_GLUT_THREAD_H

// Called to potentially setup GLUT before starting the program.
void libqb_glut_presetup(int argc, char **argv);

// Starts the "main thread", including handling all the GLUT setup.
void libqb_start_main_thread(int argc, char **argv);

// Used to support _ScreenShow, which can start the GLUT thread after the
// program is started
void libqb_start_glut_thread();

// Indicates whether GLUT is currently running (and thus whether we're able to
// do any GLUT-related stuff
bool libqb_is_glut_up();

// Convinence macros, exists a function depending on the state of GLUT
#define NEEDS_GLUT(error_result) do { \
        if (!libqb_is_glut_up()) { \
            error(5); \
            return error_result; \
        } \
    } while (0)

#define OPTIONAL_GLUT(result) do { \
        if (!libqb_is_glut_up()) \
            return result; \
    } while (0)

#endif
