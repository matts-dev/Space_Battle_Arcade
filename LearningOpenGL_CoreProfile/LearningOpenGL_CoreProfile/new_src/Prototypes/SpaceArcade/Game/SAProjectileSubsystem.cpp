#include"SAProjectileSubsystem.h"
#include "..\Tools\SAUtilities.h"
#include "..\GameFramework\SAGameBase.h"
#include "..\GameFramework\SALevelSubsystem.h"
#include "..\GameFramework\SALevel.h"

namespace SA
{
	///////////////////////////////////////////////////////////////////////////////////////////////
	// Handle defining spawn properties for projectile
	///////////////////////////////////////////////////////////////////////////////////////////////

	ProjectileClassHandle::ProjectileClassHandle(const Transform& inTransform, const sp<Model3D>& inModel) : model(inModel)
	{
		using glm::vec3;

		if (inModel)
		{
			std::tuple<glm::vec3, glm::vec3> modelAABB = inModel->getAABB();

			const vec3& aabbMin = std::get<0>(modelAABB);
			const vec3& aabbMax = std::get<1>(modelAABB);
			aabbSize = aabbMax - aabbMin;

			//Below potentially supports models not aligned with z axis; see ProjectileEditor_Level::renderDeltaTimeSimulation
			//Utils::getRotationBetween(modelForward_n, { 0,0,1 })
			//const vec3 aabb = vec3(vec4(aabb, 0.f) * modelAdjustmentMat);
		}
		else { throw std::runtime_error("No model provided when creating projectile class handle"); }
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Actual Projectile Instances; these subsystem is responsible for creating these instances
	///////////////////////////////////////////////////////////////////////////////////////////////
	void Projectile::tick(float dt_sec)
	{
		//////////////////////////////////////////////////////////////
//#ifdef _DEBUG
//		static bool freezeProjectiles = false;
//		if (freezeProjectiles)
//		{
//			return;
//		}
//#endif // _DEBUG
		//////////////////////////////////////////////////////////////

		using glm::mat4; using glm::vec3; using glm::quat;

		float timeDialationFactor = 1.f;
		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSubsystem().getCurrentLevel())
		{
			timeDialationFactor = currentLevel->getTimeDialationFactor();
		}

		timeAlive += dt_sec;

		float dt_distance = dt_sec * speed;

		//TODO investigate whether some of the matrices below can be cached once (eg fire rotation? offsetDirection?)
		vec3 start = xform.position;
		vec3 end = start + dt_distance * direction_n;
		vec3 offsetDir = glm::normalize(start - end);
		float offsetLength = dt_distance / 2;
		vec3 zOffset = vec3(0, 0, offsetLength);

		vec3 modelScaleStrech(1.f);
		modelScaleStrech.z = dt_distance / aabbSize.z;

		vec3 collisionBoxScaleStretch = aabbSize;
		collisionBoxScaleStretch.z = dt_distance;

		mat4 transToEnd_rotToFireDir_zOffset = glm::translate(glm::mat4(1.f), end);
		transToEnd_rotToFireDir_zOffset = transToEnd_rotToFireDir_zOffset * glm::toMat4(directionQuat); //TODO this could also be xform.rot... which to use?
		transToEnd_rotToFireDir_zOffset = glm::translate(transToEnd_rotToFireDir_zOffset, zOffset);

		//model matrix composition: translateToEnd * rotateToFireDirection * OffsetZValueSoTipAtPoint * StretchToFitDistance
		collisionXform = glm::scale(transToEnd_rotToFireDir_zOffset, collisionBoxScaleStretch);
		renderXform = glm::scale(transToEnd_rotToFireDir_zOffset, modelScaleStrech);

		// models parallel to z
		xform.position = end;
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
		glm::vec3 projectileSystemForward(0, 0, -1);
		glm::quat spawnRotation = Utils::getRotationBetween(projectileSystemForward, direction_n);

		//todo define an argument struct to pass for spawning projectiles
		spawned->xform.position = start;
		spawned->xform.rotQuat = spawnRotation;


		spawned->distanceStretchScale = 1;
		spawned->direction_n = direction_n;
		spawned->directionQuat = spawnRotation;

		spawned->speed = projectileTypeHandle.speed;
		spawned->model = projectileTypeHandle.model;
		spawned->lifetimeSec = projectileTypeHandle.lifeTimeSec;
		spawned->aabbSize = projectileTypeHandle.aabbSize;

		spawned->timeAlive = 0.f;

		activeProjectiles.insert( spawned );
	}

	void ProjectileSubsystem::unspawnAllProjectiles()
	{
		activeProjectiles.clear();
	}

	void ProjectileSubsystem::renderProjectiles(Shader& projectileShader) const
	{
		//TODO potential optimization is to use instanced rendering to reduce draw call number
		//TODO perhaps projectile should be made a full class and encapsulate this logic

		//invariant: shader uniforms pre-configured
		for (const sp<Projectile>& projectile : activeProjectiles)
		{
			projectileShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(projectile->renderXform));
			projectile->model->draw(projectileShader);
		}
	}

	void ProjectileSubsystem::renderProjectileBoundingBoxes(Shader& debugShader, const glm::vec3& color, const glm::mat4& view, const glm::mat4& perspective) const
	{
		for (const sp<Projectile>& projectile : activeProjectiles)
		{
			Utils::renderDebugWireCube(debugShader, color, projectile->collisionXform, view, perspective);
		}
	}

}

