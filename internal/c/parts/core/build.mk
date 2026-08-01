
# When updating the GLFW library, remove everything except the "include" and "src" directories.
# In the "src" directory, remove everything except *.c, *.m, and *.h files.

# GLAD can be updated simply by copying the generated files into the "glad" directory.
# Make sure to copy both the "include" and "src" directories.

GLFW_SRCS := \
	context.c \
	egl_context.c \
	init.c \
	input.c \
	monitor.c \
	null_init.c \
	null_joystick.c \
	null_monitor.c \
	null_window.c \
	osmesa_context.c \
	platform.c \
	vulkan.c \
	window.c

ifeq ($(OS),win)
	GLFW_SRCS += \
		wgl_context.c \
		win32_init.c \
		win32_joystick.c \
		win32_module.c \
		win32_monitor.c \
		win32_thread.c \
		win32_time.c \
		win32_window.c

	GLFW_DEFS := -D_GLFW_WIN32
endif

ifeq ($(OS),lnx)
	GLFW_SRCS += \
		glx_context.c \
		linux_joystick.c \
		posix_poll.c \
		posix_time.c \
		wl_init.c \
		wl_monitor.c \
		wl_window.c \
		x11_init.c \
		x11_monitor.c \
		x11_window.c \
		xkb_unicode.c

	GLFW_DEFS := -D_GLFW_X11
endif

ifeq ($(OS),osx)
	GLFW_SRCS += \
		cocoa_time.c
	
	GLFW_OSX_SRCS := \
		cocoa_init.m \
		cocoa_joystick.m \
		cocoa_monitor.m \
		cocoa_window.m \
		nsgl_context.m

	GLFW_DEFS := -D_GLFW_COCOA
endif

# These are needed for both macOS and Linux.
ifneq ($(OS),win)
	GLFW_SRCS += \
		posix_module.c \
		posix_thread.c
endif

GLAD_SRCS := $(PATH_INTERNAL_C)/parts/core/glad/src/gl.c

CORE_INCLUDE := -I$(PATH_INTERNAL_C)/parts/core/glad/include -I$(PATH_INTERNAL_C)/parts/core/glfw/include

GLFW_OBJS := $(patsubst %.c,$(PATH_INTERNAL_C)/parts/core/glfw/src/%.o,$(GLFW_SRCS))

ifeq ($(OS),osx)
	GLFW_OBJS += $(patsubst %.m,$(PATH_INTERNAL_C)/parts/core/glfw/src/%.o,$(GLFW_OSX_SRCS))
endif

GLAD_OBJS := $(GLAD_SRCS:.c=.o)

#GLAD_DEFS :=

CORE_LIB := $(PATH_INTERNAL_C)/parts/core/core.a

$(PATH_INTERNAL_C)/parts/core/glad/src/%.o: $(PATH_INTERNAL_C)/parts/core/glad/src/%.c
	$(CC) -O3 $(CFLAGS) $(CORE_INCLUDE) $(GLAD_DEFS) -w $< -c -o $@

$(PATH_INTERNAL_C)/parts/core/glfw/src/%.o: $(PATH_INTERNAL_C)/parts/core/glfw/src/%.c
	$(CC) -O3 $(CFLAGS) $(CORE_INCLUDE) $(GLFW_DEFS) -w $< -c -o $@

ifeq ($(OS),osx)
$(PATH_INTERNAL_C)/parts/core/glfw/src/%.o: $(PATH_INTERNAL_C)/parts/core/glfw/src/%.m
	$(CC) -O3 $(CFLAGS) $(CORE_INCLUDE) $(GLFW_DEFS) -w $< -c -o $@
endif

$(CORE_LIB): $(GLFW_OBJS) $(GLAD_OBJS)
	$(AR) rcs $@ $(GLFW_OBJS) $(GLAD_OBJS)

QB_CORE_LIB := $(CORE_LIB)

CXXFLAGS += $(CORE_INCLUDE) $(GLAD_DEFS) $(GLFW_DEFS)

CLEAN_LIST += $(CORE_LIB) $(GLFW_OBJS) $(GLAD_OBJS)
