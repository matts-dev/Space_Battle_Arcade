#pragma once
#include "../GameFramework/SAWorldEntity.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../GameFramework/RenderModelEntity.h"
#include "../GameFramework/SACollisionUtils.h"
#include "SAProjectileSystem.h"
#include "../Tools/DataStructures/SATransform.h"


namespace SA
{
	class SpawnConfig;
	class ModelCollisionInfo;
	class ShipAIBrain;
	class ProjectileConfig;
	class ActiveParticleGroup;


	struct HitPoints
	{
		int current;
		int max;
	};

	class Ship : public RenderModelEntity, public IProjectileHitNotifiable
	{
	public:
		struct SpawnData
		{
			sp<SpawnConfig> spawnConfig;
			Transform spawnTransform;
			int team = -1;
		};

	public:
		/*deprecated*/
		Ship(
			const sp<Model3D>& model,
			const Transform& spawnTransform,
			const sp<ModelCollisionInfo>& inCollisionData
			);

		/*preferred method of construction*/
		Ship(const SpawnData& spawnData);

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
		inline glm::vec4 localForwardDir_n() const { return glm::vec4(0, 0, 1, 0); }
		glm::vec4 getForwardDir();
		glm::vec4 getUpDir();
		glm::vec4 rotateLocalVec(const glm::vec4& localVec);
		float getMaxTurnAngle_PerSec() const;
		void setVelocity(glm::vec3 inVelocity) { velocity = inVelocity; }
		glm::vec3 getVelocity() { return velocity; }
		float getMaxSpeed() const { return maxSpeed; }


		////////////////////////////////////////////////////////
		// Projectiles 
		////////////////////////////////////////////////////////
		void setPrimaryProjectile(const sp<ProjectileConfig>& projectileConfig);

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
		glm::vec3 shieldColor;
		glm::vec3 shieldOffset = glm::vec3(0.f);
		float maxSpeed = 10.0f; //#TODO make part of spawn config
		HitPoints hp = { /*current*/100, /*max*/100 };
		int team;

		sp<ProjectileConfig> primaryProjectile;
		wp<ActiveParticleGroup> activeShieldEffect;
	};
}

