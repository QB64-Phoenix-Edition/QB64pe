
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

COMPRESSION_LIB := $(PATH_INTERNAL_C)/parts/compression/compression.a

$(COMPRESSION_LIB): $(MINIZ_OBJS) $(COMPRESSION_OBJS)
	$(AR) rcs $@ $(MINIZ_OBJS) $(COMPRESSION_OBJS)

EXE_LIBS += $(COMPRESSION_LIB)

CLEAN_LIST += $(COMPRESSION_LIB) $(MINIZ_OBJS) $(COMPRESSION_OBJS)
