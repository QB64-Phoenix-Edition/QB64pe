# Include the correct files if DEP_AUDIO_OUT is defined
ifdef DEP_AUDIO_OUT
MINIAUDIO_SRCS += miniaudio_impl.cpp
MINIAUDIO_SRCS += audio.cpp
else
# If DEP_AUDIO_OUT is undefined, then we compile a stub that doesn't do anything
MINIAUDIO_SRCS += stub_audio.cpp
endif

MINIAUDIO_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/%.o,$(MINIAUDIO_SRCS))

CLEAN_LIST += $(MINIAUDIO_OBJS)

$(PATH_INTERNAL_C)/parts/audio/%.o: $(PATH_INTERNAL_C)/parts/audio/%.cpp
	$(CXX) $(CXXFLAGS) -Wall $< -c -o $@

EXE_LIBS += $(MINIAUDIO_OBJS)
