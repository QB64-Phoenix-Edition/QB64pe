FREETYPE_SRCS := \
	freetypeamalgam.c

FONT_SRCS := \
	font.cpp

FONT_STUB_SRCS := \
	stub_font.cpp

FREETYPE_OBJS := $(patsubst %.c,$(PATH_INTERNAL_C)/parts/video/font/%.o,$(FREETYPE_SRCS))

FONT_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/video/font/%.o,$(FONT_SRCS))

FONT_STUB_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/video/font/%.o,$(FONT_STUB_SRCS))

$(PATH_INTERNAL_C)/parts/video/font/%.o: $(PATH_INTERNAL_C)/parts/video/font/%.c
	$(CC) -O2 $(CFLAGS) -Wall $< -c -o $@

$(PATH_INTERNAL_C)/parts/video/font/%.o: $(PATH_INTERNAL_C)/parts/video/font/%.cpp
	$(CXX) -O2 $(CXXFLAGS) -Wall $< -c -o $@

CLEAN_LIST += $(FREETYPE_OBJS) $(FONT_OBJS) $(FONT_STUB_OBJS)
