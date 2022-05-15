
AUDIO_DECODE_OGG_SRCS := $(wildcard $(PATH_INTERNAL_C)/parts/audio/decode/ogg/src/*.c)
AUDIO_DECODE_OGG_OBJS := $(AUDIO_DECODE_OGG_SRCS:.c=.o)

$(PATH_INTERNAL_C)/parts/audio/decode/ogg/src/%.o: $(PATH_INTERNAL_C)/parts/audio/decode/ogg/src/%.c
	$(CC) -Wall $< -c -o $@

QB_AUDIO_DECODE_OGG_LIB := $(PATH_INTERNAL_C)/parts/audio/decode/ogg/src.a

$(QB_AUDIO_DECODE_OGG_LIB): $(AUDIO_DECODE_OGG_OBJS)
	$(AR) rcs $@ $(AUDIO_DECODE_OGG_OBJS)

CLEAN_LIST += $(AUDIO_DECODE_OGG_OBJS) $(QB_AUDIO_DECODE_OGG_LIB)

