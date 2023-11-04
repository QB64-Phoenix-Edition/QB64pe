
ifeq ($(OS),lnx)
	GAMEPAD_SRCS := Gamepad_linux.c Gamepad_private.c
endif

ifeq ($(OS),win)
	GAMEPAD_SRCS := Gamepad_windows_mm.c Gamepad_private.c
endif

ifeq ($(OS),osx)
	GAMEPAD_SRCS := Gamepad_macosx.c Gamepad_private.c
endif

GAMECONTROLLER_SRCS := game_controller.cpp

GAMEPAD_OBJS := $(patsubst %.c,$(PATH_INTERNAL_C)/parts/input/game_controller/libstem_gamepad/%.o,$(GAMEPAD_SRCS))

GAMECONTROLLER_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/input/game_controller/%.o,$(GAMECONTROLLER_SRCS))

$(PATH_INTERNAL_C)/parts/input/game_controller/libstem_gamepad/%.o: $(PATH_INTERNAL_C)/parts/input/game_controller/libstem_gamepad/%.c
	$(CC) -O2 $(CFLAGS) -Wall $< -c -o $@

$(PATH_INTERNAL_C)/parts/input/game_controller/%.o: $(PATH_INTERNAL_C)/parts/input/game_controller/%.cpp
	$(CXX) -O2 $(CXXFLAGS) -Wall $< -c -o $@

QB_DEVICE_INPUT_LIB := $(PATH_INTERNAL_C)/parts/input/game_controller/game_controller.a

$(QB_DEVICE_INPUT_LIB): $(GAMEPAD_OBJS) $(GAMECONTROLLER_OBJS)
	$(AR) rcs $@ $(GAMEPAD_OBJS) $(GAMECONTROLLER_OBJS)

CLEAN_LIST += $(QB_DEVICE_INPUT_LIB) $(GAMEPAD_OBJS) $(GAMECONTROLLER_OBJS)

