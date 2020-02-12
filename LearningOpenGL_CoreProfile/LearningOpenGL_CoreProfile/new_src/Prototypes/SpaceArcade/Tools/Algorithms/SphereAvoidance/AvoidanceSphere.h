#pragma once

#include <array>

#include "../../DataStructures/SATransform.h"
#include "../../../GameFramework/SAGameEntity.h"
#include "../../../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../../DataStructures/AdvancedPtrs.h"

namespace SA
{
	class LevelBase;

	constexpr bool bCompileDebugDebugSpatialHashVisualizations = true;

	//TODO #scenenodes
	class AvoidanceSphere : public GameEntity
	{
	public:
		AvoidanceSphere(float radius, const glm::vec3& position, const sp<GameEntity>& owner = nullptr); 
		virtual void postConstruct() override;
	public:
		/** Sphere's AABB will be its OBB */
		const std::array<glm::vec4, 8> getOBB() { return AABB; }
		void render() const;
		/** position setting enforced rather than setting of transform to make sure radius is properly accounted for. */
		void setPosition(glm::vec3 position);
		void setRadius(float radius);
		void setParentXform(const glm::mat4& newParentXform);
		const fwp<GameEntity>& getOwner() { return owningEntity; }
		glm::vec3 getWorldPosition() const;//#TODO_minor perhaps just use glm::vec4?
		float getRadius() const { return radius; } //#TODO perhaps radius should be defined by transforms too (eg transforming a radius vector)
		float getRadiusFractForMaxAvoidance() const { return radiusFractForMaxAvoidance; }
	private:
		void handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel);
		void handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel);
		glm::mat4 getWorldMat() const;
		glm::mat4 getOOBWorldMat() const;
		void applyXform();
		void cacheProperties(const glm::mat4& sphereModel_m);
	public:
		/** Note: this step is also controlled by a compiler flag */
		static bool bDebugSpatialHashVisualization;
	private:
		glm::vec3 debugColor{ 1,0,0 };
	private:
		glm::vec3 cachedWorldPos{ 0.f };
	private:
		up<SH::HashEntry<AvoidanceSphere>> myGridEntry;
		SH::SpatialHashGrid<AvoidanceSphere>* cachedAvoidanceGrid; //NOTE: will be dangling pointer if myGridEntry is null
		glm::mat4 parentXform{ 1.0f };
		Transform localXform;
		Transform localOOBXform;	//requires scale tweaking to make cube match radius of sphere
		fwp<GameEntity> owningEntity;
		float radius;
		float radius2;
		float radiusFractForMaxAvoidance = 0.8f; // [0,1] - eg 0.66 means when something in 1/3 into the avoidance sphere, it exhibits the maximum force to get out of the sphere
		std::array<glm::vec4, 8> AABB;
	};
}