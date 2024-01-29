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

// Called at consistent intervals from a GLUT callback
void libqb_process_glut_queue();

// Called to properly exit the program. Necessary because GLUT requires a
// special care to not seg-fault when exiting the program.
void libqb_exit(int);

// These functions perform the same actions as their corresponding glut* functions.
// They tell the GLUT thread to perform the command, returning the result if applicable
void libqb_glut_set_cursor(int style);
void libqb_glut_warp_pointer(int x, int y);
int libqb_glut_get(int id);
void libqb_glut_iconify_window();
void libqb_glut_position_window(int x, int y);
void libqb_glut_show_window();
void libqb_glut_hide_window();
void libqb_glut_set_window_title(const char *title);
void libqb_glut_exit_program(int exitcode);

// Convenience macros, exists a function depending on the state of GLUT
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
