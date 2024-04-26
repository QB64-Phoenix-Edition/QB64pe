
# Disable implicit rules
MAKEFLAGS += --no-builtin-rules

ifndef OS
$(error "OS must be set to 'lnx', 'win', or 'osx'")
endif

# The extra tag to put on ./internal/temp and qbx.o when multiple instances are involved
# This is blank for the 'normal' files
TEMP_ID ?=

# Extra flags go at the beginning of the library list
#
# This is important for libraries, since they could potentially be referencing
# things from our dependencies
CXXFLAGS += $(CXXFLAGS_EXTRA)
CXXLIBS += $(CXXLIBS_EXTRA)

# There are no C lib flags, those all go in CXXLIBS
CFLAGS += $(CFLAGS_EXTRA)

EXE_OBJS :=
EXE_LIBS :=

ifeq ($(OS),lnx)
	lnx := y

	PATH_INTERNAL := ./internal
	PATH_INTERNAL_SRC := $(PATH_INTERNAL)/source
	PATH_INTERNAL_TEMP := $(PATH_INTERNAL)/temp$(TEMP_ID)
	PATH_INTERNAL_C := $(PATH_INTERNAL)/c
	PATH_LIBQB := $(PATH_INTERNAL_C)/libqb
	CP := cp -r
	RM := rm -fr
	MKDIR := mkdir -p
	OBJCOPY := objcopy
	FIXPATH = $1
	PLATFORM := posix
	EXTENSION :=

	# Compiling with -no-pie lets some file explorers know we're an executable,
	# not a shared object
	CXXFLAGS += -no-pie

	# Check bitness by getting length of `long
	# 64 bits on x86_64, 32 bits on x86
	BITS := $(shell getconf LONG_BIT)

	ifeq ($(BITS),)
		BITS := 64
	endif
endif

ifeq ($(OS),win)
	win := y

	PATH_INTERNAL := internal
	PATH_INTERNAL_SRC := $(PATH_INTERNAL)\source
	PATH_INTERNAL_TEMP := $(PATH_INTERNAL)\temp$(TEMP_ID)
	PATH_INTERNAL_C := $(PATH_INTERNAL)\c
	PATH_LIBQB := $(PATH_INTERNAL_C)\libqb
	SHELL := cmd
	CP := xcopy /E /C /H /R /Y
	AR := $(PATH_INTERNAL_C)\c_compiler\bin\ar.exe
	CC := $(PATH_INTERNAL_C)\c_compiler\bin\gcc.exe
	CXX := $(PATH_INTERNAL_C)\c_compiler\bin\c++.exe
	OBJCOPY := $(PATH_INTERNAL_C)\c_compiler\bin\objcopy.exe
	WINDRES := $(PATH_INTERNAL_C)\c_compiler\bin\windres.exe
	ICON_OBJ := $(PATH_INTERNAL_TEMP)\icon.o
	RM := del /Q
	MKDIR := mkdir
	FIXPATH = $(subst /,\,$1)
	PLATFORM := windows
	EXTENSION := .exe

	# Check bitness by seeing which compiler we have
	ifeq "$(filter $(findstring aarch64,$(shell $(CC) -dumpmachine)) $(findstring x86_64,$(shell $(CC) -dumpmachine)),aarch64 x86_64)" ""
		BITS := 32
	else
		BITS := 64
	endif
endif

ifeq ($(OS),osx)
	osx := y

	PATH_INTERNAL := ./internal
	PATH_INTERNAL_SRC := $(PATH_INTERNAL)/source
	PATH_INTERNAL_TEMP := $(PATH_INTERNAL)/temp$(TEMP_ID)
	PATH_INTERNAL_C := $(PATH_INTERNAL)/c
	PATH_LIBQB := $(PATH_INTERNAL_C)/libqb
	CP := cp -r
	RM := rm -fr
	MKDIR := mkdir -p
	FIXPATH = $1
	BITS := 64
	PLATFORM := posix
	EXTENSION :=
endif

ifeq ($(BITS),64)
	OBJCOPY_FLAGS := -Oelf64-x86-64 -Bi386:x86-64
else
	OBJCOPY_FLAGS := -Oelf32-i386 -Bi386
endif

ifdef BUILD_QB64
	ifeq ($(OS),win)
		EXE ?= qb64pe.exe
	else
		EXE ?= qb64pe
	endif
endif

ifneq ($(filter clean build-tests,$(MAKECMDGOALS)),)
	# We have to define this for the Makefile to work,
	# but it doesn't actually matter what it is since clean and build-tests don't compile an executable
	EXE := blah
endif

ifndef EXE
$(error Please provide executable name as 'EXE=executable')
endif

GENERATE_LICENSE ?= n

LICENSE ?= $(EXE).license.txt
LICENSE_IN_USE := qb64 tinyfiledialogs

all: $(EXE)

CLEAN_LIST :=
CLEAN_DEP_LIST :=

CXXFLAGS += -std=gnu++17

# libqb does some illegal type punning, this ensures the compiler will allow it
# to happen
CXXFLAGS += -fno-strict-aliasing

# Significant amounts of the code uses NULL as a placeholder for passing zero
# for any parameter. This should be fixed, but suppressing these warnings makes
# the warning list actually usable.
CXXFLAGS += -Wno-conversion-null

ifeq ($(OS),lnx)
	CXXLIBS += -lGL -lGLU -lX11 -lpthread -ldl -lrt -lxcb
	CXXFLAGS += -DFREEGLUT_STATIC
endif

ifeq ($(OS),win)
	CXXLIBS += -static-libgcc -static-libstdc++ -lcomdlg32 -lole32 -lshlwapi -lwindowscodecs
	CXXFLAGS += -DGLEW_STATIC -DFREEGLUT_STATIC
endif

ifeq ($(OS),osx)
	CXXLIBS += -framework OpenGL -framework IOKit -framework GLUT -framework Cocoa -framework ApplicationServices -framework CoreFoundation

	# OSX doesn't strip using objcopy, so we're using `-s` instead
	ifneq ($(STRIP_SYMBOLS),n)
		CXXLIBS += -s
	endif
endif

QB_QBX_SRC := $(PATH_INTERNAL_C)/qbx$(TEMP_ID).cpp
QB_QBX_OBJ := $(patsubst %.cpp,%.o,$(QB_QBX_SRC))

$(QB_QBX_OBJ): $(wildcard $(PATH_INTERNAL)/temp$(TEMP_ID)/*.txt)

EXE_OBJS += $(QB_QBX_OBJ)

CLEAN_LIST += $(QB_QBX_OBJ)

ifdef BUILD_QB64
	# Copy the QB64-PE source code into temp before compiling
ifeq ($(OS),win)
	_shell := $(shell $(CP) $(PATH_INTERNAL_SRC)\\* $(PATH_INTERNAL_TEMP)\\)
else
	_shell := $(shell $(CP) $(PATH_INTERNAL_SRC)/* $(PATH_INTERNAL_TEMP)/)
endif

	# Required dependencies of QB64-PE itself
	DEP_FONT := y
	DEP_ICON := y
	DEP_ICON_RC := y
	DEP_SOCKETS := y
	DEP_HTTP := y
	DEP_CONSOLE := y
	DEP_ZLIB := y
endif

include $(PATH_INTERNAL_C)/libqb/build.mk

CXXFLAGS += -I$(PATH_LIBQB)/include
EXE_LIBS += $(libqb-objs-y)

include $(PATH_INTERNAL_C)/parts/audio/extras/build.mk
include $(PATH_INTERNAL_C)/parts/audio/build.mk
include $(PATH_INTERNAL_C)/parts/core/build.mk
include $(PATH_INTERNAL_C)/parts/input/game_controller/build.mk
include $(PATH_INTERNAL_C)/parts/video/font/build.mk
include $(PATH_INTERNAL_C)/parts/video/image/build.mk
include $(PATH_INTERNAL_C)/parts/gui/build.mk
include $(PATH_INTERNAL_C)/parts/network/http/build.mk
include $(PATH_INTERNAL_C)/parts/compression/build.mk
include $(PATH_INTERNAL_C)/parts/os/clipboard/build.mk

.PHONY: all clean

QBLIB_NAME := libqb_make_

CLEAN_LIST += $(wildcard $(PATH_INTERNAL_C)/$(QBLIB_NAME)*.o)

ifneq ($(filter y,$(DEP_GL)),)
	CXXFLAGS += -DDEPENDENCY_GL
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_SCREENIMAGE) $(DEP_IMAGE_CODEC)),)
	CXXFLAGS += -DDEPENDENCY_IMAGE_CODEC
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))

	LICENSE_IN_USE += stb_image nanosvg dr_pcx qoi stb_image_write hqx mmpx sxbr
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_CONSOLE_ONLY)),)
	CXXFLAGS += -DDEPENDENCY_CONSOLE_ONLY
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_SOCKETS)),)
	CXXFLAGS += -DDEPENDENCY_SOCKETS
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	CXXFLAGS += -DDEPENDENCY_NO_SOCKETS
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_PRINTER)),)
	CXXFLAGS += -DDEPENDENCY_PRINTER
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	CXXFLAGS += -DDEPENDENCY_NO_PRINTER
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_ICON_RC) $(DEP_ICON)),)
	CXXFLAGS += -DDEPENDENCY_ICON
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	CXXFLAGS += -DDEPENDENCY_NO_ICON
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_ICON_RC)),)
	ifeq ($(OS),win)
		EXE_OBJS += $(ICON_OBJ)
	endif
endif

ifneq ($(filter y,$(DEP_SCREENIMAGE)),)
	CXXFLAGS += -DDEPENDENCY_SCREENIMAGE
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))

	LICENSE_IN_USE += clip
else
	CXXFLAGS += -DDEPENDENCY_NO_SCREENIMAGE
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_FONT)),)
	EXE_LIBS += $(FONT_OBJS) $(FREETYPE_EXE_LIBS)

	LICENSE_IN_USE += freetype_ftl
else
	EXE_LIBS += $(FONT_STUB_OBJS)
endif

ifneq ($(filter y,$(DEP_DEVICEINPUT)),)
	EXE_LIBS += $(QB_DEVICE_INPUT_LIB)

	CXXFLAGS += -DDEPENDENCY_DEVICEINPUT
	ifeq ($(OS),win)
		CXXLIBS += -lwinmm -lxinput -ldinput8 -ldxguid -lwbemuuid -lole32 -loleaut32
	endif

	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))

	LICENSE_IN_USE += libstem_gamepad
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_AUDIO_MINIAUDIO)),)
	EXE_LIBS += $(MINIAUDIO_OBJS)

	CXXFLAGS += -DDEPENDENCY_AUDIO_MINIAUDIO
	ifeq ($(OS),lnx)
		CXXLIBS += -lm -lasound
	endif
	ifeq ($(OS),win)
		CXXLIBS += -lwinmm -lksguid -ldxguid -lole32
	endif
	ifeq ($(OS),osx)
		CXXLIBS += -lpthread -lm -framework CoreAudio -framework CoreMIDI -framework AudioUnit -framework AudioToolbox
	endif
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))

	LICENSE_IN_USE += miniaudio stbvorbis libxmp-lite radv2 hivelytracker qoa

	ifdef DEP_AUDIO_DECODE_MIDI
		LICENSE_IN_USE += tinysoundfont tinymidiloader
	endif
else
	EXE_LIBS += $(MINIAUDIO_STUB_OBJS)
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_ZLIB)),)
	EXE_LIBS += $(MINIZ_OBJS) $(COMPRESSION_OBJS)

	LICENSE_IN_USE += miniz
endif

ifneq ($(filter y,$(DEP_HTTP)),)
	EXE_LIBS += $(CURL_EXE_LIBS)
	CXXFLAGS += $(CURL_CXXFLAGS)
	CXXLIBS += $(CURL_CXXLIBS)

	LICENSE_IN_USE += libcurl
endif

ifneq ($(OS),osx)
	EXE_LIBS += $(QB_CORE_LIB) $(GLEW_OBJS)

	LICENSE_IN_USE += freeglut
else
	EXE_LIBS += $(GLEW_OBJS)
endif

ifeq ($(OS),win)
	LICENSE_IN_USE += mingw-base-runtime libstdc++

	ifneq ($(filter y,$(DEP_CONSOLE_ONLY) $(DEP_CONSOLE)),)
		CXXLIBS += -mconsole
	else
		CXXLIBS += -mwindows
	endif

	ifneq ($(filter y,$(DEP_CONSOLE_ONLY)),)
		CXXFLAGS := $(filter-out -DFREEGLUT_STATIC,$(CXXFLAGS))
		EXE_LIBS := $(filter-out $(QB_CORE_LIB) $(GLEW_OBJS),$(EXE_LIBS))

		LICENSE_IN_USE := $(filter-out freeglut,$(LICENSE_IN_USE))
	else
		CXXLIBS += -lopengl32 -lglu32 -lwinmm -lgdi32
	endif

	ifneq ($(filter y,$(DEP_SOCKETS)),)
		CXXLIBS += -lws2_32
	endif

	ifneq ($(filter y,$(DEP_PRINTER)),)
		CXXLIBS += -lwinspool
	endif

	ifneq ($(filter y,$(DEP_ICON) $(DEP_ICON_RC) $(DEP_SCREENIMAGE) $(DEP_PRINTER)),)
		CXXLIBS += -lgdi32
	endif
endif

ifneq ($(filter y,$(DEP_EMBED)),)
	EXE_OBJS += $(PATH_INTERNAL_TEMP)/embedded.o
endif

QBLIB := $(PATH_INTERNAL_C)/$(QBLIB_NAME).o

$(QBLIB): $(PATH_INTERNAL_C)/libqb.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

ifeq ($(OS),win)
CLEAN_LIST += $(ICON_OBJ)
$(ICON_OBJ): $(PATH_INTERNAL_TEMP)\icon.rc
	$(WINDRES) -i $< -o $@
endif

# QBLIB has to go first to ensure correct linking
EXE_OBJS := $(QBLIB) $(EXE_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

# qbx produces thousands of warnings due to passing NULL for every unused parameter
$(QB_QBX_OBJ): $(QB_QBX_SRC)
	$(CXX) $(CXXFLAGS) $< -c -o $@

ifeq ($(OS),osx)
%.o: %.mm
	$(CXX) $(CXXFLAGS) $< -c -o $@
endif

$(PATH_INTERNAL_TEMP)/embedded.o: $(PATH_INTERNAL_TEMP)/embedded.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

# Clean all files out of ./internal/temp except for temp.bin
CLEAN_LIST += $(wildcard $(PATH_INTERNAL_TEMP)/*)
CLEAN_LIST := $(filter-out $(PATH_INTERNAL_TEMP)/temp.bin,$(CLEAN_LIST))

clean: $(CLEAN_DEP_LIST)
	$(RM) $(call FIXPATH,$(CLEAN_LIST))

ifeq ($(GENERATE_LICENSE),y)
LICENSE_FILES := $(patsubst %,licenses/license_%.txt,$(LICENSE_IN_USE))
LICENSE_DEP := $(LICENSE)
endif

$(EXE): $(EXE_OBJS) $(EXE_LIBS) $(LICENSE_DEP)
	$(CXX) $(CXXFLAGS) $(EXE_OBJS) -o "$@" $(EXE_LIBS) $(CXXLIBS)
ifneq ($(filter-out osx,$(OS)),)
ifneq ($(STRIP_SYMBOLS),n)
	$(OBJCOPY) --only-keep-debug "$@" "$(PATH_INTERNAL_TEMP)/$(notdir $@).sym"
	$(OBJCOPY) --strip-unneeded "$@"
endif
endif

$(LICENSE): $(LICENSE_FILES)
ifeq ($(GENERATE_LICENSE),y)
ifeq ($(OS),win)
	type $(call FIXPATH,$^) > "$@"
else
	cat $^ > "$@"
endif
endif

-include ./tests/build.mk

