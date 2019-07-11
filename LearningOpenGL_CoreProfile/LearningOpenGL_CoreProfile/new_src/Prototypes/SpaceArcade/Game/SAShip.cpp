#include "SAShip.h"
#include "SpaceArcade.h"
#include "../GameFramework/SALevel.h"
#include "AssetConfigs/SASpawnConfig.h"
#include "../GameFramework/SAGameEntity.h"
#include "SAProjectileSystem.h"
#include "../GameFramework/SAAIBrainBase.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../GameFramework/SAAssetSystem.h"
#include "../GameFramework/SAShipAIBrain.h"

namespace //translation unit statics
{
	using namespace SA;
	//temporary below, better to be stored in an official location and cached on spawn, that way
	//we don't have to check if this is created at spawn every time in every ctor
	sp<ProjectileClassHandle> laserBoltHandle = nullptr;


	void staticInitCheck() //#TODO remove this function if completely refactored out
	{
		SpaceArcade& game = SpaceArcade::get();

		if (!laserBoltHandle)
		{
			AssetSystem& assetSystem = game.getAssetSystem();
			if(sp<Model3D> laserBoltModel = assetSystem.getModel(game.URLs.laserURL))
			{
				//#TODO projectile handle creation code needs refactoring! function may not even be necessary anymore
				laserBoltHandle = game.getProjectileSystem()->createProjectileType(laserBoltModel, Transform{});
			}
		}
	}
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

		staticInitCheck();
	}

	Ship::Ship(const sp<SpawnConfig>& inSpawnConfig, const Transform& spawnTransform)
		: RenderModelEntity(inSpawnConfig->getModel(), spawnTransform),
		collisionData(inSpawnConfig->toCollisionInfo()), 
		constViewCollisionData(collisionData)
	{
		overlappingNodes_SH.reserve(10);

		//glm::vec4 pointingDir{ 0,0,1,0 };
		//glm::mat4 rotMatrix = glm::toMat4(spawnTransform.rotQuat);

		//glm::vec4 rotDir = rotMatrix * pointingDir;

		//const float speed = 1.0f;
		//velocity = rotDir * speed;

		staticInitCheck();
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

	void Ship::fireProjectile(BrainKey privateKey)
	{
		//#optimize: cache any of this at spawn?
		const sp<ProjectileSystem>& projectileSys = SpaceArcade::get().getProjectileSystem();
		glm::vec3 direction = glm::normalize(velocity);
		//#TODO below doesn't account for parent transforms
		glm::vec3 start = direction * 5.0f + getTransform().position; //#TODO make this work by not colliding with src ship; for now spawning in front of the ship
		projectileSys->spawnProjectile(start, direction, *laserBoltHandle); //#TODO needs to get laser bolt handler from somewhere official
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
		
	}

}
