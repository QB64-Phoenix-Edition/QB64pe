#include "../extras/stb/stb_vorbis.h"

// Due to the way miniaudio links to macOS frameworks at runtime, the application may not pass Apple's notarization process. :(
// So, we will avoid runtime linking on macOS. See this discussion for more info: https://github.com/mackron/miniaudio/issues/203
#ifdef __APPLE__
#    define MA_NO_RUNTIME_LINKING
#endif
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
