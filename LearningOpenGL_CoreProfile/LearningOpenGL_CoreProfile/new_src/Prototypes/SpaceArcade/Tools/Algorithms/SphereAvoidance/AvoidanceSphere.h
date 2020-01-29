#pragma once

#include <array>

#include "../../../GameFramework/SAGameEntity.h"
#include "../../DataStructures/SATransform.h"

namespace SA
{
	//TODO #scenenodes
	class AvoidanceSphere : public GameEntity
	{
	public:
		AvoidanceSphere(float radius, const glm::vec3& position); 
	public:
		/** Sphere's AABB will be its OBB */
		const std::array<glm::vec4, 8> getOBB() { return AABB; }
		void render() const;
		/** position setting enforced rather than setting of transform to make sure radius is properly accounted for. */
		void setPosition(glm::vec3 position);
	private:
		void applyXform();
	private:
		Transform xform;
		float radius;
		std::array<glm::vec4, 8> AABB;
		glm::vec3 debugColor{ 1,0,0 };
	};
}