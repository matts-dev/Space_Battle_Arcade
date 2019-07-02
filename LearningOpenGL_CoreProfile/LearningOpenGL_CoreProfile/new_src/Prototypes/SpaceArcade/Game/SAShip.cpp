#include "SAShip.h"
#include "SpaceArcade.h"
#include "../GameFramework/SALevel.h"
#include "SASpawnConfig.h"
#include "../GameFramework/SAGameEntity.h"
#include "SAProjectileSystem.h"


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
	}

	Ship::Ship(const sp<SpawnConfig>& inSpawnConfig, const Transform& spawnTransform)
		: RenderModelEntity(inSpawnConfig->getModel(), spawnTransform),
		collisionData(inSpawnConfig->toCollisionInfo()), 
		constViewCollisionData(collisionData)
	{
		overlappingNodes_SH.reserve(10);

		glm::vec4 pointingDir{ 0,0,1,0 };
		glm::mat4 rotMatrix = glm::toMat4(spawnTransform.rotQuat);

		glm::vec4 rotDir = rotMatrix * pointingDir;

		const float speed = 1.0f;
		velocity = rotDir * speed;

	}

	const sp<const ModelCollisionInfo>& Ship::getCollisionInfo() const
	{
		//This will actually make a copy of the sp if we return collision data directly
		//So, optimizing by storing a shared pointer that views owning data as const and returning that by reference
		return constViewCollisionData;
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
				//TODO make sure this object's shape has been updated to transform! this should be done before the loop
				//TODO for each node, get their shape and do a collision tests
				//TODO if collision, make sure this object's SAT::Shapes are updated
			}
#ifdef SA_CAPTURE_SPATIAL_HASH_CELLS
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
