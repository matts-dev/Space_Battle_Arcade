#pragma once
#include "../../GameFramework/RenderModelEntity.h"
#include "../../GameFramework/SAGameEntity.h"
#include "../../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"


namespace SA
{
	class SpawnConfig;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// represents a static asteroid that AI will avoid
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Asteroid : public RenderModelEntity
	{
	private:
		using Parent = RenderModelEntity;
	public:
		static void setRenderAvoidanceSpheres(bool bRender);
		struct SpawnData
		{
			sp<SpawnConfig> spawnConfig;
			Transform spawnTransform;
		};
		Asteroid(const SpawnData& spawnData);
	public:
		virtual void postConstruct() override;
		virtual void render(Shader& shader) override;
		virtual void setTransform(const Transform& inTransform) override;
	private:
		void updateAvoidanceSpheres();
		void updateCollision();
	private:
		std::vector<sp<class AvoidanceSphere>> avoidanceSpheres;
		sp<class SpawnConfig> spawnConfig = nullptr;
		up<SH::HashEntry<WorldEntity>> collisionHandle = nullptr;
		const sp<CollisionData> collisionData = nullptr; 
	};
}