#include "SAShip.h"
#include "SpaceArcade.h"
#include "../GameFramework/SALevel.h"
#include "AssetConfigs/SASpawnConfig.h"
#include "../GameFramework/SAGameEntity.h"
#include "SAProjectileSystem.h"
#include "../GameFramework/SAAIBrainBase.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../GameFramework/SAAssetSystem.h"
#include "../GameFramework/SAParticleSystem.h"
#include "../GameFramework/EngineParticles/BuiltInParticles.h"
#include "AI/SAShipAIBrain.h"
#include "../GameFramework/Components/GameplayComponents.h"
#include "../Tools/SAUtilities.h"

namespace
{
	using namespace SA;

	sp<ShieldEffect::ParticleCache> shieldEffects = new_sp<ShieldEffect::ParticleCache>();
}


namespace SA
{
	Ship::Ship(
		const sp<Model3D>& model,
		const Transform& spawnTransform,
		const sp<ModelCollisionInfo>& inCollisionData
	)
		: RenderModelEntity(model, spawnTransform),
		collisionData(inCollisionData),
		constViewCollisionData(collisionData)
	{
		overlappingNodes_SH.reserve(10);

		glm::vec4 pointingDir{ 0,0,1,0 };
		glm::mat4 rotMatrix = glm::toMat4(spawnTransform.rotQuat);

		glm::vec4 rotDir = rotMatrix * pointingDir;

		const float speed = 1.0f;
		velocity = rotDir * speed;

		//staticInitCheck();
	}

	Ship::Ship(const SpawnData& spawnData)
		: RenderModelEntity(spawnData.spawnConfig->getModel(), spawnData.spawnTransform),
		collisionData(spawnData.spawnConfig->toCollisionInfo()), 
		constViewCollisionData(collisionData),
		cachedTeamIdx(spawnData.team)
	{
		overlappingNodes_SH.reserve(10);
		primaryProjectile = spawnData.spawnConfig->getPrimaryProjectileConfig();
		shieldOffset = spawnData.spawnConfig->getShieldOffset();
		shipData = spawnData.spawnConfig;

		createGameComponent<TeamComponent>();
		if (TeamComponent* teamComp = getGameComponent<TeamComponent>())
		{
			//bound to delegate in post construct
			teamComp->setTeam(spawnData.team);
			updateTeamDataCache();
		}
	}

	const sp<const ModelCollisionInfo>& Ship::getCollisionInfo() const
	{
		//This will actually make a copy of the sp if we return collision data directly
		//So, optimizing by storing a shared pointer that views owning data as const and returning that by reference
		return constViewCollisionData;
	}

	glm::vec4 Ship::getForwardDir() const
	{
		//#optimize #todo cache per frame/move update; transforming vector (perhaps gameBase can track a framenumber with an inline call)
		const Transform& transform = getTransform();

		// #Warning this doesn't include any parent transformations #parentxform; those can also be cached per frame
		//Currently there is no system for parent/child scene nodes for these. But if there were/ever is, it should
		//get the parent transform and use that like : parentXform * rotMatrix * pointingDir; but I believe non-unfirom scaling will cause
		//issues with vectors (like normals) and some care (normalMatrix, ie inverse transform), or (no non-uniform scales) may be needed to make sure vectors are correct
		glm::vec4 pointingDir = localForwardDir_n();//#TODO this should could from spawn config in case models are not aligned properly
		glm::mat4 rotMatrix = glm::toMat4(transform.rotQuat);
		glm::vec4 rotDir = rotMatrix * pointingDir;

		return rotDir;
	}

	glm::vec4 Ship::getUpDir() const
	{
		//#optimize follow optimizations done in getForwardDir if any are made
		const Transform& transform = getTransform();
		glm::vec4 upDir{ 0, 1, 0, 0 }; //#TODO this should could from spawn config in case models are not aligned properly
		glm::mat4 rotMatrix = glm::toMat4(transform.rotQuat);
		glm::vec4 rotDir = rotMatrix * upDir;
		return rotDir;
	}

	glm::vec4 Ship::rotateLocalVec(const glm::vec4& localVec)
	{
		const Transform& transform = getTransform();
		glm::mat4 rotMatrix = glm::toMat4(transform.rotQuat);
		glm::vec4 rotDir = rotMatrix * localVec;
		return rotDir;
	}

	float Ship::getMaxTurnAngle_PerSec() const
	{
		//this perhaps should be related to speed, but for now just returning a reasonable 
		return glm::pi<float>();
	}

	void Ship::setPrimaryProjectile(const sp<ProjectileConfig>& projectileConfig)
	{
		primaryProjectile = projectileConfig;
	}

	void Ship::updateTeamDataCache()
{
		if (TeamComponent* teamComp = getGameComponent<TeamComponent>())
		{
			cachedTeamIdx = teamComp->getTeam();
			const std::vector<TeamData>& teams = shipData->getTeams();

			if (cachedTeamIdx >= teams.size()) { cachedTeamIdx = 0;}
		
			assert(teams.size() > 0);
			cachedTeamData = teams[cachedTeamIdx];
		}

	}

	void Ship::handleTeamChanged(size_t oldTeamId, size_t newTeamId)
	{
		updateTeamDataCache();
	}

	void Ship::draw(Shader& shader)
	{
		shader.setUniform3f("objectTint", cachedTeamData.teamTint);
		RenderModelEntity::draw(shader);
	}

	void Ship::fireProjectile(BrainKey privateKey)
	{
		//#optimize: set a default projectile config so this doesn't have to be checked every time a ship fires? reduce branch divergence
		if (primaryProjectile)
		{
			const sp<ProjectileSystem>& projectileSys = SpaceArcade::get().getProjectileSystem();

			ProjectileSystem::SpawnData spawnData;
			//#TODO #scenenodes doesn't account for parent transforms
			spawnData.direction_n = glm::normalize(velocity);
			spawnData.start = spawnData.direction_n * 5.0f + getTransform().position; //#TODO make this work by not colliding with src ship; for now spawning in front of the ship
			spawnData.color = cachedTeamData.projectileColor;
			spawnData.team = cachedTeamIdx;
			spawnData.owner = this;

			projectileSys->spawnProjectile(spawnData, *primaryProjectile); 
		}
	}

	void Ship::fireProjectileInDirection(glm::vec3 dir_n) const
	{
		if (primaryProjectile)
		{
			const sp<ProjectileSystem>& projectileSys = SpaceArcade::get().getProjectileSystem();

			ProjectileSystem::SpawnData spawnData;
			//#TODO #scenenodes doesn't account for parent transforms
			spawnData.direction_n = glm::normalize(velocity);
			spawnData.start = spawnData.direction_n * 5.0f + getTransform().position; //#TODO make this work by not colliding with src ship; for now spawning in front of the ship
			spawnData.color = cachedTeamData.projectileColor;
			spawnData.team = cachedTeamIdx;
			spawnData.owner = this;

			projectileSys->spawnProjectile(spawnData, *primaryProjectile);
		}
	}

	void Ship::setNewBrain(const sp<ShipAIBrain> newBrain, bool bStartNow /*= true*/)
	{
		if (brain)
		{
			brain->sleep();
		}

		//update brain to new brain after stopping previous; should not allow two brains to operate on a single ship 
		brain = newBrain;
		if (newBrain && bStartNow)
		{
			newBrain->awaken();
		}
	}

	void Ship::moveTowardsPoint(const glm::vec3& moveLoc, float dt_sec, float speedFactor)
	{
		using namespace glm;

		const Transform& xform = getTransform();
		vec3 targetDir = moveLoc - xform.position;

		vec3 forwardDir_n = glm::normalize(vec3(getForwardDir()));
		vec3 targetDir_n = glm::normalize(targetDir);
		bool bPointingTowardsMoveLoc = glm::dot(forwardDir_n, targetDir_n) >= 0.99f;

		if (!bPointingTowardsMoveLoc)
		{
			vec3 rotationAxis_n = glm::cross(forwardDir_n, targetDir_n);

			float angleBetween_rad = Utils::getRadianAngleBetween(forwardDir_n, targetDir_n);
			float maxTurn_Rad = getMaxTurnAngle_PerSec() * dt_sec;
			float possibleRotThisTick = glm::clamp(maxTurn_Rad / angleBetween_rad, 0.f, 1.f);

			quat completedRot = Utils::getRotationBetween(forwardDir_n, targetDir_n) * xform.rotQuat;
			quat newRot = glm::slerp(xform.rotQuat, completedRot, possibleRotThisTick);

			//roll ship -- we want the ship's right (or left) vector to match this rotation axis to simulate it pivoting while turning
			vec3 rightVec_n = newRot * vec3(1, 0, 0);
			bool bRollMatchesTurnAxis = glm::dot(rightVec_n, rotationAxis_n) >= 0.99f;

			vec3 newForwardVec_n = newRot * vec3(localForwardDir_n());
			if (!bRollMatchesTurnAxis)
			{
				float rollAngle_rad = Utils::getRadianAngleBetween(rightVec_n, rotationAxis_n);
				float rollThisTick = glm::clamp(maxTurn_Rad / rollAngle_rad, 0.f, 1.f);
				glm::quat roll = glm::angleAxis(rollThisTick, newForwardVec_n);
				newRot = roll * newRot;
			}

			Transform newXform = xform;
			newXform.rotQuat = newRot;
			setTransform(newXform);

			setVelocity(newForwardVec_n * getMaxSpeed() * speedFactor);
		}
		else
		{
			setVelocity(forwardDir_n * getMaxSpeed() * speedFactor);
		}
	}

	void Ship::postConstruct()
	{
		//WARNING: caching world sp will create cyclic reference
		if (LevelBase* world = getWorld())
		{
			Transform xform = getTransform();
			glm::mat4 xform_m = xform.getModelMatrix();
			//collisionHandle = world->getWorldGrid().insert(*this, getWorldOBB(xform_m));
			collisionHandle = world->getWorldGrid().insert(*this, collisionData->getWorldOBB()); 
		}
		else
		{
			throw std::logic_error("World entity being created but there is no containing world");
		}

		if(TeamComponent* team = getGameComponent<TeamComponent>())
		{
			team->onTeamChanged.addWeakObj(sp_this(), &Ship::handleTeamChanged);
		}
	}
	
	void Ship::tick(float deltaTimeSecs)
	{
		Transform xform = getTransform();
		xform.position += velocity * deltaTimeSecs;
		glm::mat4 movedXform_m = xform.getModelMatrix();

		//update collision data
		collisionData->updateToNewWorldTransform(movedXform_m);

		//update the spatial hash
		if (LevelBase* world = getWorld())
		{
			SH::SpatialHashGrid<WorldEntity>& worldGrid = world->getWorldGrid();
			//worldGrid.updateEntry(collisionHandle, getWorldOBB(xform.getModelMatrix()));
			worldGrid.updateEntry(collisionHandle, collisionData->getWorldOBB());

			//test if collisions occurred
			overlappingNodes_SH.clear();
			worldGrid.lookupNodesInCells(*collisionHandle, overlappingNodes_SH);
			for (sp <SH::GridNode<WorldEntity>> node : overlappingNodes_SH)
			{
				//#TODO make sure this object's shape has been updated to transform! this should be done before the loop
				//#TODO for each node, get their shape and do a collision tests
				//#TODO if collision, make sure this object's SAT::Shapes are updated
			}
#if SA_CAPTURE_SPATIAL_HASH_CELLS
			SpatialHashCellDebugVisualizer::appendCells(worldGrid, *collisionHandle);
#endif //SA_CAPTURE_SPATIAL_HASH_CELLS
		}

		setTransform(xform);

		if (!activeShieldEffect.expired())
		{
			//locking wp may be slow as it requires atomic reference count increment; may need to use soft-ptrs if I make that system
			sp<ActiveParticleGroup> activeShield_sp = activeShieldEffect.lock();
			activeShield_sp->xform.rotQuat = xform.rotQuat;
			activeShield_sp->xform.position = xform.position;
			//offset for non-centered scaling issues
			activeShield_sp->xform.position += glm::vec3(rotateLocalVec(glm::vec4(shieldOffset, 0.f))); //#optimize rotating dir is expensive; perhaps cache with dirty flag?
		}
	} 

	//const std::array<glm::vec4, 8> Ship::getWorldOBB(const glm::mat4 xform) const
	//{
	//	const std::array<glm::vec4, 8>& localAABB = collisionData->getLocalAABB();
	//	const std::array<glm::vec4, 8> OBB =
	//	{
	//		xform * localAABB[0],
	//		xform * localAABB[1],
	//		xform * localAABB[2],
	//		xform * localAABB[3],
	//		xform * localAABB[4],
	//		xform * localAABB[5],
	//		xform * localAABB[6],
	//		xform * localAABB[7],
	//	};
	//	return OBB;
	//}

	void Ship::notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc)
	{
		if (cachedTeamIdx != hitProjectile.team)
		{
			//#BUG something is keeping ships alive after they are destroyed (eg holding on to a shared ptr)
			//changing references to #releasing_ptr is the best fix, but for now only spawning particle if not destroyed
			hp.current -= hitProjectile.damage;
			if (hp.current <= 0)
			{
				if(!isPendingDestroy())
				{
					ParticleSystem::SpawnParams particleSpawnParams;
					particleSpawnParams.particle = ParticleFactory::getSimpleExplosionEffect();
					particleSpawnParams.xform.position = this->getTransform().position;
					particleSpawnParams.velocity = this->velocity;
					GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
					if (brain)
					{
						brain->sleep();
						brain = nullptr;
					}
				}
				destroy(); //perhaps enter a destroyed state with timer to remove actually destroy -- rather than immediately despawning
			}
			else
			{
				if (!activeShieldEffect.expired())
				{
					sp<ActiveParticleGroup> shieldEffect_sp = activeShieldEffect.lock();
					shieldEffect_sp->resetTimeAlive();
				}
				else
				{
					if (!activeShieldEffect.expired())
					{
						sp<ActiveParticleGroup> shieldEffect_sp = activeShieldEffect.lock();
						//#TODO kill the shield effect
					}

					ParticleSystem::SpawnParams particleSpawnParams;
					particleSpawnParams.particle = shieldEffects->getEffect(getMyModel(), cachedTeamData.shieldColor);
					const Transform& shipXform = this->getTransform(); 
					particleSpawnParams.xform.position = shipXform.position;
					particleSpawnParams.xform.position += glm::vec3(rotateLocalVec(glm::vec4(shieldOffset, 0.f)));
					particleSpawnParams.xform.rotQuat = shipXform.rotQuat;
					particleSpawnParams.xform.scale = shipXform.scale * 1.1f;  //scale up to see effect around ship

					activeShieldEffect = GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
				}
			}
		}
	}

}
