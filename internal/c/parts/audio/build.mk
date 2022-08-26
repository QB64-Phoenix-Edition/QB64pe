# Include the correct files if DEP_AUDIO_OUT is defined
ifdef DEP_AUDIO_OUT
	MINIAUDIO_SRCS := \
		audio.cpp \
		miniaudio_impl.cpp

	MINIAUDIO_EXTRAS_SRCS := \
		extras/libxmp-lite/common.c \
		extras/libxmp-lite/control.c \
		extras/libxmp-lite/dataio.c \
		extras/libxmp-lite/effects.c \
		extras/libxmp-lite/filter.c \
		extras/libxmp-lite/format.c \
		extras/libxmp-lite/hio.c \
		extras/libxmp-lite/it_load.c \
		extras/libxmp-lite/itsex.c \
		extras/libxmp-lite/lfo.c \
		extras/libxmp-lite/load.c \
		extras/libxmp-lite/load_helpers.c \
		extras/libxmp-lite/md5.c \
		extras/libxmp-lite/memio.c \
		extras/libxmp-lite/misc.c \
		extras/libxmp-lite/mix_all.c \
		extras/libxmp-lite/mixer.c \
		extras/libxmp-lite/mod_load.c \
		extras/libxmp-lite/period.c \
		extras/libxmp-lite/player.c \
		extras/libxmp-lite/read_event.c \
		extras/libxmp-lite/s3m_load.c \
		extras/libxmp-lite/sample.c \
		extras/libxmp-lite/scan.c \
		extras/libxmp-lite/smix.c \
		extras/libxmp-lite/virtual.c \
		extras/libxmp-lite/win32.c \
		extras/libxmp-lite/xm_load.c
else
# If DEP_AUDIO_OUT is undefined, then we compile a stub that doesn't do anything
	MINIAUDIO_SRCS := \
		stub_audio.cpp
endif

MINIAUDIO_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/%.o,$(MINIAUDIO_SRCS))
MINIAUDIO_OBJS += $(patsubst %.c,$(PATH_INTERNAL_C)/parts/audio/%.o,$(MINIAUDIO_EXTRAS_SRCS))

$(PATH_INTERNAL_C)/parts/audio/%.o: $(PATH_INTERNAL_C)/parts/audio/%.cpp
	$(CXX) $(CXXFLAGS) -Wall $< -c -o $@

$(PATH_INTERNAL_C)/parts/audio/extras/libxmp-lite/%.o: $(PATH_INTERNAL_C)/parts/audio/extras/libxmp-lite/%.c
	$(CC) $(CFLAGS) -Wall -DLIBXMP_CORE_PLAYER -DLIBXMP_NO_PROWIZARD -DLIBXMP_NO_DEPACKERS -DBUILDING_STATIC $< -c -o $@

EXE_LIBS += $(MINIAUDIO_OBJS)

CLEAN_LIST += $(MINIAUDIO_OBJS)
