#pragma once
#include "GameFramework/SAGameEntity.h"

namespace SA
{
	class DifficultyConfig : public GameEntity
	{
	public:
		float GameModeObjectiveBalancing() const { return gameModeObjectiveBalancing; }
		void GameModeObjectiveBalancing(float val) { gameModeObjectiveBalancing = val; }

	private: //difficulty settings are on a range from [0,1], where 0 is easiest and 1 is hardest
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// all difficulty settings grouped in a single place for easy designing
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		float gameModeObjectiveBalancing = 0.f;
	};

}
