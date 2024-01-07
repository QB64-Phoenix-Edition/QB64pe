
FONT_SRCS := \
	font.cpp

FONT_STUB_SRCS := \
	stub_font.cpp

FONT_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/video/font/%.o,$(FONT_SRCS))

FONT_STUB_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/video/font/%.o,$(FONT_STUB_SRCS))

$(PATH_INTERNAL_C)/parts/video/font/%.o: $(PATH_INTERNAL_C)/parts/video/font/%.cpp
	$(CXX) -O2 $(CXXFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

ifeq ($(OS),win)
# The internal FreeType library is only used for Windows. Linux and macOS uses
# the one installed by their respective package manager. When updating the
# FreeType library for Windows, simply download the latest source release,
# delete the old contents of the freetype directory and move the entire
# FreeType source tree into it.

FREETYPE_MAKE_FLAGS := OS=Windows_NT
FREETYPE_MAKE_FLAGS += CC=../../../../c_compiler/bin/gcc.exe
FREETYPE_MAKE_FLAGS += AR=../../../../c_compiler/bin/ar.exe

FREETYPE_LIB := $(PATH_INTERNAL_C)/parts/video/font/freetype/objs/freetype.a

$(PATH_INTERNAL_C)/parts/video/font/freetype/config.mk:
	$(MAKE) -C $(PATH_INTERNAL_C)/parts/video/font/freetype -f ./Makefile $(FREETYPE_MAKE_FLAGS)

$(FREETYPE_LIB): $(PATH_INTERNAL_C)/parts/video/font/freetype/config.mk
	$(MAKE) -C $(PATH_INTERNAL_C)/parts/video/font/freetype -f ./Makefile $(FREETYPE_MAKE_FLAGS)

.PHONY: clean-freetype-lib
clean-freetype-lib:
	$(MAKE) -C $(PATH_INTERNAL_C)/parts/video/font/freetype -f ./Makefile $(FREETYPE_MAKE_FLAGS) distclean

CLEAN_DEP_LIST += clean-freetype-lib
CLEAN_LIST += $(FREETYPE_LIB) $(FONT_OBJS) $(FONT_STUB_OBJS)

FREETYPE_EXE_LIBS := $(FREETYPE_LIB)

FREETYPE_CXXFLAGS := -I$(PATH_INTERNAL_C)/parts/video/font/freetype/include

FREETYPE_CXXLIBS :=

else

CLEAN_LIST += $(FONT_OBJS) $(FONT_STUB_OBJS)

FREETYPE_EXE_LIBS :=

FREETYPE_CXXFLAGS := $(shell pkg-config --cflags freetype2)

FREETYPE_CXXLIBS := $(shell pkg-config --libs freetype2)

endif
