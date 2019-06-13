#include "SAWorldEntity.h"
#include "SAGameBase.h"
#include "SALevel.h"
#include "SALevelSubsystem.h"

namespace SA
{
	SA::Level* WorldEntity::getWorld()
	{
		GameBase& game = GameBase::get();
		return game.getLevelSubsystem().getCurrentLevel().get();
	}

}

