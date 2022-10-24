
TFD_SRCS := \
	tinyfiledialogs.c

GUI_SRCS := \
	gui.cpp

TFD_OBJS := $(patsubst %.c,$(PATH_INTERNAL_C)/parts/gui/%.o,$(TFD_SRCS))

GUI_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/gui/%.o,$(GUI_SRCS))

$(PATH_INTERNAL_C)/parts/gui/%.o: $(PATH_INTERNAL_C)/parts/gui/%.c
	$(CC) -O2 $(CFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

$(PATH_INTERNAL_C)/parts/gui/%.o: $(PATH_INTERNAL_C)/parts/gui/%.cpp
	$(CXX) -O2 $(CXXFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

EXE_LIBS += $(TFD_OBJS) $(GUI_OBJS)

CLEAN_LIST += $(TFD_OBJS) $(GUI_OBJS)
