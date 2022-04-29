
AUDIO_CONVERSION_SRCS := $(wildcard $(PATH_INTERNAL_C)/parts/audio/conversion/src/*.c)
AUDIO_CONVERSION_OBJS := $(AUDIO_CONVERSION_SRCS:.c=.o)

$(PATH_INTERNAL_C)/parts/audio/conversion/src/%.o: $(PATH_INTERNAL_C)/parts/audio/conversion/src/%.c
	$(CC) -Wall $< -c -o $@

QB_AUDIO_CONVERSION_LIB := $(PATH_INTERNAL_C)/parts/audio/conversion/src.a

$(QB_AUDIO_CONVERSION_LIB): $(AUDIO_CONVERSION_OBJS)
	$(AR) rcs $@ $(AUDIO_CONVERSION_OBJS)

CLEAN_LIST += $(AUDIO_CONVERSION_OBJS) $(QB_AUDIO_CONVERSION_LIB)

