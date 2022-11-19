#pragma once
#include "GameFramework/SARandomNumberGenerationSystem.h"

namespace SA
{
	class RNG;

	class PerfChoiceChooserHelper
	{
	public:
		inline size_t add(size_t newOptionSize)
		{
			total += newOptionSize;
			return newOptionSize;
		}
		inline bool inRange(size_t value, size_t range)
		{
			size_t correctedRange = range + adjustableSize;
			bool bInRange = value >= adjustableSize && value < correctedRange;

			adjustableSize += range;
			return bInRange;
		}
		inline void reset() { adjustableSize = 0; }
		inline size_t getTotalRange() const { return total; }
		inline size_t getChoice(RNG& rng) 
		{ 
			reset();
			return rng.getInt<size_t>(0, getTotalRange() - 1); 
		}
	private:
		size_t total;
		size_t adjustableSize = 0;
	};



}