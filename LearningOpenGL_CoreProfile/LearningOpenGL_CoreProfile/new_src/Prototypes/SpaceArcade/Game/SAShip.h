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
	class ShipAIBrain;

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

	public: 
		////////////////////////////////////////////////////////
		// Functions for AI/Player brains
		////////////////////////////////////////////////////////
		void fireProjectile(class BrainKey privateKey);

		////////////////////////////////////////////////////////
		//AI
		////////////////////////////////////////////////////////
		template <typename BrainType>
		void spawnNewBrain()
		{
			static_assert(std::is_base_of<ShipAIBrain, BrainType>::value, "BrainType must be derived from ShipAIBrain");
			setNewBrain(new_sp<BrainType>(sp_this()));
		}
		void setNewBrain(const sp<ShipAIBrain> newBrain, bool bStartNow = true);

		////////////////////////////////////////////////////////
		//Collision
		////////////////////////////////////////////////////////
		virtual bool hasCollisionInfo() const override { return true; }
		virtual const sp<const ModelCollisionInfo>& getCollisionInfo() const override ;

		////////////////////////////////////////////////////////
		// Kinematics
		////////////////////////////////////////////////////////
		glm::vec4 getForwardDir();
		void setVelocity(glm::vec3 inVelocity) { velocity = inVelocity; }

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
		sp<ShipAIBrain> brain; 
		glm::vec3 velocity;
	};
}

