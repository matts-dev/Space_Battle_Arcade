#pragma once



#include "../GameFramework\SASubsystemBase.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../../../Algorithms/SeparatingAxisTheorem/SATComponent.h"
#include "../Tools/DataStructures/SATransform.h"
#include <list>
#include "../Tools/DataStructures/ObjectPools.h"
#include <set>

namespace SA
{
	class ProjectileSubsystem;

	///////////////////////////////////////////////////////////////////////////////////////////////
	// User Configured projectile specification
	///////////////////////////////////////////////////////////////////////////////////////////////
	class ProjectileClassHandle
	{
		friend ProjectileSubsystem;
	public: //std::make_shared requires public ctor, friend declarations of scope calling std::make_shared isn't enough for std::make_shared
		ProjectileClassHandle(const Transform& inTransform, const sp<Model3D>& inModel)
			: collisionTransform(inTransform), model(inModel)
		{
		}

	private:
		/** Transform a unit cube into the model's single AABB
			projectiles only support simple cube collision for speed
			but the cube can be molded in fit around the model*/
		const Transform collisionTransform;
		const sp<Model3D> model;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Actual Projectile Instances; these subsystem is responsible for creating these instances
	///////////////////////////////////////////////////////////////////////////////////////////////
	struct Projectile
	{
		//TODO on the fence between designing this as mostly data managed by ProjectilSubsystem vs making this a proper OO class 
		glm::vec3 direction;
		Transform xform; 
		sp<Model3D> model;
		Transform collisionTransform;
		float speed;
		float lifetimeSec;
		float timeAlive;

		void tick(float dt_sec);
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
	// Projectile subsystem
	///////////////////////////////////////////////////////////////////////////////////////////////
	class ProjectileSubsystem : public SubsystemBase
	{
	public:
		sp<ProjectileClassHandle> createProjectileType(const sp<Model3D>& model, const Transform& AABB_unitCubeTransform);
		virtual void spawnProjectile(const glm::vec3& start, const glm::vec3& direction, const ProjectileClassHandle& projectileTypeHandle);
		
		void renderProjectiles(Shader& projectileShader) const;

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
