#pragma once
#include "../GameFramework/EngineCompileTimeFlagsAndMacros.h"

//NOTE: these can be defined at a project level

#define SA_CAPTURE_SPATIAL_HASH_CELLS 1 & !SHIPPING_BUILD
#define SA_RENDER_DEBUG_INFO 1
#define ERROR_CHECK_GL_RELEASE 1

#define COMPILE_CHEATS 1 &!SHIPPING_BUILD

#define GAME_GL_MAJOR_VERSION 3
#define GAME_GL_MINOR_VERSION 3
#define ENABLE_GL_DEBUG_OUTPUT_GL43 0

//enable this for release versions; these are fixes that were made without fully investigating problem so that the project can be released and work
#define ENABLE_BANDAID_FIXES 0

#define FIND_MEMORY_LEAKS 0
#ifdef FIND_MEMORY_LEAKS
//see https://docs.microsoft.com/en-us/visualstudio/debugger/finding-memory-leaks-using-the-crt-library?view=vs-2019
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif //FIND_MEMORY_LEAKS

//define debug build  based on platform specific 
#ifdef _WIN32 
#ifdef _DEBUG
#define DEBUG_BUILD
#endif //_DEBUG
#endif //_WIN32 
//#TODO linux debug build
//#TODO osx debug builds