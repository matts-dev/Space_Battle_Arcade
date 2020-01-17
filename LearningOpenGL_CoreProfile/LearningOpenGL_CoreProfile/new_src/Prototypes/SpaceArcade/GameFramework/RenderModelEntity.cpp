#include "RenderModelEntity.h"
#include "SAWorldEntity.h"
#include "../Rendering/SAShader.h"

namespace SA
{
	const sp<const CollisionInfo>& RenderModelEntity_NoCollision::getCollisionInfo() const
	{
		return WorldEntity::getCollisionInfo();
	}

	bool RenderModelEntity_NoCollision::hasCollisionInfo() const
	{
		return false;
	}

	void RenderModelEntity::draw(Shader& shader)
	{
		getModel()->draw(shader);
	}

}
