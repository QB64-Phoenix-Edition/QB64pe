#include "../extras/stb/stb_vorbis.h"

#ifdef __APPLE__
#    define MA_NO_RUNTIME_LINKING
#endif
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
