#pragma once
#include "../../GameFramework/RenderModelEntity.h"
#include "GameFramework/SAGameEntity.h"
#include "ReferenceCode/OpenGL/Algorithms/SpatialHashing/SpatialHashingComponent.h"


namespace SA
{
	class SpawnConfig;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// represents a static mesh in world that AI will avoid
	// Can be used for asteroids, or just general space stations.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class AvoidMesh : public RenderModelEntity
	{
	private:
		using Parent = RenderModelEntity;
	public:
		static void setRenderAvoidanceSpheres(bool bRender);
	public:
		struct SpawnData
		{
			sp<SpawnConfig> spawnConfig;
			Transform spawnTransform;
			bool bEditorMode = false;
		};
		AvoidMesh(const SpawnData& spawnData);
	public:
		virtual void postConstruct() override;
		virtual void render(Shader& shader) override;
		virtual void setTransform(const Transform& inTransform) override;
	private:
		void updateAvoidanceSpheres();
		void updateCollision();
	private:
		bool bEditorMode = false;
		std::vector<sp<class AvoidanceSphere>> avoidanceSpheres;
		sp<class SpawnConfig> spawnConfig = nullptr;
		up<SH::HashEntry<WorldEntity>> collisionHandle = nullptr;
		const sp<CollisionData> collisionData = nullptr; 
	};
}