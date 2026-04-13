
libqb-objs-y += $(PATH_LIBQB)/src/threading.o
libqb-objs-y += $(PATH_LIBQB)/src/buffer.o
libqb-objs-y += $(PATH_LIBQB)/src/command.o
libqb-objs-y += $(PATH_LIBQB)/src/environ.o
libqb-objs-y += $(PATH_LIBQB)/src/file-fields.o
libqb-objs-y += $(PATH_LIBQB)/src/filepath.o
libqb-objs-y += $(PATH_LIBQB)/src/filesystem.o
libqb-objs-y += $(PATH_LIBQB)/src/datetime.o
libqb-objs-y += $(PATH_LIBQB)/src/error_handle.o
libqb-objs-y += $(PATH_LIBQB)/src/gfs.o
libqb-objs-y += $(PATH_LIBQB)/src/qblist.o
libqb-objs-y += $(PATH_LIBQB)/src/hexoctbin.o
libqb-objs-y += $(PATH_LIBQB)/src/keyboard.o
libqb-objs-y += $(PATH_LIBQB)/src/key-events.o
libqb-objs-y += $(PATH_LIBQB)/src/memblock.o
libqb-objs-y += $(PATH_LIBQB)/src/shell.o
libqb-objs-y += $(PATH_LIBQB)/src/qbs.o
libqb-objs-y += $(PATH_LIBQB)/src/qbs_str.o
libqb-objs-y += $(PATH_LIBQB)/src/qbs__tostr.o
libqb-objs-y += $(PATH_LIBQB)/src/qbs_cmem.o
libqb-objs-y += $(PATH_LIBQB)/src/qbs_mk_cv.o
libqb-objs-y += $(PATH_LIBQB)/src/qbs_val.o
libqb-objs-y += $(PATH_LIBQB)/src/string_functions.o
libqb-objs-y += $(PATH_LIBQB)/src/graphics.o
libqb-objs-y += $(PATH_LIBQB)/src/glut-emu.o
libqb-objs-y$(DEP_CONSOLE_ONLY) += $(PATH_LIBQB)/src/window-gui.o
libqb-objs-$(DEP_CONSOLE_ONLY) += $(PATH_LIBQB)/src/window-console.o

libqb-objs-y += $(PATH_LIBQB)/src/logging/logging.o
libqb-objs-y += $(PATH_LIBQB)/src/logging/qb64pe_symbol.o
libqb-objs-y += $(PATH_LIBQB)/src/logging/stacktrace.o
libqb-objs-y += $(PATH_LIBQB)/src/logging/handlers/fp_handler.o

# Windows MinGW symbol resolution
libqb-objs-$(win) += $(PATH_LIBQB)/src/logging/mingw/file.o
libqb-objs-$(win) += $(PATH_LIBQB)/src/logging/mingw/pe.o
libqb-objs-$(win) += $(PATH_LIBQB)/src/logging/mingw/pe_symtab.o
libqb-objs-$(win) += $(PATH_LIBQB)/src/logging/mingw/symbol.o

# Unix symbol resolution
libqb-objs-$(unix) += $(PATH_LIBQB)/src/logging/unix/symbol.o

libqb-objs-$(DEP_HTTP) += $(PATH_LIBQB)/src/qb_http.o
libqb-objs-y$(DEP_HTTP) += $(PATH_LIBQB)/src/qb_http-stub.o

libqb-objs-y += $(PATH_LIBQB)/src/threading-$(PLATFORM).o

libqb-objs-y$(DEP_CONSOLE_ONLY) += $(PATH_LIBQB)/src/main-thread-gui.o
libqb-objs-$(DEP_CONSOLE_ONLY) += $(PATH_LIBQB)/src/main-thread-console.o

$(PATH_LIBQB)/src/%.o: $(PATH_LIBQB)/src/%.cpp
	$(CXX) -O3 $(CXXFLAGS) -Wall -Wextra $< -c -o $@

CLEAN_LIST += $(libqb-objs-y) $(libqb-objs-yy) $(libqb-objs-)
