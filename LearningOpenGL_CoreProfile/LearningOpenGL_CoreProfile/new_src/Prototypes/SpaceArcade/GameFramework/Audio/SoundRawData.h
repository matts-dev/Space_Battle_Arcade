#pragma once
#include <vector>
#include <dr_lib/dr_wav.h>

namespace SA
{
	struct SoundRawData
	{
	public:
		unsigned int channels = 0;
		unsigned int sampleRate = 0;
		drwav_uint64 totalPCMFrameCount = 0;
		std::vector<uint16_t> pcmData;
	public:
		drwav_uint64 getTotalSamples() { return totalPCMFrameCount * channels; }
	};
}