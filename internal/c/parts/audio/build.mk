# Include the correct files if DEPENDENCY_AUDIO_OUT is defined
ifdef DEPENDENCY_AUDIO_OUT
MINIAUDIO_SRCS += miniaudio_impl.cpp
MINIAUDIO_SRCS += audio.cpp
else
# If DEP_AUDIO_MINIAUDIO is blank, then we compile a stub that doesn't do anything
MINIAUDIO_SRCS += stub_audio.cpp
endif

MINIAUDIO_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/audio/miniaudio/%.o,$(MINIAUDIO_SRCS))

CLEAN_LIST += $(MINIAUDIO_OBJS)

$(PATH_INTERNAL_C)/parts/audio/miniaudio/%.o: $(PATH_INTERNAL_C)/parts/audio/miniaudio/%.cpp
	$(CXX) $(CXXFLAGS) -Wall $< -c -o $@

EXE_LIBS += $(MINIAUDIO_OBJS)
