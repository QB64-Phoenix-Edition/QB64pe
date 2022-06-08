
ifeq ($(OS),lnx)
	GAMEPAD_SRCS := Gamepad_linux.c Gamepad_private.c
endif

ifeq ($(OS),win)
	GAMEPAD_SRCS := Gamepad_windows_mm.c Gamepad_private.c
endif

ifeq ($(OS),osx)
	GAMEPAD_SRCS := Gamepad_macosx.c Gamepad_private.c
endif

GAMEPAD_OBJS := $(GAMEPAD_SRCS:.c=.o)
GAMEPAD_OBJS := $(patsubst %,$(PATH_INTERNAL_C)/parts/input/game_controller/src/%,$(GAMEPAD_OBJS))

$(PATH_INTERNAL_C)/parts/input/game_controller/src/%.o: $(PATH_INTERNAL_C)/parts/input/game_controller/src/%.c
	$(CC) -Wall $< -c -o $@

QB_DEVICE_INPUT_LIB := $(PATH_INTERNAL_C)/parts/input/game_controller/src.a

$(QB_DEVICE_INPUT_LIB): $(GAMEPAD_OBJS)
	$(AR) rcs $@ $(GAMEPAD_OBJS)

