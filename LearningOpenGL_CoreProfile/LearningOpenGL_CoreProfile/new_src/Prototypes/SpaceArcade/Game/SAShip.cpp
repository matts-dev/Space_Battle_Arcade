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
		team(spawnData.team)
	{
		overlappingNodes_SH.reserve(10);
		primaryProjectile = spawnData.spawnConfig->getPrimaryProjectileConfig();
		shieldColor = spawnData.spawnConfig->getShieldColor();
		shieldOffset = spawnData.spawnConfig->getShieldOffset();
	}

	const sp<const ModelCollisionInfo>& Ship::getCollisionInfo() const
	{
		//This will actually make a copy of the sp if we return collision data directly
		//So, optimizing by storing a shared pointer that views owning data as const and returning that by reference
		return constViewCollisionData;
	}

	glm::vec4 Ship::getForwardDir()
	{
		//#optimize cache per frame/move update; transforming vector 
		const Transform& transform = getTransform();

		// #Warning this doesn't include any parent transformations #parentxform; those can also be cached per frame
		//Currently there is no system for parent/child scene nodes for these. But if there were/ever is, it should
		//get the parent transform and use that like : parentXform * rotMatrix * pointingDir; but I believe non-unfirom scaling will cause
		//issues with vectors (like normals) and some care (normalMatrix, ie inverse transform), or (no non-uniform scales) may be needed to make sure vectors are correct
		glm::vec4 pointingDir{ 0, 0, 1, 0 }; //#TODO this should could from spawn config in case models are not aligned properly
		glm::mat4 rotMatrix = glm::toMat4(transform.rotQuat);
		glm::vec4 rotDir = rotMatrix * pointingDir;

		return rotDir;
	}

	glm::vec4 Ship::getUpDir()
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

	void Ship::setPrimaryProjectile(const sp<ProjectileConfig>& projectileConfig)
	{
		primaryProjectile = projectileConfig;
	}

	void Ship::fireProjectile(BrainKey privateKey)
	{
		//#optimize: set a default projectile config so this doesn't have to be checked every time a ship fires? reduce branch divergence
		if (primaryProjectile)
		{
			const sp<ProjectileSystem>& projectileSys = SpaceArcade::get().getProjectileSystem();

			ProjectileSystem::SpawnData spawnData;
			spawnData.direction_n = glm::normalize(velocity);
			
			//#TODO below doesn't account for parent transforms
			spawnData.start = spawnData.direction_n * 5.0f + getTransform().position; //#TODO make this work by not colliding with src ship; for now spawning in front of the ship
			
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
		if (team != hitProjectile.team)
		{
			hp.current -= hitProjectile.damage;
			if (hp.current <= 0)
			{
				ParticleSystem::SpawnParams particleSpawnParams;
				particleSpawnParams.particle = ParticleFactory::getSimpleExplosionEffect();
				particleSpawnParams.xform.position = this->getTransform().position;
				particleSpawnParams.velocity = this->velocity;
				GameBase::get().getParticleSystem().spawnParticle(particleSpawnParams);
				destroy(); //perhaps enter a destroyed state with timer to remove actually destroy -- rather than immediately despawning
			}
			else
			{
				if (!activeShieldEffect.expired())
				{
					sp<ActiveParticleGroup> shieldEffect_sp = activeShieldEffect.lock();
					shieldEffect_sp->ResetTimeAlive();
				}
				else
				{
					ParticleSystem::SpawnParams particleSpawnParams;
					particleSpawnParams.particle = shieldEffects->getEffect(getMyModel(), shieldColor);
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
