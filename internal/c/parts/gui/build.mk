
GUI_SRCS := \
	gui.cpp

TFD_SRCS := \
	tinyfiledialogs.c

GUI_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/gui/%.o,$(GUI_SRCS))

TFD_OBJS := $(patsubst %.c,$(PATH_INTERNAL_C)/parts/gui/%.o,$(TFD_SRCS))

$(PATH_INTERNAL_C)/parts/gui/%.o: $(PATH_INTERNAL_C)/parts/gui/%.cpp
	$(CXX) -O2 $(CXXFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

$(PATH_INTERNAL_C)/parts/gui/%.o: $(PATH_INTERNAL_C)/parts/gui/%.c
	$(CC) -O2 $(CFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

ifdef DEP_COMMON_DIALOGS
	EXE_LIBS += $(GUI_OBJS) $(TFD_OBJS)
endif

CLEAN_LIST += $(GUI_OBJS) $(TFD_OBJS)
