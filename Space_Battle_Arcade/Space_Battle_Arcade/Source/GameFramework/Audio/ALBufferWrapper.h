#pragma once
#include "GameFramework/BuildConfiguration/SAPreprocessorDefines.h"

#if USE_OPENAL_API

#include <AL/al.h>

namespace SA
{
	struct ALBufferWrapper
	{
		ALuint buffer = 0;
		float durationSec = 0.f;
	};
}
#endif //USE_OPENAL_API
