#pragma once
#include <optional>
#include "GameSystems/SAProjectileSystem.h"
#include "AssetConfigs/SASpawnConfig.h"
#include "../GameFramework/SAWorldEntity.h"
#include "../GameFramework/RenderModelEntity.h"
#include "../GameFramework/SACollisionUtils.h"
#include "../GameFramework/Components/GameplayComponents.h"
#include "../GameFramework/Interfaces/SAIControllable.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../Tools/DataStructures/SATransform.h"
#include "../Tools/RemoveSpecialMemberFunctionUtils.h"
#include "../Tools/DataStructures/LifetimePointer.h"

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Compile flags
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	constexpr bool bDebugAvoidance = true;
#define COMPILE_SHIP_WIDGET 0

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
	class FighterSpawnComponent;
	class RNG;
	class ShipCamera;
	class ShipPlacementEntity;
	class WorldEntity;
	class AudioEmitter;

	struct EndGameParameters;

	namespace ShipUtilLibrary
	{
		void setShipTarget(const sp<Ship>& ship, const lp<WorldEntity>& target);
		void setEngineParticleOffset(Transform& outParticleXform, const Transform& shipXform, const EngineEffectData& fx);
	}

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
			bool bEditorMode = false;
		};
	public:
		Ship(const SpawnData& spawnData);
		~Ship();

		////////////////////////////////////////////////////////
		// Interface and Virtuals
		////////////////////////////////////////////////////////
		virtual void render(Shader& shader) override;
		//virtual void onLevelRender() override;
		void onDestroyed() override;

		////////////////////////////////////////////////////////
		//IControllable
		////////////////////////////////////////////////////////
		virtual void onPlayerControlTaken(const sp<PlayerBase>& player) override;
		virtual void onPlayerControlReleased() override;
		virtual sp<CameraBase> getCamera() override;
		virtual WorldEntity* asWorldEntity() override;

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

		void enterSpawnStasis();

		////////////////////////////////////////////////////////
		//Control functions
		////////////////////////////////////////////////////////
		struct MoveTowardsPointArgs : public RemoveCopies, public RemoveMoves
		{
			MoveTowardsPointArgs(const glm::vec3& moveLoc, float dt_sec) : 
				moveLoc(moveLoc), dt_sec(dt_sec) {};

			glm::vec3 moveLoc{ 0.f };
			float dt_sec = 0.f;
			float speedMultiplier = 1.0f;
			float turnMultiplier = 1.f;
			float viscosity = 0.f;
			bool bRoll = true;
		};
		void moveTowardsPoint(const Ship::MoveTowardsPointArgs& args);
		void moveTowardsPoint(const glm::vec3& location, float dt_sec, float speedFactor = 1.0f, bool bRoll = true, float rollAmplifier = 1.f, float viscosity = 0.f); //deprecated in favor of arg version
		void roll(float rollspeed_rad, float dt_sec, float clamp_rad = 3.14f);
		void adjustSpeedFraction(float targetSpeedFactor, float dt_sec);
		void setNextFrameBoost(float targetSpeedFactor);

		bool fireProjectileAtShip(const WorldEntity& myTarget, std::optional<float> fireRadius_cosTheta = std::optional<float>{}, float shootRandomOffsetStrength = 1.f);
		void fireProjectile(class BrainKey privateKey); //#todo perhaps remove this in favor of fire in direction; #todo don't delete without cleaning up brain key
		void fireProjectileInDirection(glm::vec3 dir_n); //#todo reconsider limiting this so only brains
		void setProjectileStartPoint(glm::vec3& outProjectileStart, std::optional<glm::vec3>& traceStart, const glm::vec3& dir_n);

		inline float getAISkillLevel() { return aiSkillLevel; } //[0,1] 1 being the hardest enemy to face

		void tickVFX(); //offsets VFX effects like shield and engine

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

		/** this spawn component may not exist, you should also cache as weak pointer as the owner of the spawn component (eg carrier ship) may be destroyed
			and the component should be destroyed with its owner. This is mainly provided as a respawn mechanism. */
		fwp<FighterSpawnComponent> getOwningSpawner_cacheAsWeak() { return owningSpawnComponent; }
		void setOwningSpawnComponent(fwp<FighterSpawnComponent>& newOwningSpawnComp);

		inline float getFireCooldownSec() const { return fireCooldownSec; }
		////////////////////////////////////////////////////////
		// Projectiles 
		////////////////////////////////////////////////////////
		void setPrimaryProjectile(const sp<ProjectileConfig>& projectileConfig);
		void playerMuzzleSFX();
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
		void postctor_configureObjectivePlacements();
		////////////////////////////////////////////////////////
		// Debug
		////////////////////////////////////////////////////////
		void renderPercentageDebugWidget(float rightOffset, float percFrac) const;
	public:
		inline FighterSpawnComponent* getFighterComp() { return fighterSpawnComp; } //optimization to avoid getting gameplay component, 
		void TryTargetPlayer();
		bool isCarrierShip() const;
		inline size_t activeObjectives() const { return activePlacements; }
		inline size_t numObjectivesAtSpawn() const { return generatorEntities.size() + communicationEntities.size() + turretEntities.size(); }
		inline bool hasObjectives() const { return numObjectivesAtSpawn() != 0; }
		inline bool hasAliveObjectives() const { return activeObjectives() != 0; }
		sp<ShipPlacementEntity> getRandomObjective();
		void requestSpawnFighterAgainst(const sp<WorldEntity>& attacker);
		void setAvoidanceSensitivity(float newValue);
		const sp<const SpawnConfig>& getSpawnConfig() { return shipConfigData; }
	protected:
		virtual void postConstruct() override;
		virtual void tick(float deltatime) override;
	private:
		friend class ShipCameraTweakerWidget; //allow camera tweaker widget to modify ship properties in real time.
		void tickKinematic(float dt_sec);
		void tickSounds();
		std::optional<glm::vec3> updateAvoidance(float dt_sec);
		virtual void notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc) override;
		void doShieldFX();
		void tickShieldFX();
		void startCarrierExplosionSequence();
		void timerTick_CarrierExplosion();
		bool getAvoidanceDampenedVelocity(std::optional<glm::vec3>& avoidVec) const;
		void regenerateEngineVFX();
		void tickEngineFX();
		void destroyEngineVFX();
		void configureForEditorMode(const SpawnData& spawnData);
	private:
		void handlePlacementDestroyed(const sp<GameEntity>& placement);
		void handleSpawnStasisOver();
		void handleSpawnedEntity(const sp<Ship>& ship);
		void handleHpAdjusted(const HitPoints& old, const HitPoints& current);
		void handleGameEnding(const EndGameParameters& endParams);
#if COMPILE_CHEATS
	private:
		void cheat_oneShotPlacements();
		void cheat_destroyAllShipPlacements();
		void cheat_destroyAllGenerators();
		void cheat_turretsTargetPlayer();
		void cheat_commsTargetPlayer();
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
		float aiSkillLevel = 0.f; //[0,1], higher number means harder enemy
		float avoidanceSensitivity = 1.f; //[0,1] may be reduced in cases where AI shouldn't do avoidance -- these are very niche cases (targeting player, etc)
		sp<RNG> rng;
		size_t cachedTeamIdx;
		TeamData cachedTeamData;
		sp<const SpawnConfig> shipConfigData;
		std::vector<sp<class AvoidanceSphere>> avoidanceSpheres;
		sp<MultiDelegate<>> spawnStasisTimerDelegate = nullptr;
		sp<MultiDelegate<>> carrierDestroyTickTimerDelegate = nullptr;
		fwp<FighterSpawnComponent> owningSpawnComponent = nullptr; //spawn component used to spawn this ship instance 

		std::vector<sp<ShipPlacementEntity>> generatorEntities;
		std::vector<sp<ShipPlacementEntity>> turretEntities;
		std::vector<sp<ShipPlacementEntity>> communicationEntities;
		size_t activePlacements = 0;
		size_t activeGenerators = 0;
		size_t activeTurrets = 0;
		size_t activeCommunications = 0;

		std::vector<sp<AudioEmitter>> carrierExplosionSFX;
		size_t numExplosionSequenceTicks = 0;

		//because health is done in a component, we need a way to track the projectile that hit the ship, so that we can get information off of projectile.
		//this is cleared immediately to avoid lifetime issues
		const Projectile* transientCollidingProjectile = nullptr; 

		HitPointComponent* hpComp = nullptr;
		ShipEnergyComponent* energyComp = nullptr;
		FighterSpawnComponent* fighterSpawnComp = nullptr;

		sp<ShipCamera> shipCamera = nullptr;
#if COMPILE_SHIP_WIDGET
		sp<class Widget3D_Ship> shipWidget; 
#endif //COMPILE_SHIP_WIDGET

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
		bool bAwakeBrainAfterStasis = false;
		bool bEditorMode = false;//basically signals that this ship should not do anything

		sp<AudioEmitter> sfx_engine;
		sp<AudioEmitter> sfx_muzzle;
		sp<AudioEmitter> sfx_explosion;

		sp<ProjectileConfig> primaryProjectile;
		wp<ActiveParticleGroup> activeShieldEffect;

		//some ships may have multiple engines, hence may have multiple fire particles
		std::vector<sp<ActiveParticleGroup>> engineFireParticlesFX;

		//where projectiles spawn from, if none specified in spawn config, projectiles will spawn immediately in front of the ship.
		size_t fireLocationIndex = 0;
		std::vector<glm::vec3> projectileFireLocationOffsets;
	};
}

