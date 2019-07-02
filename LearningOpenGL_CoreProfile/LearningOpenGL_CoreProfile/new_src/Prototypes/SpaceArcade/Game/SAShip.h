#pragma once
#include "../GameFramework/SAWorldEntity.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../GameFramework/RenderModelEntity.h"
#include "../GameFramework/SACollisionUtils.h"
#include "SAProjectileSystem.h"


namespace SA
{
	class SpawnConfig;
	class ModelCollisionInfo;

	class Ship : public RenderModelEntity, public IProjectileHitNotifiable
	{
	public:
		/*deprecated*/
		Ship(
			const sp<Model3D>& model,
			const Transform& spawnTransform,
			const sp<ModelCollisionInfo>& inCollisionData
			);

		/*preferred method of construction*/
		Ship(const sp<SpawnConfig>& spawnConfig, const Transform& spawnTransform);

		virtual bool hasCollisionInfo() const override { return true; }
		virtual const sp<const ModelCollisionInfo>& getCollisionInfo() const override ;

	protected:
		virtual void postConstruct() override;
		virtual void tick(float deltatime) override;

	private:
		//const std::array<glm::vec4, 8> getWorldOBB(const glm::mat4 xform) const;
		virtual void notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc) override;

	private:
		//helper data structures
		std::vector<sp<SH::GridNode<WorldEntity>>> overlappingNodes_SH;

	private:
		up<SH::HashEntry<WorldEntity>> collisionHandle = nullptr;
		const sp<ModelCollisionInfo> collisionData;
		const sp<const ModelCollisionInfo> constViewCollisionData;

		glm::vec3 velocity;
	};
}

