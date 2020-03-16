#pragma once

//NOTE: these can be defined at a project level

#define SA_CAPTURE_SPATIAL_HASH_CELLS 1
#define SA_RENDER_DEBUG_INFO 1
#define ERROR_CHECK_GL_RELEASE 1


#define GAME_GL_MAJOR_VERSION 3
#define GAME_GL_MINOR_VERSION 3
#define ENABLE_GL_DEBUG_OUTPUT_GL43 0


#define FIND_MEMORY_LEAKS 0
#ifdef FIND_MEMORY_LEAKS
//see https://docs.microsoft.com/en-us/visualstudio/debugger/finding-memory-leaks-using-the-crt-library?view=vs-2019
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif //FIND_MEMORY_LEAKS