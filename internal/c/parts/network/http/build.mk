
ifeq ($(OS),win)
# This version is only used for Windows, Linux and OSX use the library provided by their system

CURL_LIB := $(PATH_INTERNAL_C)/parts/network/http/libcurl.a

CURL_MAKE_FLAGS := CFG=-schannel
CURL_MAKE_FLAGS += "CURL_CFLAG_EXTRAS=-DCURL_STATICLIB -DHTTP_ONLY"
CURL_MAKE_FLAGS += CC=../../../../c_compiler/bin/gcc.exe
CURL_MAKE_FLAGS += AR=../../../../c_compiler/bin/ar.exe
CURL_MAKE_FLAGS += RANLIB=../../../../c_compiler/bin/ranlib.exe
CURL_MAKE_FLAGS += STRIP=../../../../c_compiler/bin/strip.exe
CURL_MAKE_FLAGS += libcurl_a_LIBRARY="../libcurl.a"
CURL_MAKE_FLAGS += ARCH=w$(BITS)

$(CURL_LIB):
	$(MAKE) -C $(PATH_INTERNAL_C)/parts/network/http/curl -f ./Makefile.m32 $(CURL_MAKE_FLAGS) "../libcurl.a"

.PHONY: clean-curl-lib
clean-curl-lib:
	$(MAKE) -C $(PATH_INTERNAL_C)/parts/network/http/curl -f ./Makefile.m32 $(CURL_MAKE_FLAGS) clean

CLEAN_DEP_LIST += clean-curl-lib
CLEAN_LIST += $(CURL_LIB)

CURL_EXE_LIBS := $(CURL_LIB)

CURL_CXXFLAGS += -DCURL_STATICLIB
CURL_CXXFLAGS += -I$(PATH_INTERNAL_C)/parts/network/http/include

CURL_CXXLIBS += -lcrypt32

else

CURL_EXE_LIBS :=
CURL_CXXLIBS :=
CURL_CXXLIBS += -lcurl
endif
