
libqb-objs-y += $(PATH_LIBQB)/src/threading.o
libqb-objs-y += $(PATH_LIBQB)/src/buffer.o
libqb-objs-y += $(PATH_LIBQB)/src/bitops.o
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
libqb-objs-y += $(PATH_LIBQB)/src/mem.o
libqb-objs-y += $(PATH_LIBQB)/src/math.o
libqb-objs-y += $(PATH_LIBQB)/src/rounding.o
libqb-objs-y += $(PATH_LIBQB)/src/shell.o
libqb-objs-y += $(PATH_LIBQB)/src/qbs.o
libqb-objs-y += $(PATH_LIBQB)/src/qbs_str.o
libqb-objs-y += $(PATH_LIBQB)/src/qbs_cmem.o
libqb-objs-y += $(PATH_LIBQB)/src/qbs_mk_cv.o
libqb-objs-y += $(PATH_LIBQB)/src/string_functions.o

libqb-objs-$(DEP_HTTP) += $(PATH_LIBQB)/src/http.o
libqb-objs-y$(DEP_HTTP) += $(PATH_LIBQB)/src/http-stub.o

libqb-objs-y += $(PATH_LIBQB)/src/threading-$(PLATFORM).o

libqb-objs-y$(DEP_CONSOLE_ONLY) += $(PATH_LIBQB)/src/glut-main-thread.o
libqb-objs-y$(DEP_CONSOLE_ONLY) += $(PATH_LIBQB)/src/glut-message.o
libqb-objs-y$(DEP_CONSOLE_ONLY) += $(PATH_LIBQB)/src/glut-msg-queue.o

libqb-objs-$(DEP_CONSOLE_ONLY) += $(PATH_LIBQB)/src/console-only-main-thread.o

ifeq ($(OS),osx)
libqb-objs-y$(DEP_CONSOLE_ONLY) += $(PATH_LIBQB)/src/mac-key-monitor.o $(PATH_LIBQB)/src/mac-mouse-support.o
endif

$(PATH_LIBQB)/src/%.o: $(PATH_LIBQB)/src/%.cpp
	$(CXX) -O2 $(CXXFLAGS) -Wall -Wextra $< -c -o $@

ifeq ($(OS),osx)
$(PATH_LIBQB)/src/%.o: $(PATH_LIBQB)/src/%.mm
	$(CXX) -O2 $(CXXFLAGS) -Wall -Wextra $< -c -o $@
endif

CLEAN_LIST += $(libqb-objs-y) $(libqb-objs-yy) $(libqb-objs-)
