#include "SAProjectileSystem.h"
#include "../Tools/SAUtilities.h"
#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SALevelSystem.h"
#include "../GameFramework/SALevel.h"
#include "../GameFramework/SACollisionUtils.h"
#include "../../../Algorithms/SeparatingAxisTheorem/SATComponent.h"

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
	// Actual Projectile Instances; these system is responsible for creating these instances
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

		using glm::mat4; using glm::vec3; using glm::quat; using glm::vec4;

		float timeDialationFactor = 1.f;
		//#optimize perhaps cache level system in local static for perf; profiler may cacn help to determine if this is worth it
		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			timeDialationFactor = currentLevel->getTimeDialationFactor();

			timeAlive += dt_sec;

			float dt_distance = dt_sec * speed;

			stretchToDistance(dt_distance, true, *currentLevel);
		}
	}

	void Projectile::stretchToDistance(float dt_distance, float bDoCollisionTest, LevelBase& currentLevel)
	{
		using glm::mat4; using glm::vec3; using glm::quat; using glm::vec4;

		//if the projectile hit its target, change logic so it just stretches to hit location
		if (bHit)
		{
			vec3 toHit = hitLocation - xform.position;
			dt_distance = glm::length(toHit);
			++ticksSinceHit;
		}

		//#optimize investigate whether some of the matrices below can be cached once (eg fire rotation? offsetDirection?)
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

		//collision check
		if (bDoCollisionTest && !bHit)
		{
			SH::SpatialHashGrid<WorldEntity>& worldGrid = currentLevel.getWorldGrid();

			/* #optimize #alternative collision. perhaps simple ray casts would be faster; it will require that triangles for collision shapes are available
				This method isn't going to be ideal for large structures, because the distance to the center of the shape collided with isn't going
				to produce a distance to generate an accurate end point; a ray trace will however. Plus it will probably be more efficient than
				a cube x cube collision test.
			*/
			std::vector<std::shared_ptr<const SH::HashCell<WorldEntity>>> cells;
			worldGrid.lookupCellsForLine(start, end, cells);

			//#optimize below may be slow; but don't want a bunch of collisions with same objects that occupy large # of cells (eg large ship)
			static std::set<WorldEntity*> potentialCollisions;
			potentialCollisions.clear(); //absolute must do this since it is a static local

			for (sp<const SH::HashCell<WorldEntity>> cell : cells)
			{
				for (std::shared_ptr<SH::GridNode<WorldEntity>> gridNode : cell->nodeBucket)
				{
					potentialCollisions.insert(&gridNode->element);
				}
			}

			static sp<SAT::Shape> projectileShape = new_sp<SAT::CubeShape>();
			projectileShape->updateTransform(collisionXform);

			float smallestDistanceCollision_2 = std::numeric_limits<float>::infinity();
			WorldEntity* collidingEntity = nullptr;

			for (WorldEntity* entity : potentialCollisions)
			{
				if (entity->hasCollisionInfo())
				{
					const sp<const ModelCollisionInfo>& colisionData = entity->getCollisionInfo();
					const sp<const SAT::Shape>& OBBShape = colisionData->getOBBShape();

					glm::vec4 obbMTV;
					if (SAT::Shape::CollisionTest(*projectileShape, *OBBShape, obbMTV))
					{
						//TODO perhaps this shouldn't find the closest shape it collided with? will be redundant
						for (const ModelCollisionInfo::ConstShapeData& shapeData : colisionData->getConstShapeData())
						{
							glm::vec4 mtv;
							if (SAT::Shape::CollisionTest(*projectileShape, *shapeData.shape, mtv))
							{
								vec3 shapeOrigin = vec3(shapeData.shape->getTransformedOrigin());

								//it would be better to use distance the contact point, but that isn't available in my SAT implementation
								//perhaps I will change that if I figure out an efficient means to get the contact point
								float distToShapOri = glm::length2(shapeOrigin - start); //get distance from start to shape ori
								if (distToShapOri < smallestDistanceCollision_2)
								{
									smallestDistanceCollision_2 = distToShapOri;
									collidingEntity = entity;
								}
							}
						}
					}
				}
			}

			if (collidingEntity)
			{
				float hitDistance = glm::sqrt(smallestDistanceCollision_2);
				glm::vec3 hit = start + offsetDir * hitDistance;

				if (IProjectileHitNotifiable* toNotify = dynamic_cast<IProjectileHitNotifiable*>(collidingEntity))
				{
					toNotify->notifyProjectileCollision(*this, hit);
				}

				//recalculate end point etc so visuals don't go through
				stretchToDistance(hitDistance, false, currentLevel);

				//make sure this projectile will expire on next tick!
				hitLocation = hit;
				ticksSinceHit = 0;
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Projectile system
	///////////////////////////////////////////////////////////////////////////////////////////////

	void ProjectileSystem::postGameLoopTick(float dt_sec)
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

			if (projectile->timeAlive > projectile->lifetimeSec || projectile->ticksSinceHit > 3)
			{
				
				//note: this projectile will keep any sp alive, so clear before release if needed
				objPool.releaseInstance(projectile);

				//removing iterator from set does not invalidate other iterators; 
				//IMPORANT: this must after releasing to pool, otherwise the smart pointer will be deleted
				activeProjectiles.erase(iterCopy);
			}
		}
	}

	void ProjectileSystem::initSystem()
	{
		//align projectiles with camera
		GameBase::get().PostGameloopTick.addStrongObj(sp_this(), &ProjectileSystem::postGameLoopTick);
	}

	sp<ProjectileClassHandle> ProjectileSystem::createProjectileType(const sp<Model3D>& model, const Transform& AABB_unitCubeTransform)
	{
		//this method met not be necessary after all; clients should be free to just create projectile handles when ever
		sp<ProjectileClassHandle> projectileType = new_sp<ProjectileClassHandle>(AABB_unitCubeTransform, model);

		return projectileType;
	}

	void ProjectileSystem::spawnProjectile(const glm::vec3& start, const glm::vec3& direction_n, const ProjectileClassHandle& projectileTypeHandle)
	{
		sp<Projectile> spawned = objPool.getInstance();

		//#optimize note there may some optimized functions in glm to do this work
		glm::vec3 projectileSystemForward(0, 0, -1);
		glm::quat spawnRotation = Utils::getRotationBetween(projectileSystemForward, direction_n);

		//todo define an argument struct to pass for spawning projectiles
		spawned->xform.position = start;
		spawned->xform.rotQuat = spawnRotation;

		spawned->bHit = false;
		spawned->ticksSinceHit = 0;

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

	void ProjectileSystem::unspawnAllProjectiles()
	{
		activeProjectiles.clear();
	}

	void ProjectileSystem::renderProjectiles(Shader& projectileShader) const
	{
		//#optimize potential optimization is to use instanced rendering to reduce draw call number
		//TODO perhaps projectile should be made a full class and encapsulate this logic

		//invariant: shader uniforms pre-configured
		for (const sp<Projectile>& projectile : activeProjectiles)
		{
			projectileShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(projectile->renderXform));
			projectile->model->draw(projectileShader);
		}
	}

	void ProjectileSystem::renderProjectileBoundingBoxes(Shader& debugShader, const glm::vec3& color, const glm::mat4& view, const glm::mat4& perspective) const
	{
		for (const sp<Projectile>& projectile : activeProjectiles)
		{
			Utils::renderDebugWireCube(debugShader, color, projectile->collisionXform, view, perspective);
		}
	}

}

