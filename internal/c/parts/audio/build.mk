
MINIAUDIO_SRCS := \
	audio.cpp \
	miniaudio_impl.cpp

# We always produce both lists so that `make clean` will clean them up even
# when not passed a particular DEP_* flag
MINIAUDIO_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/%.o,$(MINIAUDIO_SRCS))

# a740g: get rid of this
ifdef DEP_AUDIO_DECODE_MIDI
	MINIAUDIO_OBJS += $(MIDI_MA_VTABLES_OBJS)
else
	MINIAUDIO_OBJS += $(MIDI_MA_VTABLES_STUB_OBJS)
endif

AUDIO_LIB := $(PATH_INTERNAL_C)/parts/audio/audio.a

$(AUDIO_LIB): $(MINIAUDIO_OBJS) $(MA_VTABLES_OBJS) $(HIVELY_OBJS) $(OPAL_OBJS) $(LIBXMP_OBJS) $(MIDI_MA_VTABLES_STUB_OBJS)
	$(AR) rcs $@ $(MINIAUDIO_OBJS) $(MA_VTABLES_OBJS) $(HIVELY_OBJS) $(OPAL_OBJS) $(LIBXMP_OBJS) $(MIDI_MA_VTABLES_STUB_OBJS)

# DEPENDENCY_CONSOLE_ONLY is added here to keep these .cpp files from including
# the FreeGLUT headers via `libqb.h`. Ideally this is fixed properly in the future.
$(PATH_INTERNAL_C)/parts/audio/%.o: $(PATH_INTERNAL_C)/parts/audio/%.cpp
	$(CXX) -O2 $(CXXFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

CLEAN_LIST += $(AUDIO_LIB) $(MINIAUDIO_OBJS) $(MA_VTABLES_OBJS) $(HIVELY_OBJS) $(OPAL_OBJS) $(LIBXMP_OBJS) $(MIDI_MA_VTABLES_STUB_OBJS)

