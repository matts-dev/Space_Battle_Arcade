#include "RenderModelEntity.h"
#include "SAWorldEntity.h"

namespace SA
{
	const sp<const ModelCollisionInfo>& RenderModelEntity_NoCollision::getCollisionInfo() const
	{
		return WorldEntity::getCollisionInfo();
	}

	bool RenderModelEntity_NoCollision::hasCollisionInfo() const
	{
		return false;
	}
}
