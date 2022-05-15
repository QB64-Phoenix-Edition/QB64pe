
ifeq ($(OS),lnx)
	AUDIO_OUT_SRCS := \
		src/helpers.c \
		src/bs2b.c \
		src/alAuxEffectSlot.c \
		src/alBuffer.c \
		src/ALc.c \
		src/alcConfig.c \
		src/alcDedicated.c \
		src/alcEcho.c \
		src/alcModulator.c \
		src/alcReverb.c \
		src/alcRing.c \
		src/alcThread.c \
		src/alEffect.c \
		src/alError.c \
		src/alExtension.c \
		src/alFilter.c \
		src/alListener.c \
		src/alsa.c \
		src/alSource.c \
		src/alState.c \
		src/alThunk.c \
		src/ALu.c \
		src/hrtf.c \
		src/loopback.c \
		src/mixer.c \
		src/null.c \
		src/panning.c \
		src/wave.c
endif

ifeq ($(OS),win)
	AUDIO_OUT_SRCS := \
		src/winmm.c \
		src/null.c \
		src/loopback.c \
		src/dsound.c \
		src/panning.c \
		src/mixer.c \
		src/hrtf.c \
		src/helpers.c \
		src/bs2b.c \
		src/ALu.c \
		src/alcThread.c \
		src/alcRing.c \
		src/alcReverb.c \
		src/alcModulator.c \
		src/alcEcho.c \
		src/alcDedicated.c \
		src/alcConfig.c \
		src/ALc.c \
		src/alThunk.c \
		src/alState.c \
		src/alSource.c \
		src/alListener.c \
		src/alFilter.c \
		src/alExtension.c \
		src/alError.c \
		src/alEffect.c \
		src/alBuffer.c \
		src/alAuxEffectSlot.c
endif

ifeq ($(OS),osx)
	AUDIO_OUT_SRCS := \
		src/coreaudio.c \
		src/helpers.c \
		src/bs2b.c \
		src/alAuxEffectSlot.c \
		src/alBuffer.c \
		src/ALc.c \
		src/alcConfig.c \
		src/alcDedicated.c \
		src/alcEcho.c \
		src/alcModulator.c \
		src/alcReverb.c \
		src/alcRing.c \
		src/alcThread.c \
		src/alEffect.c \
		src/alError.c \
		src/alExtension.c \
		src/alFilter.c \
		src/alListener.c \
		src/alSource.c \
		src/alState.c \
		src/alThunk.c \
		src/ALu.c \
		src/hrtf.c \
		src/loopback.c \
		src/mixer.c \
		src/null.c \
		src/panning.c \
		src/wave.c
endif

AUDIO_OUT_OBJS := $(AUDIO_OUT_SRCS:.c=.o)
AUDIO_OUT_OBJS := $(patsubst %,$(PATH_INTERNAL_C)/parts/audio/out/%,$(AUDIO_OUT_OBJS))

$(PATH_INTERNAL_C)/parts/audio/out/src/%.o: $(PATH_INTERNAL_C)/parts/audio/out/src/%.c
	$(CC) -Wall -D AL_LIBTYPE_STATIC $< -c -o $@

QB_AUDIO_OUT_LIB := $(PATH_INTERNAL_C)/parts/audio/out/src.a

$(QB_AUDIO_OUT_LIB): $(AUDIO_OUT_OBJS)
	$(AR) rcs $@ $(AUDIO_OUT_OBJS)

CLEAN_LIST += $(AUDIO_OUT_OBJS) $(QB_AUDIO_OUT_LIB)

