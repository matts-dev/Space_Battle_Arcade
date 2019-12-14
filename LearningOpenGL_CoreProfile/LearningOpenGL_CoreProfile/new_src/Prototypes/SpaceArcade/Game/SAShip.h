#pragma once
#include "../GameFramework/SAWorldEntity.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../GameFramework/RenderModelEntity.h"
#include "../GameFramework/SACollisionUtils.h"
#include "SAProjectileSystem.h"
#include "../Tools/DataStructures/SATransform.h"
#include "AssetConfigs/SASpawnConfig.h"
#include "../GameFramework/Components/GameplayComponents.h"
#include <optional>


namespace SA
{
	class SpawnConfig;
	class ModelCollisionInfo;
	class ShipAIBrain;
	class ProjectileConfig;
	class ActiveParticleGroup;
	class ShipEnergyComponent;
	class RNG;

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
			size_t team = 0;
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
		// Interface and Virtuals
		////////////////////////////////////////////////////////
		virtual void draw(Shader& shader) override;

		////////////////////////////////////////////////////////
		//AI
		////////////////////////////////////////////////////////
	public:
		template <typename BrainType>
		void spawnNewBrain()
		{
			//automatically give the brain the current shp
			static_assert(std::is_base_of<ShipAIBrain, BrainType>::value, "BrainType must be derived from ShipAIBrain");
			if (BrainComponent* brainComp = getGameComponent<BrainComponent>())
			{
				brainComp->spawnNewBrain<BrainType>(sp_this());
			}
		}

	public:

		////////////////////////////////////////////////////////
		//Control functions
		////////////////////////////////////////////////////////
		void moveTowardsPoint(const glm::vec3& location, float dt_sec, float speedFactor = 1.0f, bool bRoll = true, float rollAmplifier = 1.f);
		void roll(float roll_rad, float dt_sec);
		void adjustSpeedFraction(float targetSpeedFactor, float dt_sec);
		void setNextFrameBoost(float targetSpeedFactor);

		bool fireProjectileAtShip(const WorldEntity& myTarget, std::optional<float> fireRadius_cosTheta = std::optional<float>{}, float shootRandomOffsetStrength = 1.f) const;
		void fireProjectile(class BrainKey privateKey); //#todo perhaps remove this in favor of fire in direction; #todo don't delete without cleaning up brain key
		void fireProjectileInDirection(glm::vec3 dir_n) const; //#todo reconsider limiting this so only brains

		////////////////////////////////////////////////////////
		//Collision
		////////////////////////////////////////////////////////
		virtual bool hasCollisionInfo() const override { return true; }
		virtual const sp<const ModelCollisionInfo>& getCollisionInfo() const override ;

		////////////////////////////////////////////////////////
		// Kinematics
		////////////////////////////////////////////////////////
		inline glm::vec4 localForwardDir_n() const { return glm::vec4(0, 0, 1, 0); }
		glm::vec4 getForwardDir() const;
		glm::vec4 getUpDir() const;
		glm::vec4 getRightDir() const;
		glm::vec4 rotateLocalVec(const glm::vec4& localVec);
		float getMaxTurnAngle_PerSec() const;

		void setVelocityDir(glm::vec3 inVelocity);
		glm::vec3 getVelocityDir() { return velocityDir_n; }
		glm::vec3 getVelocity();

		void setMaxSpeed(float inMaxSpeed) { maxSpeed = inMaxSpeed; }
		float getMaxSpeed() const { return maxSpeed; }

		void setSpeedFactor(float inSpeedFactor) { currentSpeedFactor = glm::clamp(inSpeedFactor,0.f, 1.f); }
		float getSpeed() const{ return getMaxSpeed() * currentSpeedFactor;}

		////////////////////////////////////////////////////////
		// Projectiles 
		////////////////////////////////////////////////////////
		void setPrimaryProjectile(const sp<ProjectileConfig>& projectileConfig);


		////////////////////////////////////////////////////////
		// teams
		////////////////////////////////////////////////////////
		void updateTeamDataCache();
		size_t getTeam() { return cachedTeamIdx; }
		void handleTeamChanged(size_t oldTeamId, size_t newTeamId);

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
		glm::vec3 velocityDir_n;
		glm::vec3 shieldOffset = glm::vec3(0.f);
		float maxSpeed = 10.0f; //#TODO make part of spawn config
		float currentSpeedFactor = 1.0f;
		float speedGamifier = 1.0f;
		float engineSpeedChangeFactor = 0.1f; //somewhat like acceleration, but linear and gamified.
		HitPoints hp = { /*current*/100, /*max*/100 };
		sp<RNG> rng;

		size_t cachedTeamIdx;
		TeamData cachedTeamData;
		sp<const SpawnConfig> shipData;

		ShipEnergyComponent* energyComp = nullptr;

		//boost
		const float ENERGY_BOOST_RATIO_SEC = 50.f / 1.0f; // ( energy_cost / speed_increase). eg a speed up for 1.0 could cost 50 energy per sec
		const float BOOST_DECREASE_PER_SEC = 1.0f; //speed factor per sec
		const float BOOST_RAMPUP_PER_SEC = 4.0f; //speed factor per sec
		float adjustedBoost = 1.0f;
		float targetBoost = 1.0f;
		std::optional<float> boostNextFrame;

		sp<ProjectileConfig> primaryProjectile;
		wp<ActiveParticleGroup> activeShieldEffect;
	};
}

