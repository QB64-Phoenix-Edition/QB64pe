
libqb-objs-y += $(PATH_LIBQB)/src/threading.o

libqb-objs-y += $(PATH_LIBQB)/src/threading-$(PLATFORM).o

CLEAN_LIST += $(libqb-objs-y)
