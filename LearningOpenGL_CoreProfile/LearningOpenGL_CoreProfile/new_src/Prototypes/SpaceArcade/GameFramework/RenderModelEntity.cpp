#include "RenderModelEntity.h"
#include "SAWorldEntity.h"
#include "../Rendering/SAShader.h"

namespace SA
{
	void RenderModelEntity::draw(Shader& shader)
	{
		getModel()->draw(shader);
	}
}
