// Although, QOA files use lossy compression, they can be quite large (like ADPCM compressed audio)
// We certainly do not want to load these files in memory in one go
// So, we'll simply exclude the stdio one-shot read/write APIs
#define QOA_NO_STDIO
#define QOA_IMPLEMENTATION
#include "qoa.h"
