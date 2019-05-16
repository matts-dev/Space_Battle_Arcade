#include"SAProjectileSubsystem.h"
#include "..\Tools\SAUtilities.h"
#include "..\GameFramework\SAGameBase.h"

namespace SA
{
	///////////////////////////////////////////////////////////////////////////////////////////////
	// Actual Projectile Instances; these subsystem is responsible for creating these instances
	///////////////////////////////////////////////////////////////////////////////////////////////
	void Projectile::tick(float dt_sec)
	{
		xform.position += dt_sec * speed * direction;
		timeAlive += dt_sec;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Projectile subsystem
	///////////////////////////////////////////////////////////////////////////////////////////////

	void ProjectileSubsystem::postGameLoopTick(float dt_sec)
	{
		auto iter = std::begin(activeProjectiles);
		auto end = std::end(activeProjectiles);
		while (iter != end)
		{
			//since projectiles may expire, we want need to remove them from the set of active projectiles
			//we can do this while iterating over std::set, but we need to take care. Removing an interator
			//only invalidates that iterator, but if it is invalidated we cannot get the next iterator from it.
			//So, we copy the iterator and immediately get the next iterator; meaning removing the copy doesn't
			//affect our ability to get the next iterator
			auto iterCopy = iter;
			++iter; 

			const sp<Projectile>& projectile = *iterCopy;
			projectile->tick(dt_sec);

			if (projectile->timeAlive > projectile->lifetimeSec)
			{
				
				//note: this projectile will keep any sp alive, so clear before release if needed
				objPool.releaseInstance(projectile);

				//removing iterator from set does not invalidate other iterators; 
				//IMPORANT: this must after releasing to pool, otherwise the smart pointer will be deleted
				activeProjectiles.erase(iterCopy);
			}
		}
	}

	void ProjectileSubsystem::initSystem()
	{
		//align projectiles with camera
		GameBase::get().PostGameloopTick.addStrongObj(sp_this(), &ProjectileSubsystem::postGameLoopTick);
	}

	sp<ProjectileClassHandle> ProjectileSubsystem::createProjectileType(const sp<Model3D>& model, const Transform& AABB_unitCubeTransform)
	{
		//this method met not be necessary after all; clients should be free to just create projectile handles when ever
		sp<ProjectileClassHandle> projectileType = new_sp<ProjectileClassHandle>(AABB_unitCubeTransform, model);

		return projectileType;
	}

	void ProjectileSubsystem::spawnProjectile(const glm::vec3& start, const glm::vec3& direction_n, const ProjectileClassHandle& projectileTypeHandle)
	{
		sp<Projectile> spawned = objPool.getInstance();

		//note there may some optimized functions in glm to do this work
		glm::vec3 modelForward_n(0, 0, 1);//TODO make this part of the projectile type handle
		glm::vec3 rotAxis = glm::normalize(glm::cross(modelForward_n, direction_n)); //theoretically, I don't think I need to normalize if both normal; but generally I normalize the result of xproduct
		float cosTheta = glm::dot(modelForward_n, direction_n);
		float rotDegreesRadians = glm::acos(cosTheta);
		glm::quat spawnRotation = glm::angleAxis(rotDegreesRadians, rotAxis);

		//case when vectors are 180 apart; 
		if (Utils::float_equals(cosTheta, -1.0f))
		{
			//if tail end and front of projectile are not the same, we need a 180 rotation around ?any? axis
			glm::vec3 temp = Utils::getDifferentVector(modelForward_n);
			glm::vec3 rotAxisFor180 = glm::normalize(cross(modelForward_n, temp));

			spawnRotation = glm::angleAxis(glm::pi<float>(), rotAxisFor180); 
		}

		//todo define a argument struct to pass for spawning projectiles
		spawned->xform.position = start;
		spawned->xform.rotQuat = spawnRotation;
		spawned->direction = direction_n;
		spawned->speed = 200.0f;
		spawned->collisionTransform = projectileTypeHandle.collisionTransform;
		spawned->model = projectileTypeHandle.model;
		spawned->timeAlive = 0.f;
		spawned->lifetimeSec = 1.f;

		activeProjectiles.insert( spawned );
	}

	void ProjectileSubsystem::renderProjectiles(Shader& projectileShader) const
	{
		//TODO potential optimization is to use instanced rendering to reduce draw call number

		//invariant: shader uniforms preconfigured
		for (const sp<Projectile>& projectile : activeProjectiles)
		{
			projectileShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(projectile->xform.getModelMatrix()));
			projectile->model->draw(projectileShader);
		}
	}
}

