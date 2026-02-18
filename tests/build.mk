
TESTS :=

TEST_CFLAGS-y := -I'./tests/c/include' \
			   -I$(PATH_LIBQB)/include \
			   -g -std=gnu++11

TEST_CFLAGS-$(win) += -mconsole -static-libgcc -static-libstdc++

TEST_DEF_OBJS := tests/c/test.o

# Defines the list of test sets
TESTS += buffer
TESTS += http

# Describe how to build each test
buffer.src-y := ./tests/c/buffer.cpp \
				$(PATH_LIBQB)/src/buffer.cpp

http.src-y := ./tests/c/http.cpp \
				$(PATH_LIBQB)/src/qb_http.cpp \
				$(PATH_LIBQB)/src/buffer.cpp \
				$(PATH_LIBQB)/src/threading-$(PLATFORM).cpp \
				$(PATH_LIBQB)/src/threading.cpp

http.cflags-y := $(CURL_CXXFLAGS)
http.libs-y := $(CURL_CXXLIBS)
http.exe-libs-y := $(CURL_EXE_LIBS)

http.libs-$(lnx) += -lpthread
http.libs-$(win) += -lws2_32


TEST_OBJS := $(TEST_DEF_OBJS)
TEST_OBJS += $(foreach test,$(TESTS),$(filter ./tests/c/%,$($(test)).objs-y))

TEST_TESTS :=

define TEST_template
TEST_TESTS += ./tests/exes/cpp/$(1)_test$(EXTENSION)
tests/exes/cpp/$(1)_test$(EXTENSION): $$(TEST_DEF_OBJS) $$($(1).src-y) $$($(1).exe-libs-y) | tests/exes/cpp
	$$(CXX) $$(TEST_CFLAGS-y) $$($(1).cflags-y) $$^ -o $$@ $$($(1).exe-libs-y) $$($(1).libs-y)
endef

$(foreach test,$(TESTS),$(eval $(call TEST_template,$(test))))

CLEAN_LIST += $(TEST_OBJS)

tests/exes:
	$(MKDIR) $(call FIXPATH,$@)

tests/exes/cpp: | tests/exes
	$(MKDIR) $(call FIXPATH,$@)

PHONY += build-tests
build-tests: $(TEST_DEF_OBJS) $(TEST_TESTS)
