
# GLEW Setup:
# Download the latest GLEW source release from https://github.com/nigels-com/glew/releases/latest
# Only copy glew.c in src/ to internal/c/parts/core/glew
# Copy the include directory to internal/c/parts/core/glew
# Compile the source using -DGLEW_STATIC

FREEGLUT_SRCS := \
	$(wildcard $(PATH_INTERNAL_C)/parts/core/src/*.c) \
	$(wildcard $(PATH_INTERNAL_C)/parts/core/glew/*.c)

FREEGLUT_INCLUDE := -I$(PATH_INTERNAL_C)/parts/core/src/ -I$(PATH_INTERNAL_C)/parts/core/glew/include/

FREEGLUT_OBJS := $(FREEGLUT_SRCS:.c=.o)

FREEGLUT_LIB := $(PATH_INTERNAL_C)/parts/core/src.a

$(PATH_INTERNAL_C)/parts/core/glew/%.o: $(PATH_INTERNAL_C)/parts/core/glew/%.c
	$(CC) -O3 $(CFLAGS) $(FREEGLUT_INCLUDE) -DGLEW_STATIC -Wall $< -c -o $@

$(PATH_INTERNAL_C)/parts/core/src/%.o: $(PATH_INTERNAL_C)/parts/core/src/%.c
	$(CC) -O3 $(CFLAGS) $(FREEGLUT_INCLUDE) -Wall $< -c -o $@

$(FREEGLUT_LIB): $(FREEGLUT_OBJS)
	$(AR) rcs $@ $(FREEGLUT_OBJS)

QB_CORE_LIB := $(FREEGLUT_LIB)

CXXFLAGS += $(FREEGLUT_INCLUDE)

CLEAN_LIST += $(FREEGLUT_LIB) $(FREEGLUT_OBJS)
