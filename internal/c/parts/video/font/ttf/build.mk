
QB_FONT_OBJS := $(PATH_INTERNAL_C)/parts/video/font/ttf/src/freetypeamalgam.o

QB_FONT_LIB := $(PATH_INTERNAL_C)/parts/video/font/ttf/src.a

$(PATH_INTERNAL_C)/parts/video/font/ttf/src/%.o: $(PATH_INTERNAL_C)/parts/video/font/ttf/src/%.c
	$(CC) -Wall $< -c -o $@

$(QB_FONT_LIB): $(QB_FONT_OBJS)
	$(AR) rcs $@ $(QB_FONT_OBJS)

CLEAN_LIST += $(QB_FONT_OBJS) $(QB_FONT_LIB)

