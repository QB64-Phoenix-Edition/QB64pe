
# GLEW Setup:
# Download the latest GLEW source release from https://github.com/nigels-com/glew/releases/latest
# Only copy glew.c in src/ to internal/c/parts/core/glew
# Copy the include directory to internal/c/parts/core/glew
# Compile the source using -DGLEW_STATIC
#
# FreeGLUT Setup:
# Although newer version of FreeGLUT (3.x) are available we do not use those.
# This is because the local version has quite a few custom changes that should be moved out.

FREEGLUT_SRCS := \
	$(wildcard $(PATH_INTERNAL_C)/parts/core/freeglut/*.c) \
	$(wildcard $(PATH_INTERNAL_C)/parts/core/glew/*.c)

FREEGLUT_INCLUDE := -I$(PATH_INTERNAL_C)/parts/core/freeglut/include -I$(PATH_INTERNAL_C)/parts/core/glew/include

FREEGLUT_OBJS := $(FREEGLUT_SRCS:.c=.o)

FREEGLUT_LIB := $(PATH_INTERNAL_C)/parts/core/freeglut.a

$(PATH_INTERNAL_C)/parts/core/glew/%.o: $(PATH_INTERNAL_C)/parts/core/glew/%.c
	$(CC) -O3 $(CFLAGS) $(FREEGLUT_INCLUDE) -DGLEW_STATIC -Wall $< -c -o $@

$(PATH_INTERNAL_C)/parts/core/freeglut/%.o: $(PATH_INTERNAL_C)/parts/core/freeglut/%.c
	$(CC) -O3 $(CFLAGS) $(FREEGLUT_INCLUDE) -DFREEGLUT_STATIC -DHAVE_UNISTD_H -Wall $< -c -o $@

$(FREEGLUT_LIB): $(FREEGLUT_OBJS)
	$(AR) rcs $@ $(FREEGLUT_OBJS)

QB_CORE_LIB := $(FREEGLUT_LIB)

CXXFLAGS += $(FREEGLUT_INCLUDE)

CLEAN_LIST += $(FREEGLUT_LIB) $(FREEGLUT_OBJS)
