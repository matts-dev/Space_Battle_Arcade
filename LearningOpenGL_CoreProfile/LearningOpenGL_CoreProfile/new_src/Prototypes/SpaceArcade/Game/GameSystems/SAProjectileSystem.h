#pragma once


#include <list>
#include <set>

#include "../../GameFramework/SASystemBase.h"
#include "../../Tools/ModelLoading/SAModel.h"
#include "../../../../Algorithms/SeparatingAxisTheorem/SATComponent.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../Tools/DataStructures/ObjectPools.h"
#include "../AssetConfigs/SoundEffectSubConfig.h"

namespace SA
{
	class ProjectileSystem;
	class ProjectileConfig;
	class LevelBase;
	class WorldEntity;
	class AudioEmitter;

	struct SoundEffectSubConfig;


	///////////////////////////////////////////////////////////////////////////////////////////////
	// Actual Projectile Instances; these system is responsible for creating these instances
	//
	///////////////////////////////////////////////////////////////////////////////////////////////
	struct Projectile
	{
		//#note When adding a field to this struct, make sure it is properly initialized in ProjectileSystem::spawnProjectile

		Transform xform; 
		glm::vec3 direction_n;
		glm::vec3 hitLocation;
		glm::vec3 aabbSize;
		glm::vec3 color;
		glm::quat directionQuat; //#TODO maybe? duplicate info in xform
		glm::mat4 collisionXform;
		glm::mat4 renderXform;
		float distanceStretchScale;
		float speed;
		float lifetimeSec;
		float timeAlive;
		sp<WorldEntity> owner;
		int damage;
		size_t team;
		bool forceRelease;
		bool bHit;
		sp<const Model3D> model;
		sp<AudioEmitter> soundEmitter = nullptr;

		void tick(float dt_sec, LevelBase&);
		void stretchToDistance(float distance, float bDoCollisionTest, LevelBase& currentLevel);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Projectile Spawn Params
	///////////////////////////////////////////////////////////////////////////////////////////////
	struct ProjectileSpawnParams
	{
		glm::vec3 start;
		glm::vec3 direction;
		float lifetimeSecs = 3.0f;
		float speed = 10.0f;
		WorldEntity* Owner = nullptr;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Projectile Notification Interface
	///////////////////////////////////////////////////////////////////////////////////////////////
	struct IProjectileHitNotifiable
	{
		friend Projectile;
	private:
		virtual void notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc) = 0;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Projectile system
	///////////////////////////////////////////////////////////////////////////////////////////////
	class ProjectileSystem : public SystemBase
	{
		friend class ProjectileEditor_Level;

	public:
		struct SpawnData
		{
			glm::vec3 start;
			glm::vec3 direction_n;
			glm::vec3 color = glm::vec3(0.8f, 0.8f, 0.f);
			int damage = 25;
			size_t team = 0;
			SoundEffectSubConfig sfx;
			sp<WorldEntity> owner = nullptr;
		};
		void spawnProjectile(const SpawnData& spawnData, const ProjectileConfig& projectileTypeHandle);
		void unspawnAllProjectiles();

		void renderProjectiles(Shader& projectileShader) const;
		void renderProjectileBoundingBoxes(Shader& debugShader, const glm::vec3& color, const glm::mat4& view, const glm::mat4& perspective) const;

		sp<AudioEmitter> spawnSfxEffect(const SoundEffectSubConfig& sfx, glm::vec3 position);

	private:
		virtual void initSystem() override;
		virtual void tick(float dt_sec) override {};
		void postGameLoopTick(float dt_sec);
		void handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel);

	private:
		bool bAutomaticTickProjectiles = true;
		SP_SimpleObjectPool<Projectile> objPool;
		SP_SimpleObjectPool_RestrictedConstruction<AudioEmitter> sfxPool;

		//this is not the most cache coherent design, but wanting to get working prototype before spending time on optimizations
		//one potential cache friend optimization is to not use sp, and store in a large std::vector that does swap removes (with objects maintaing idices)
		std::set<sp<Projectile>> activeProjectiles;
	};
}
