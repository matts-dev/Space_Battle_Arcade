#include "RenderModelEntity.h"
#include "SAWorldEntity.h"
#include "../Rendering/SAShader.h"

namespace SA
{
	void RenderModelEntity::replaceModel(const sp<Model3D>& newModel)
	{
		model = newModel;
		constView = newModel;
	}

	void RenderModelEntity::draw(Shader& shader)
	{
		getModel()->draw(shader);
	}
}
