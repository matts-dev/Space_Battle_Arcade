#pragma once
#include "SAGameEntity.h"
#include "Interfaces\SATickable.h"


namespace SA
{
	class LevelSubsystem;

	/** Base class for a level object */
	class Level : public GameEntity, public Tickable
	{
		friend LevelSubsystem;

	private:
		virtual void startLevel() = 0;
		virtual void endLevel() = 0;
	};
}