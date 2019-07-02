#pragma once


#include <list>
#include <set>

#include "../GameFramework/SASystemBase.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../../../Algorithms/SeparatingAxisTheorem/SATComponent.h"
#include "../Tools/DataStructures/SATransform.h"
#include "../Tools/DataStructures/ObjectPools.h"

namespace SA
{
	class ProjectileSystem;
	class LevelBase;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// User Configured projectile specification
	///////////////////////////////////////////////////////////////////////////////////////////////
	class ProjectileClassHandle
	{
		friend ProjectileSystem;
	public: //std::make_shared requires public ctor, friend declarations of scope calling std::make_shared isn't enough for std::make_shared
		ProjectileClassHandle(const Transform& inTransform, const sp<Model3D>& inModel);

		glm::vec3 getAABBSize() { return aabbSize; }
	public:
		sp<Model3D> model;
		float speed = 250.0f;
		float lifeTimeSec = 1.f;
	private:
		glm::vec3 aabbSize;
	};

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
		glm::quat directionQuat; //TODO maybe? duplicate info in xform
		glm::vec3 aabbSize;
		glm::mat4 collisionXform;
		glm::mat4 renderXform;
		float distanceStretchScale;
		float speed;
		float lifetimeSec;
		float timeAlive;
		bool forceRelease;
		bool bHit;
		sp<const Model3D> model;

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
		sp<ProjectileClassHandle> createProjectileType(const sp<Model3D>& model, const Transform& AABB_unitCubeTransform);
		void spawnProjectile(const glm::vec3& start, const glm::vec3& direction, const ProjectileClassHandle& projectileTypeHandle);
		void unspawnAllProjectiles();

		void renderProjectiles(Shader& projectileShader) const;
		void renderProjectileBoundingBoxes(Shader& debugShader, const glm::vec3& color, const glm::mat4& view, const glm::mat4& perspective) const;

	private:
		virtual void tick(float dt_sec) override {};
		void postGameLoopTick(float dt_sec);
		virtual void initSystem() override;

	private:
		bool bAutomaticTickProjectiles = true;
		SP_SimpleObjectPool<Projectile> objPool;

		//this is not the most cache coherent design, but wanting to get working prototype before spending time on optimizations
		//one potential cache friend optimization is to not use sp, and store in a large std::vector that does swap removes (with objects maintaing idices)
		std::set<sp<Projectile>> activeProjectiles;
	};
}
