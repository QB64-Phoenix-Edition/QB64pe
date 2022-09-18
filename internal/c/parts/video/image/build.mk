
IMAGE_SRCS := \
	image.cpp

IMAGE_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/video/image/%.o,$(IMAGE_SRCS))

$(PATH_INTERNAL_C)/parts/video/image/%.o: $(PATH_INTERNAL_C)/parts/video/image/%.cpp
	$(CXX) -O2 $(CXXFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

ifdef DEP_IMAGE_CODEC
    EXE_LIBS += $(IMAGE_OBJS)
endif

CLEAN_LIST += $(IMAGE_OBJS)
