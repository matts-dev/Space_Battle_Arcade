#pragma once

#include <array>

#include "../../DataStructures/SATransform.h"
#include "../../../GameFramework/SAGameEntity.h"
#include "../../../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"

namespace SA
{
	class LevelBase;

	//TODO #scenenodes
	class AvoidanceSphere : public GameEntity
	{
	public:
		AvoidanceSphere(float radius, const glm::vec3& position); 
		virtual void postConstruct() override;
	public:
		/** Sphere's AABB will be its OBB */
		const std::array<glm::vec4, 8> getOBB() { return AABB; }
		void render() const;
		/** position setting enforced rather than setting of transform to make sure radius is properly accounted for. */
		void setPosition(glm::vec3 position);
		void setRadius(float radius);
		void setParentXform(const glm::mat4& newParentXform);
	private:
		void handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel);
		void handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel);
		glm::mat4 getWorldMat() const;
		void applyXform();
	private:
		glm::vec3 debugColor{ 1,0,0 };
	private:
		up<SH::HashEntry<AvoidanceSphere>> myGridEntry;
		SH::SpatialHashGrid<AvoidanceSphere>* cachedAvoidanceGrid; //NOTE: will be dangling pointer if myGridEntry is null
		glm::mat4 parentXform{ 1.0f };
		Transform localXform;
		float radius;
		std::array<glm::vec4, 8> AABB;
	};
}