#pragma once

#include "../../Tools/DataStructures/SATransform.h"

namespace SA
{

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Holds the data for a light speed jump
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct StarJumpData
	{
		float starJumpCompletionSec = 3.f;
		bool bStarJump = false;
		float starJumpPerc = 0.f;
		float totalTime = 0.f;

	public:
		void enableStarJump(bool bEnable, bool bSkipTransition = false)
		{
			bStarJump = bEnable;
			if (bSkipTransition)
			{
				starJumpPerc = bEnable ? 1.f : 0.f;
			}
		}

		void tick(float dt_sec)
		{
			starJumpPerc += (bStarJump ? 1.f : -1.f) * dt_sec / starJumpCompletionSec;
			starJumpPerc = glm::clamp(starJumpPerc, 0.f, 1.f);
			totalTime += dt_sec;
		}

		bool isStarJumpInProgress() const
		{
			bool bStarJumpInProgress = bStarJump || (!bStarJump && starJumpPerc != 0.f);
			return bStarJumpInProgress;
		}
	};

}