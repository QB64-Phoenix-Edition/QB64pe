
IMAGE_SRCS := \
	image.cpp \
	jo_gif/jo_gif.cpp \
	nanosvg/nanosvg.cpp \
	pixelscalers/hqx.cpp \
	pixelscalers/mmpx.cpp \
	pixelscalers/sxbr.cpp \
	qoi/qoi.cpp \
	sg_curico/sg_curico.cpp \
	sg_pcx/sg_pcx.cpp \
	stb/stb_image.cpp

IMAGE_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/video/image/%.o,$(IMAGE_SRCS))

$(PATH_INTERNAL_C)/parts/video/image/%.o: $(PATH_INTERNAL_C)/parts/video/image/%.cpp
	$(CXX) -O3 $(CXXFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

IMAGE_LIB := $(PATH_INTERNAL_C)/parts/video/image/image.a

$(IMAGE_LIB): $(IMAGE_OBJS)
	$(AR) rcs $@ $(IMAGE_OBJS)

ifdef DEP_IMAGE_CODEC
    EXE_LIBS += $(IMAGE_LIB)
endif

CLEAN_LIST += $(IMAGE_OBJS) $(IMAGE_LIB)
