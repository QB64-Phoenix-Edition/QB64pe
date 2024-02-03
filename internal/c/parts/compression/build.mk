
MINIZ_SRCS := \
	miniz.c

COMPRESSION_SRCS := \
	compression.cpp

MINIZ_OBJS := $(patsubst %.c,$(PATH_INTERNAL_C)/parts/compression/%.o,$(MINIZ_SRCS))

COMPRESSION_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/compression/%.o,$(COMPRESSION_SRCS))

$(PATH_INTERNAL_C)/parts/compression/%.o: $(PATH_INTERNAL_C)/parts/compression/%.c
	$(CC) -O2 $(CFLAGS) -DDEPENDENCY_CONSOLE_ONLY -DMINIZ_NO_STDIO -DMINIZ_NO_TIME -DMINIZ_NO_ARCHIVE_APIS -DMINIZ_NO_ARCHIVE_WRITING_APIS -Wall $< -c -o $@

$(PATH_INTERNAL_C)/parts/compression/%.o: $(PATH_INTERNAL_C)/parts/compression/%.cpp
	$(CXX) -O2 $(CXXFLAGS) -DDEPENDENCY_CONSOLE_ONLY -Wall $< -c -o $@

CLEAN_LIST += $(MINIZ_OBJS) $(COMPRESSION_OBJS)
