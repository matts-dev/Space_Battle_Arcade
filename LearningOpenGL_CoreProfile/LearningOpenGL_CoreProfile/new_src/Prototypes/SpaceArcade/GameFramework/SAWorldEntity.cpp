#include "SAWorldEntity.h"
#include "SAGameBase.h"
#include "SALevel.h"
#include "SALevelSystem.h"
#include "../Tools/SAUtilities.h"

namespace SA
{
	SA::LevelBase* WorldEntity::getWorld()
	{
		GameBase& game = GameBase::get();
		return game.getLevelSystem().getCurrentLevel().get();
	}

	void WorldEntity::setTransform(const Transform& inTransform)
	{
		NAN_BREAK(inTransform.position);
		NAN_BREAK(inTransform.rotQuat);
		transform = inTransform;

		if (onTransformUpdated.numBound() > 0)
		{
			onTransformUpdated.broadcast(transform);
		}
	}

	const sp<const CollisionInfo>& WorldEntity::getCollisionInfo() const
	{
		static sp<const CollisionInfo> nullCollision = nullptr;
		return nullCollision;
	}

}

