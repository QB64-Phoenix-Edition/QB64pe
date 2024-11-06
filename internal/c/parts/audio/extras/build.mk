
# foo_midi

FOO_MIDI_SRCS := \
	InstrumentBankManager.cpp \
	MIDIPlayer.cpp \
	OpalPlayer.cpp \
	PSPlayer.cpp \
	TSFPlayer.cpp

ifeq ($(OS),win)
	FOO_MIDI_SRCS += VSTiPlayer.cpp
endif

FOO_MIDI_OBJS += $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/foo_midi/%.o,$(FOO_MIDI_SRCS))

$(PATH_INTERNAL_C)/parts/audio/extras/foo_midi/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/foo_midi/%.cpp
	$(CXX) -O3 $(CXXFLAGS) -Wall $< -c -o $@

# hivelytracker

HIVELYTRACKER_SRCS := hvl_replay.c

HIVELYTRACKER_OBJS += $(patsubst %.c,$(PATH_INTERNAL_C)/parts/audio/extras/hivelytracker/%.o,$(HIVELYTRACKER_SRCS))

$(PATH_INTERNAL_C)/parts/audio/extras/hivelytracker/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/hivelytracker/%.c
	$(CC) -O3 $(CFLAGS) -Wall $< -c -o $@

#libmidi

LIBMIDI_SRCS := \
	MIDIContainer.cpp \
	MIDIProcessor.cpp \
	MIDIProcessorGMF.cpp \
	MIDIProcessorHMI.cpp \
	MIDIProcessorHMP.cpp \
	MIDIProcessorLDS.cpp \
	MIDIProcessorMDS.cpp \
	MIDIProcessorMUS.cpp \
	MIDIProcessorRCP.cpp \
	MIDIProcessorRIFF.cpp \
	MIDIProcessorSMF.cpp \
	MIDIProcessorXMI.cpp \
	Recomposer/CM6File.cpp \
	Recomposer/GDSFile.cpp \
	Recomposer/MIDIStream.cpp \
	Recomposer/RCP.cpp \
	Recomposer/RCPConverter.cpp \
	Recomposer/RunningNotes.cpp \
	Recomposer/Support.cpp

LIBMIDI_OBJS += $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/libmidi/%.o,$(LIBMIDI_SRCS))

$(PATH_INTERNAL_C)/parts/audio/extras/libmidi/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/libmidi/%.cpp
	$(CXX) -O3 $(CXXFLAGS) -Wall $< -c -o $@

# libxmp-lite

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

$(PATH_INTERNAL_C)/parts/audio/extras/libxmp-lite/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/libxmp-lite/%.c
	$(CC) -O3 $(CFLAGS) -Wall -DLIBXMP_CORE_PLAYER -DLIBXMP_STATIC $< -c -o $@

# primesynth

PRIMESYNTH_SRCS := primesynth.cpp

PRIMESYNTH_OBJS += $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/primesynth/%.o,$(PRIMESYNTH_SRCS))

$(PATH_INTERNAL_C)/parts/audio/extras/primesynth/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/primesynth/%.cpp
	$(CXX) -O3 $(CXXFLAGS) -Wall $< -c -o $@

# radv2

OPAL_SRCS := opal.cpp

OPAL_OBJS += $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/radv2/%.o,$(OPAL_SRCS))

$(PATH_INTERNAL_C)/parts/audio/extras/radv2/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/radv2/%.cpp
	$(CXX) -O3 $(CXXFLAGS) -Wall $< -c -o $@

# TinySoundFont

TINYSOUNDFONT_SRCS := tsf.cpp

TINYSOUNDFONT_OBJS += $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/tinysoundfont/%.o,$(TINYSOUNDFONT_SRCS))

$(PATH_INTERNAL_C)/parts/audio/extras/tinysoundfont/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/tinysoundfont/%.cpp
	$(CXX) -O3 $(CXXFLAGS) -Wall $< -c -o $@

# ymfmidi

YMFMIDI_SRCS := \
	patches.cpp \
	player.cpp

YMFMIDI_OBJS += $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/ymfmidi/%.o,$(YMFMIDI_SRCS))

$(PATH_INTERNAL_C)/parts/audio/extras/ymfmidi/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/ymfmidi/%.cpp
	$(CXX) -O3 $(CXXFLAGS) -Wall $< -c -o $@

# ma_vtables

MA_VTABLES_SRCS := \
	hively_ma_vtable.cpp \
	midi_ma_vtable.cpp \
	mod_ma_vtable.cpp \
	qoa_ma_vtable.cpp \
	radv2_ma_vtable.cpp

MA_VTABLES_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/extras/%.o,$(MA_VTABLES_SRCS))

$(PATH_INTERNAL_C)/parts/audio/extras/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/%.cpp
	$(CXX) -O3 $(CXXFLAGS) -Wall $< -c -o $@

CLEAN_LIST += $(FOO_MIDI_OBJS) $(HIVELYTRACKER_OBJS) $(LIBMIDI_OBJS) $(LIBXMP_OBJS) $(PRIMESYNTH_OBJS) $(OPAL_OBJS) $(TINYSOUNDFONT_OBJS) $(YMFMIDI_OBJS) $(MA_VTABLES_OBJS)

