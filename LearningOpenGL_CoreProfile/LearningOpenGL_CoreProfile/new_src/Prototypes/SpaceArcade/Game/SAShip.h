#pragma once
#include <optional>
#include "SAProjectileSystem.h"
#include "AssetConfigs/SASpawnConfig.h"
#include "../GameFramework/SAWorldEntity.h"
#include "../GameFramework/RenderModelEntity.h"
#include "../GameFramework/SACollisionUtils.h"
#include "../GameFramework/Components/GameplayComponents.h"
#include "../GameFramework/Interfaces/SAIControllable.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../Tools/DataStructures/SATransform.h"

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Compile flags
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	constexpr bool bDebugAvoidance = true;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Runtime flags
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	extern bool bDrawAvoidance_debug;
	extern bool bForcePlayerAvoidance_debug;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Forward Declarations
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class SpawnConfig;
	class CollisionData;
	class ShipAIBrain;
	class ProjectileConfig;
	class ActiveParticleGroup;
	class ShipEnergyComponent;
	class RNG;
	class ShipCamera;
	class ShipPlacementEntity;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ship Class
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 	class Ship : public RenderModelEntity, public IProjectileHitNotifiable, public IControllable
	{
	public:
		const bool FIRE_PROJECTILE_ENABLED = true;
	public:
		struct SpawnData
		{
			sp<SpawnConfig> spawnConfig;
			Transform spawnTransform;
			size_t team = 0;
		};
	public:
		Ship(const SpawnData& spawnData);
		~Ship();

		////////////////////////////////////////////////////////
		// Interface and Virtuals
		////////////////////////////////////////////////////////
		virtual void draw(Shader& shader) override;
		void onDestroyed() override;

		////////////////////////////////////////////////////////
		//IControllable
		////////////////////////////////////////////////////////
		virtual void onPlayerControlTaken() override;
		virtual void onPlayerControlReleased() override;
		virtual sp<CameraBase> getCamera() override;

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
		////////////////////////////////////////////////////////
		//Control functions
		////////////////////////////////////////////////////////
		void moveTowardsPoint(const glm::vec3& location, float dt_sec, float speedFactor = 1.0f, bool bRoll = true, float rollAmplifier = 1.f, float viscosity = 0.f);
		void roll(float rollspeed_rad, float dt_sec, float clamp_rad = 3.14f);
		void adjustSpeedFraction(float targetSpeedFactor, float dt_sec);
		void setNextFrameBoost(float targetSpeedFactor);

		bool fireProjectileAtShip(const WorldEntity& myTarget, std::optional<float> fireRadius_cosTheta = std::optional<float>{}, float shootRandomOffsetStrength = 1.f) const;
		void fireProjectile(class BrainKey privateKey); //#todo perhaps remove this in favor of fire in direction; #todo don't delete without cleaning up brain key
		void fireProjectileInDirection(glm::vec3 dir_n) const; //#todo reconsider limiting this so only brains

		////////////////////////////////////////////////////////
		// Kinematics
		////////////////////////////////////////////////////////
		inline glm::vec4 localForwardDir_n() const { return glm::vec4(0, 0, 1, 0); }
		glm::vec4 getForwardDir() const;
		glm::vec4 getUpDir() const;
		glm::vec4 getRightDir() const;
		glm::vec4 rotateLocalVec(const glm::vec4& localVec);
		float getMaxTurnAngle_PerSec() const;

		glm::vec3 getVelocity(const glm::vec3 customVelocityDir_n);
		glm::vec3 getVelocity();
		glm::vec3 getVelocityDir() const { return velocityDir_n; }
		void setVelocityDir(glm::vec3 inVelocity);

		void setMaxSpeed(float inMaxSpeed) { maxSpeed = inMaxSpeed; }
		float getMaxSpeed() const { return maxSpeed; }

		void setSpeedFactor(float inSpeedFactor) { currentSpeedFactor = glm::clamp(inSpeedFactor,0.f, 1.f); }
		float getSpeed() const{ return getMaxSpeed() * currentSpeedFactor;}

		inline float getFireCooldownSec() const { return fireCooldownSec; }
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
		////////////////////////////////////////////////////////
		// avoidance
		////////////////////////////////////////////////////////
		static void setRenderAvoidanceSpheres(bool bNewRenderAvoidance);
		void debugRender_avoidance(float accumulatedAvoidanceStrength) const;
		bool hasAvoidanceSpheres() { return avoidanceSpheres.size() > 0; }
		////////////////////////////////////////////////////////
		// objectives
		////////////////////////////////////////////////////////
		void ctor_configureObjectivePlacements();
		////////////////////////////////////////////////////////
		// Debug
		////////////////////////////////////////////////////////
		void renderPercentageDebugWidget(float rightOffset, float percFrac) const;
	protected:
		virtual void postConstruct() override;
		virtual void tick(float deltatime) override;
	private:
		friend class ShipCameraTweakerWidget; //allow camera tweaker widget to modify ship properties in real time.
		void tickKinematic(float dt_sec);
		std::optional<glm::vec3> updateAvoidance(float dt_sec);
		virtual void notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc) override;
		void doShieldFX();
		bool getAvoidanceDampenedVelocity(std::optional<glm::vec3>& avoidVec) const;
	private:
		void handlePlacementDestroyed(const sp<GameEntity>& placement);
#if COMPILE_CHEATS
	private:
		void cheat_OneShotPlacements();
		void cheat_DestroyAllShipPlacements();
		void cheat_TurretsTargetPlayer();
#endif //COMPILE_CHEATS
	public:
		MultiDelegate<> onCollided;
	private: //statics
		static bool bRenderAvoidanceSpheres;
	private: //cheat flags
		bool bOneShotPlacements = false;
	private:
		//helper data structures
		std::vector<sp<SH::GridNode<WorldEntity>>> overlappingNodes_SH;
	private:
		up<SH::HashEntry<WorldEntity>> collisionHandle = nullptr; //#TODO not sure if this should be on the collision component, keeping it off the component encapsulates it better.
		const sp<CollisionData> collisionData; //#TODO perhaps just reference what's in the component so we don't have two pointers
		sp<ShipAIBrain> brain; 
		glm::vec3 velocityDir_n;
		float maxSpeed = 10.0f; //#TODO make part of spawn config
		float currentSpeedFactor = 1.0f;
		float speedGamifier = 1.0f;
		float engineSpeedChangeFactor = 1.0f; //somewhat like acceleration, but linear and gamified.
		float fireCooldownSec = 0.15f;
		HitPoints hp = { /*current*/100, /*max*/100 };
		sp<RNG> rng;

		size_t cachedTeamIdx;
		TeamData cachedTeamData;
		sp<const SpawnConfig> shipData;
		std::vector<sp<class AvoidanceSphere>> avoidanceSpheres;

		std::vector<sp<ShipPlacementEntity>> defenseEntities;
		std::vector<sp<ShipPlacementEntity>> turretEntities;
		std::vector<sp<ShipPlacementEntity>> communicationEntities;
		size_t activePlacements = 0;

		ShipEnergyComponent* energyComp = nullptr;

		sp<ShipCamera> shipCamera = nullptr;

		//boost
		const float ENERGY_BOOST_RATIO_SEC = 50.f / 1.0f; // ( energy_cost / speed_increase). eg a speed up for 1.0 could cost 50 energy per sec
		const float BOOST_DECREASE_PER_SEC = 1.0f; //speed factor per sec
		const float BOOST_RAMPUP_PER_SEC = 4.0f; //speed factor per sec
		float adjustedBoost = 1.0f;
		float targetBoost = 1.0f;
		std::optional<float> boostNextFrame;

		//flags
		bool bCollisionReflectForward:1;
		bool bEnableAvoidanceFields = true;

		sp<ProjectileConfig> primaryProjectile;
		wp<ActiveParticleGroup> activeShieldEffect;
	};
}

