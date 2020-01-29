#include "AvoidanceSphere.h"
#include "../../../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../../../GameFramework/SADebugRenderSystem.h"
#include "../../../GameFramework/SAGameBase.h"

namespace SA
{
	using namespace glm;

	AvoidanceSphere::AvoidanceSphere(float radius, const glm::vec3& position) : 
		radius(radius)
	{
		xform.position = position;
		applyXform();
	}

	void AvoidanceSphere::render() const
	{
		glm::mat4 model = xform.getModelMatrix();

		DebugRenderSystem& debugRenderSystem = GameBase::get().getDebugRenderSystem();
		debugRenderSystem.renderSphere(model, debugColor);
	}

	void AvoidanceSphere::setPosition(glm::vec3 position)
	{
		//#TODO #scenenodes #partial_transform would be better to have this as a scene node that accepts a partial transform that doesn't include scale.
		//or setting position could just be setting local position
		xform.position = position;
		applyXform();
	}

	void AvoidanceSphere::applyXform()
	{
		static const float scaleUpFactor = 1.f / glm::length(vec3(SH::AABB[0])); //correct for cube verts not being length 1 from origin.
		float cubeScaleUp = radius * scaleUpFactor; //unit cube
		xform.scale = glm::vec3(cubeScaleUp);
		glm::mat4 model_m = xform.getModelMatrix();
		
		for (size_t vert = 0; vert < SH::AABB.size(); ++vert)
		{
			AABB[vert] = model_m * SH::AABB[vert];
		}
	}

}

