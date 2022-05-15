
AUDIO_DECODE_MP3_SRCS := $(wildcard $(PATH_INTERNAL_C)/parts/audio/decode/mp3_mini/src/*.c)
AUDIO_DECODE_MP3_OBJS := $(AUDIO_DECODE_MP3_SRCS:.c=.o)

$(PATH_INTERNAL_C)/parts/audio/decode/mp3_mini/src/%.o: $(PATH_INTERNAL_C)/parts/audio/decode/mp3_mini/src/%.c
	$(CC) -Wall $< -c -o $@

QB_AUDIO_DECODE_MP3_LIB := $(PATH_INTERNAL_C)/parts/audio/decode/mp3_mini/src.a

$(QB_AUDIO_DECODE_MP3_LIB): $(AUDIO_DECODE_MP3_OBJS)
	$(AR) rcs $@ $(AUDIO_DECODE_MP3_OBJS)

CLEAN_LIST += $(AUDIO_DECODE_MP3_OBJS) $(QB_AUDIO_DECODE_MP3_LIB)

