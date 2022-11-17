#pragma once
#include "../BuildConfiguration/SAPreprocessorDefines.h"

#if USE_OPENAL_API
#include<AL/al.h>
#include<AL/alc.h>

#include <iostream>
#include "../../Tools/PlatformUtils.h"

//OpenAL error checking
#define OpenAL_ErrorCheck(message)\
{\
	ALenum error = alGetError();\
	if( error != AL_NO_ERROR)\
	{\
		std::cerr << "OpenAL Error: " << error << " with call for " << #message << std::endl;\
		STOP_DEBUGGER_HERE();\
	}\
}

#define alec(FUNCTION_CALL)\
FUNCTION_CALL;\
OpenAL_ErrorCheck(FUNCTION_CALL)


#endif //USE_OPENAL_API