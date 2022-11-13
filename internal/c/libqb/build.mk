
libqb-objs-y += $(PATH_LIBQB)/src/threading.o
libqb-objs-y += $(PATH_LIBQB)/src/buffer.o
libqb-objs-y += $(PATH_LIBQB)/src/filepath.o

libqb-objs-$(DEP_HTTP) += $(PATH_LIBQB)/src/http.o
libqb-objs-y$(DEP_HTTP) += $(PATH_LIBQB)/src/http-stub.o

libqb-objs-y += $(PATH_LIBQB)/src/threading-$(PLATFORM).o

CLEAN_LIST += $(libqb-objs-y) $(libqb-objs-yy) $(libqb-objs-)
