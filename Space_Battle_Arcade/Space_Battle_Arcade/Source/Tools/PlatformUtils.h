#pragma once

#ifdef _WIN32
	//anything needed for __debugbreak; ?
#else
	#include <signal.h> //for raise(SIGTRAP);
#endif

#ifdef _WIN32
	#define STOP_DEBUGGER_HERE()\
	__debugbreak();//step up callstack to see problem code
#else
	#define STOP_DEBUGGER_HERE()\
	raise(SIGTRAP); //step up callstack to see problem code
#endif