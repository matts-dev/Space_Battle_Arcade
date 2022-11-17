#pragma once


#ifdef _WIN32
#define STOP_DEBUGGER_HERE()\
__debugbreak();//step up callstack to see problem code
#else
//add platform specific debug reak statements as they exist, otherwise do nothing
#endif