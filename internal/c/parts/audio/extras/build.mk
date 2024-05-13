
LIBXMP_SRCS := \
	common.c \
	control.c \
	dataio.c \
	effects.c \
	filetype.c \
	filter.c \
	format.c \
	hio.c \
	it_load.c \
	itsex.c \
	lfo.c \
	load.c \
	load_helpers.c \
	md5.c \
	memio.c \
	misc.c \
	mix_all.c \
	mixer.c \
	mod_load.c \
	period.c \
	player.c \
	read_event.c \
	s3m_load.c \
	sample.c \
	scan.c \
	smix.c \
	virtual.c \
	win32.c \
	xm_load.c

LIBXMP_OBJS += $(patsubst %.c,$(PATH_INTERNAL_C)/parts/audio/extras/libxmp-lite/%.o,$(LIBXMP_SRCS))

LIBXMP_LIB := $(PATH_INTERNAL_C)/parts/audio/extras/libxmp-lite.a

$(PATH_INTERNAL_C)/parts/audio/extras/libxmp-lite/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/libxmp-lite/%.c
	$(CC) -O2 $(CFLAGS) -Wall -DLIBXMP_CORE_PLAYER -DLIBXMP_STATIC $< -c -o $@

$(LIBXMP_LIB): $(LIBXMP_OBJS)
	$(AR) rcs $@ $^

HIVELY_SRCS := hvl_replay.c

HIVELY_OBJS += $(patsubst %.c,$(PATH_INTERNAL_C)/parts/audio/extras/hivelytracker/%.o,$(HIVELY_SRCS))

OPAL_SRCS := opal.cpp

OPAL_OBJS += $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/radv2/%.o,$(OPAL_SRCS))

$(PATH_INTERNAL_C)/parts/audio/extras/hivelytracker/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/hivelytracker/%.c
	$(CC) -O2 $(CFLAGS) -Wall $< -c -o $@

MA_VTABLES_SRCS := \
	mod_ma_vtable.cpp \
	radv2_ma_vtable.cpp \
	hively_ma_vtable.cpp \
	qoa_ma_vtable.cpp

MA_VTABLES_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/%.o,$(MA_VTABLES_SRCS))

MIDI_MA_VTABLE_SRCS := midi_ma_vtable.cpp
MIDI_MA_VTABLE_STUB_SRCS := midi_ma_vtable_stub.cpp

MIDI_MA_VTABLES_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/%.o,$(MIDI_MA_VTABLE_SRCS))
MIDI_MA_VTABLES_STUB_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/%.o,$(MIDI_MA_VTABLE_STUB_SRCS))

# If we're using MIDI, then there should be a soundfont available to be linked in
MIDI_MA_VTABLES_OBJS += $(PATH_INTERNAL_TEMP)/soundfont.o

# Turn the soundfont into a linkable object
ifeq ($(OS),win)
# Fairly ugly, but we have to get the right relative path to objcopy on Windows
# to make the whole thing work out
$(PATH_INTERNAL_TEMP)/soundfont.o: $(PATH_INTERNAL_TEMP)/soundfont.sf2
	cd $(call FIXPATH,$(PATH_INTERNAL_TEMP)) && ..\..\$(OBJCOPY) -Ibinary $(OBJCOPY_FLAGS) soundfont.sf2 soundfont.o
else
ifeq ($(OS),osx)
# Mac OS does not ship an objcopy implementation for some reason
# We're instead using xxd to produce a source file, and compiling it
$(PATH_INTERNAL_TEMP)/soundfont.c: $(PATH_INTERNAL_TEMP)/soundfont.sf2
	cd $(call FIXPATH,$(PATH_INTERNAL_TEMP)) && xxd --include soundfont.sf2 > soundfont.c

$(PATH_INTERNAL_TEMP)/soundfont.o: $(PATH_INTERNAL_TEMP)/soundfont.c
	$(CC) $< -c -o $@

else
# The "cd" is used to make the symbol name predictable and not dependent upon
# the "PATH_INTERNAL_TEMP" value
$(PATH_INTERNAL_TEMP)/soundfont.o: $(PATH_INTERNAL_TEMP)/soundfont.sf2
	cd $(call FIXPATH,$(PATH_INTERNAL_TEMP)) && $(OBJCOPY) -Ibinary $(OBJCOPY_FLAGS) soundfont.sf2 soundfont.o
endif
endif

CLEAN_LIST += $(LIBXMP_LIB) $(LIBXMP_OBJS) $(HIVELY_OBJS) $(OPAL_OBJS) $(MA_VTABLES_OBJS) $(MIDI_MA_VTABLES_OBJS) $(MIDI_MA_VTABLES_STUB_OBJS)

