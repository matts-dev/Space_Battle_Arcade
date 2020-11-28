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

		//#TODO #SUGGESTED perhaps this should return a reference to the world and crash if there isn't a world, is it ever valid that a world entity should exist but the world has been cleaned up?
		//also, currently this can return a world that doesn't own the entity, which is nice for transitioning between levels. But may be weird.
		//making it a reference will make somethings more fragile, but only when things are potentially being used incorrectly... I think. Leaving
		//as pointer for now because not sure how level transitions will work, will we clean up everything or allow some entities to go to the new level.

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

}

