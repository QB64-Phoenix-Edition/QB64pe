
# We use the flat-directory compilation for FreeType as explained in:
# https://github.com/freetype/freetype/blob/master/docs/INSTALL.ANY
#
# When updating the library:
# 1. Flatten all directories inside "src" except "tools". Omit contents of "tools" entirely.
# 2. Then only copy all .c & .h files except:
#       autofit.c, bdf.c, cff.c, ftbase.c, ftcache.c, gxvalid.c, gxvfgen.c,
#       otvalid.c, pcf.c, pfr.c, pshinter.c, psnames.c, psaux.c, raster.c, sdf.c,
#       sfnt.c, smooth.c, svg.c, truetype.c, type1.c, type1cid.c, type42.c
# 3. Copy the FreeType "include" directory *without* flattening!
# 4. Include <freetype/internal/compiler-macros.h> in "ftzopen.h".
# 5. Include <freetype/config/ftstdlib.h> in "zutil.h".

FREETYPE_SRCS := $(wildcard $(PATH_INTERNAL_C)/parts/video/font/freetype/*.c)

FREETYPE_INCLUDE := -I$(PATH_INTERNAL_C)/parts/video/font/freetype/include

FREETYPE_OBJS := $(FREETYPE_SRCS:.c=.o)

FREETYPE_LIB := $(PATH_INTERNAL_C)/parts/video/font/freetype/freetype.a

FONT_SRCS := font.cpp
FONT_STUB_SRCS := stub_font.cpp

FONT_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/video/font/%.o,$(FONT_SRCS))
FONT_STUB_OBJS := $(patsubst %.cpp,$(PATH_INTERNAL_C)/parts/video/font/%.o,$(FONT_STUB_SRCS))

$(PATH_INTERNAL_C)/parts/video/font/%.o: $(PATH_INTERNAL_C)/parts/video/font/%.cpp
	$(CXX) -O2 $(CXXFLAGS) $(FREETYPE_INCLUDE) -DDEPENDENCY_CONSOLE_ONLY -w $< -c -o $@

$(PATH_INTERNAL_C)/parts/video/font/freetype/%.o: $(PATH_INTERNAL_C)/parts/video/font/freetype/%.c
	$(CC) -O3 $(CFLAGS) $(FREETYPE_INCLUDE) -DFT2_BUILD_LIBRARY -w $< -c -o $@

$(FREETYPE_LIB): $(FREETYPE_OBJS)
	$(AR) rcs $@ $(FREETYPE_OBJS)

FREETYPE_EXE_LIBS := $(FREETYPE_LIB)

CLEAN_LIST += $(FREETYPE_LIB) $(FREETYPE_OBJS) $(FONT_OBJS) $(FONT_STUB_OBJS)
