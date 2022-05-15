
FREEGLUT_SRCS := $(wildcard $(PATH_INTERNAL_C)/parts/core/src/*.c)
FREEGLUT_OBJS := $(FREEGLUT_SRCS:.c=.o)

FREEGLUT_LIB := $(PATH_INTERNAL_C)/parts/core/src.a

$(PATH_INTERNAL_C)/parts/core/src/%.o: $(PATH_INTERNAL_C)/parts/core/src/%.c
	$(CC) -O2 -c $< -o $@

$(FREEGLUT_LIB): $(FREEGLUT_OBJS)
	$(AR) rcs $@ $(FREEGLUT_OBJS)

QB_CORE_LIB := $(FREEGLUT_LIB)

CLEAN_LIST += $(FREEGLUT_LIB) $(FREEGLUT_OBJS)

