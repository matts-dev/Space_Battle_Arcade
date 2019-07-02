#include "SAWorldEntity.h"
#include "SAGameBase.h"
#include "SALevel.h"
#include "SALevelSystem.h"

namespace SA
{
	SA::LevelBase* WorldEntity::getWorld()
	{
		GameBase& game = GameBase::get();
		return game.getLevelSystem().getCurrentLevel().get();
	}

	const sp<const ModelCollisionInfo>& WorldEntity::getCollisionInfo() const
	{
		static sp<const ModelCollisionInfo> nullCollision = nullptr;
		return nullCollision;
	}

}

