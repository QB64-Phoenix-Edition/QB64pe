
AUDIO_STUB_SRCS := stub_audio.cpp

AUDIO_STUB_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/%.o,$(AUDIO_STUB_SRCS))

AUDIO_SRCS := audio.cpp

AUDIO_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/%.o,$(AUDIO_SRCS))

MINIAUDIO_SRCS := miniaudio/miniaudio.c

MINIAUDIO_OBJS := $(patsubst %.c,$(PATH_INTERNAL_C)/parts/audio/%.o,$(MINIAUDIO_SRCS))

# DEPENDENCY_CONSOLE_ONLY is added here to keep these .cpp files from including
# the FreeGLUT headers via `libqb.h`. Ideally this is fixed properly in the future.
$(PATH_INTERNAL_C)/parts/audio/%.o: $(PATH_INTERNAL_C)/parts/audio/%.cpp
	$(CXX) -O3 $(CXXFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

$(PATH_INTERNAL_C)/parts/audio/%.o: $(PATH_INTERNAL_C)/parts/audio/%.c
	$(CC) -O3 $(CFLAGS) -Wall $< -c -o $@

AUDIO_LIB := $(PATH_INTERNAL_C)/parts/audio/audio.a

$(AUDIO_LIB): $(AUDIO_OBJS) $(MINIAUDIO_OBJS) $(FOO_MIDI_OBJS) $(HIVELYTRACKER_OBJS) $(LIBMIDI_OBJS) $(LIBXMP_OBJS) $(PRIMESYNTH_OBJS) $(QOA_OBJS) $(OPAL_OBJS) $(STB_VORBIS_OBJS) $(TINYSOUNDFONT_OBJS) $(YMFMIDI_OBJS) $(MA_VTABLES_OBJS)
	$(AR) rcs $@  $(AUDIO_OBJS) $(MINIAUDIO_OBJS) $(FOO_MIDI_OBJS) $(HIVELYTRACKER_OBJS) $(LIBMIDI_OBJS) $(LIBXMP_OBJS) $(PRIMESYNTH_OBJS) $(QOA_OBJS) $(OPAL_OBJS) $(STB_VORBIS_OBJS) $(TINYSOUNDFONT_OBJS) $(YMFMIDI_OBJS) $(MA_VTABLES_OBJS)

CLEAN_LIST += $(AUDIO_STUB_OBJS) $(AUDIO_OBJS) $(MINIAUDIO_OBJS) $(AUDIO_LIB)
