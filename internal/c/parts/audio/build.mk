
MINIAUDIO_REAL_SRCS := \
	audio.cpp \
	miniaudio_impl.cpp

MINIAUDIO_STUB_SRCS := \
	stub_audio.cpp

# We always produce both lists so that `make clean` will clean them up even
# when not passed a particular DEP_* flag
MINIAUDIO_REAL_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/%.o,$(MINIAUDIO_REAL_SRCS))

MINIAUDIO_STUB_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/%.o,$(MINIAUDIO_STUB_SRCS))

ifdef DEP_AUDIO_MINIAUDIO
	MINIAUDIO_OBJS :=  $(MINIAUDIO_REAL_OBJS) $(MA_VTABLES_OBJS) $(HIVELY_OBJS) $(OPAL_OBJS) $(LIBXMP_LIB)

ifdef DEP_AUDIO_DECODE_MIDI
	MINIAUDIO_OBJS += $(MIDI_MA_VTABLES_OBJS)
else
	MINIAUDIO_OBJS += $(MIDI_MA_VTABLES_STUB_OBJS)
endif

else
	MINIAUDIO_OBJS := $(MINIAUDIO_STUB_OBJS)
endif

# DEPENDENCY_CONSOLE_ONLY is added here to keep these .cpp files from including
# the FreeGLUT headers via `libqb.h`. Ideally this is fixed properly in the future.
$(PATH_INTERNAL_C)/parts/audio/%.o: $(PATH_INTERNAL_C)/parts/audio/%.cpp
	$(CXX) -O2 $(CXXFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

CLEAN_LIST += $(MINIAUDIO_REAL_OBJS) $(MINIAUDIO_STUB_OBJS)
