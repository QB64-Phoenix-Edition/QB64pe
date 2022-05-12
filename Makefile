
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
	PATH_INTERNAL := ./internal
	PATH_INTERNAL_SRC := $(PATH_INTERNAL)/source
	PATH_INTERNAL_TEMP := $(PATH_INTERNAL)/temp$(TEMP_ID)
	PATH_INTERNAL_C := $(PATH_INTERNAL)/c
	CP := cp -r
	RM := rm -fr
	OBJCOPY := objcopy
	FIXPATH = $1

	# Check bitness by getting length of `long
	# 64 bits on x86_64, 32 bits on x86
	BITS := $(shell getconf LONG_BIT)

	ifeq ($(BITS),)
		BITS := 64
	endif
endif

ifeq ($(OS),win)
	PATH_INTERNAL := internal
	PATH_INTERNAL_SRC := $(PATH_INTERNAL)\source
	PATH_INTERNAL_TEMP := $(PATH_INTERNAL)\temp$(TEMP_ID)
	PATH_INTERNAL_C := $(PATH_INTERNAL)\c
	SHELL := cmd
	CP := xcopy /E /C /H /R /Y
	AR := $(PATH_INTERNAL_C)\c_compiler\bin\ar.exe
	CC := $(PATH_INTERNAL_C)\c_compiler\bin\gcc.exe
	CXX := $(PATH_INTERNAL_C)\c_compiler\bin\c++.exe
	OBJCOPY := $(PATH_INTERNAL_C)\c_compiler\bin\objcopy.exe
	WINDRES := $(PATH_INTERNAL_C)\c_compiler\bin\windres.exe
	ICON_OBJ := $(PATH_INTERNAL_TEMP)\icon.o
	RM := del /Q
	FIXPATH = $(subst /,\,$1)

	# Check bitness by seeing which compiler we have
	ifeq ($(wildcard $(PATH_INTERNAL_C)\c_compiler\i686-w64-mingw32),)
		BITS := 64
	else
		BITS := 32
	endif
endif

ifeq ($(OS),osx)
	PATH_INTERNAL := ./internal
	PATH_INTERNAL_SRC := $(PATH_INTERNAL)/source
	PATH_INTERNAL_TEMP := $(PATH_INTERNAL)/temp$(TEMP_ID)
	PATH_INTERNAL_C := $(PATH_INTERNAL)/c
	CP := cp -r
	RM := rm -fr
	FIXPATH = $1
	BITS := 64
endif

ifeq ($(BITS),64)
	OBJCOPY_FLAGS := -Oelf64-x86-64 -Bi386:x86-64
else
	OBJCOPY_FLAGS := -Oelf32-i386 -Bi386
endif

ifdef BUILD_QB64
	EXE ?= qb64
else

ifneq ($(filter clean,$(MAKECMDGOALS)),)
	# We have to define this for the Makefile to work,
	# but it doesn't actually matter what it is since clean won't compile anything
	EXE := blah
endif

ifndef EXE
$(error Please provide executable name as 'EXE=executable')
endif

endif

all: $(EXE)

CLEAN_LIST :=

CXXFLAGS := -w

ifeq ($(OS),lnx)
	CXXLIBS += -lGL -lGLU -lX11 -lpthread -ldl -lrt
	CXXFLAGS += -DFREEGLUT_STATIC
endif

ifeq ($(OS),win)
	CXXLIBS += -static-libgcc -static-libstdc++
	CXXFLAGS += -DGLEW_STATIC -DFREEGLUT_STATIC
endif

ifeq ($(OS),osx)
	CXXLIBS += -framework OpenGL -framework IOKit -framework GLUT -framework Cocoa
endif

QB_QBX_OBJ := $(PATH_INTERNAL_C)/qbx$(TEMP_ID).o

$(QB_QBX_OBJ): $(wildcard $(PATH_INTERNAL)/temp/*.txt)

EXE_OBJS += $(QB_QBX_OBJ)

CLEAN_LIST += $(QB_QBX_OBJ)

ifdef BUILD_QB64
	# Copy the QB64 source code into temp before compiling
ifeq ($(OS),win)
	_shell := $(shell $(CP) $(PATH_INTERNAL_SRC)\\* $(PATH_INTERNAL_TEMP)\\)
else
	_shell := $(shell $(CP) $(PATH_INTERNAL_SRC)/* $(PATH_INTERNAL_TEMP)/)
endif

	# Required dependencies of QB64 itself
	DEP_FONT := y
	DEP_ICON := y
	DEP_ICON_RC := y
	DEP_SOCKETS := y
endif

include $(PATH_INTERNAL_C)/libqb/build.mk
include $(PATH_INTERNAL_C)/parts/audio/conversion/build.mk
include $(PATH_INTERNAL_C)/parts/audio/decode/mp3_mini/build.mk
include $(PATH_INTERNAL_C)/parts/audio/decode/ogg/build.mk
include $(PATH_INTERNAL_C)/parts/audio/out/build.mk
include $(PATH_INTERNAL_C)/parts/core/build.mk
include $(PATH_INTERNAL_C)/parts/input/game_controller/build.mk
include $(PATH_INTERNAL_C)/parts/video/font/ttf/build.mk

.PHONY: all clean

QBLIB_NAME := libqb_make_

CLEAN_LIST += $(wildcard $(PATH_INTERNAL_C)/libqb/$(QBLIB_NAME)*.o)

ifneq ($(filter y,$(DEP_GL)),)
	CXXFLAGS += -DDEPENDENCY_GL
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_SCREENIMAGE) $(DEP_IMAGE_CODEC)),)
	CXXFLAGS += -DDEPENDENCY_IMAGE_CODEC
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
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
else
	CXXFLAGS += -DDEPENDENCY_NO_SCREENIMAGE
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_FONT)),)
	EXE_LIBS += $(QB_FONT_LIB)
	CXXFLAGS += -DDEPENDENCY_LOADFONT
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_DEVICEINPUT)),)
	EXE_LIBS += $(QB_DEVICE_INPUT_LIB)
	CXXFLAGS += -DDEPENDENCY_DEVICEINPUT
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_AUDIO_CONVERSION) $(DEP_AUDIO_DECODE)),)
	EXE_LIBS += $(QB_AUDIO_CONVERSION_LIB)
	CXXFLAGS += -DDEPENDENCY_AUDIO_CONVERSION
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_AUDIO_DECODE)),)
	EXE_LIBS += $(QB_AUDIO_DECODE_MP3_LIB) $(QB_AUDIO_DECODE_OGG_LIB)
	CXXFLAGS += -DDEPENDENCY_AUDIO_DECODE
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_AUDIO_OUT) $(DEP_AUDIO_CONVERSION) $(DEP_AUDIO_DECODE)),)
	EXE_LIBS += $(QB_AUDIO_OUT_LIB)
	CXXFLAGS += -DDEPENDENCY_AUDIO_OUT
	ifeq ($(OS),osx)
		CXXLIBS += -framework AudioUnit -framework AudioToolbox
	endif
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(filter y,$(DEP_ZLIB)),)
	CXXFLAGS += -DDEPENDENCY_ZLIB
	ifeq ($(OS),osx)
		CXXLIBS += "-lz"
	else
		CXXLIBS += "-l:libz.a"
	endif
	QBLIB_NAME := $(addsuffix 1,$(QBLIB_NAME))
else
	QBLIB_NAME := $(addsuffix 0,$(QBLIB_NAME))
endif

ifneq ($(OS),osx)
	EXE_LIBS += $(QB_CORE_LIB)
endif

ifeq ($(OS),win)
	ifneq ($(filter y,$(DEP_CONSOLE_ONLY) $(DEP_CONSOLE)),)
		CXXLIBS += -mconsole
	endif

	ifneq ($(filter y,$(DEP_CONSOLE_ONLY)),)
		CXXFLAGS := $(filter-out -DFREEGLUT_STATIC,$(CXXFLAGS))
		EXE_LIBS := $(filter-out $(QB_CORE_LIB),$(EXE_LIBS))
	else
		CXXLIBS += -mwindows -lopengl32 -lglu32 -lwinmm
	endif

	ifneq ($(filter y,$(DEP_SOCKETS)),)
		CXXLIBS += -lws2_32
	endif

	ifneq ($(filter y,$(DEP_PRINTER)),)
		CXXLIBS += -lwinspool
	endif

	ifneq ($(filter y,$(DEP_AUDIO_OUT) $(DEP_AUDIO_CONVERSION) $(DEP_AUDIO_DECODE)),)
		CXXLIBS += -lwinmm -lksguid -ldxguid -lole32
	endif

	ifneq ($(filter y,$(DEP_ICON) $(DEP_ICON_RC) $(DEP_SCREENIMAGE) $(DEP_PRINTER)),)
		CXXLIBS += -lgdi32
	endif
endif

ifneq ($(filter y,$(DEP_DATA)),)
	EXE_OBJS += $(PATH_INTERNAL_TEMP)/data.o
endif

# CXXLIBS += $(shell cat "$(PATH_INTERNAL_TEMP)/extra_libs.txt")

QBLIB := $(PATH_INTERNAL_C)/libqb/$(QBLIB_NAME).o

ifneq ($(OS),osx)
$(QBLIB): $(PATH_INTERNAL_C)/libqb.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@
else
$(QBLIB): $(PATH_INTERNAL_C)/libqb.mm
	$(CXX) $(CXXFLAGS) $< -c -o $@
endif

ifeq ($(OS),win)
CLEAN_LIST += $(ICON_OBJ)
$(ICON_OBJ): $(PATH_INTERNAL_TEMP)\icon.rc
	$(WINDRES) -i $< -o $@
endif

# QBLIB has to go first to ensure correct linking
EXE_OBJS := $(QBLIB) $(EXE_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -c -o $@

$(PATH_INTERNAL_TEMP)/data.o: $(PATH_INTERNAL_TEMP)/data.bin
	$(OBJCOPY) -Ibinary $(OBJCOPY_FLAGS) $< $@

# Clean all files out of ./internal/temp except for temp.bin
CLEAN_LIST += $(wildcard $(PATH_INTERNAL_TEMP)/*)
CLEAN_LIST := $(filter-out $(PATH_INTERNAL_TEMP)/temp.bin,$(CLEAN_LIST))

clean:
	$(RM) $(call FIXPATH,$(CLEAN_LIST))

$(EXE): $(EXE_OBJS) $(EXE_LIBS)
	$(CXX) $(CXXFLAGS) $(EXE_OBJS) -o $@ $(EXE_LIBS) $(CXXLIBS)
