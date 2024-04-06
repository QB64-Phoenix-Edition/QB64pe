
# clip Setup:
# Download the latest release from https://github.com/dacap/clip
# Copy all source files except clip_none.cpp to internal/c/parts/os/clipboard/clip
# Compile the source using -DCLIP_ENABLE_IMAGE=1, -DHAVE_XCB_XLIB_H (Linux) and DHAVE_PNG_H (Linux)

CLIP_DEFS := -DCLIP_ENABLE_IMAGE=1

CLIP_SRCS := \
	clip.cpp \
	image.cpp

ifeq ($(OS),lnx)
	CLIP_SRCS += clip_x11.cpp
	CLIP_DEFS += -DHAVE_XCB_XLIB_H -DHAVE_PNG_H
	CXXLIBS += -lpng
endif

ifeq ($(OS),win)
	CLIP_SRCS += clip_win.cpp
endif

ifeq ($(OS),osx)
	CLIP_OSX_SRCS := clip_osx.mm
endif

CLIPBOARD_SRCS := clipboard.cpp

CLIP_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/os/clipboard/clip/%.o,$(CLIP_SRCS))

ifeq ($(OS),osx)
CLIP_OBJS += $(patsubst %.mm,$(PATH_INTERNAL_C)/parts/os/clipboard/clip/%.o,$(CLIP_OSX_SRCS))
endif

CLIPBOARD_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/os/clipboard/%.o,$(CLIPBOARD_SRCS))

$(PATH_INTERNAL_C)/parts/os/clipboard/clip/%.o: $(PATH_INTERNAL_C)/parts/os/clipboard/clip/%.cpp
	$(CXX) -O2 $(CXXFLAGS) $(CLIP_DEFS) -w $< -c -o $@

ifeq ($(OS),osx)
$(PATH_INTERNAL_C)/parts/os/clipboard/clip/%.o: $(PATH_INTERNAL_C)/parts/os/clipboard/clip/%.mm
	$(CXX) -O2 $(CXXFLAGS) $(CLIP_DEFS) -w $< -c -o $@
endif

$(PATH_INTERNAL_C)/parts/os/clipboard/%.o: $(PATH_INTERNAL_C)/parts/os/clipboard/%.cpp
	$(CXX) -O2 $(CXXFLAGS) $(CLIP_DEFS) -Wall -Wextra $< -c -o $@

CLIPBOARD_LIB := $(PATH_INTERNAL_C)/parts/os/clipboard/clipboard.a

$(CLIPBOARD_LIB): $(CLIP_OBJS) $(CLIPBOARD_OBJS)
	$(AR) rcs $@ $(CLIP_OBJS) $(CLIPBOARD_OBJS)

EXE_LIBS += $(CLIPBOARD_LIB)

CLEAN_LIST += $(CLIPBOARD_LIB) $(CLIP_OBJS) $(CLIPBOARD_OBJS)

