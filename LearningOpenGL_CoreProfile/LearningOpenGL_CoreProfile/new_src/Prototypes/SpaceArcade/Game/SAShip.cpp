#include "SAShip.h"
#include "SpaceArcade.h"
#include "..\GameFramework\SALevel.h"


namespace SA
{
	Ship::Ship(
		const sp<Model3D>& model,
		const Transform& spawnTransform,
		const sp<ModelCollisionInfo>& inCollisionData
	)
		: RenderModelEntity(model, spawnTransform),
		collisionData(inCollisionData)
	{
		overlappingNodes_SH.reserve(10);

		glm::vec4 pointingDir{ 0,0,1,0 };
		glm::mat4 rotMatrix = glm::toMat4(spawnTransform.rotQuat);

		glm::vec4 rotDir = rotMatrix * pointingDir;

		const float speed = 1.0f;
		velocity = rotDir * speed;
	}

	void Ship::postConstruct()
	{
		//WARNING: caching world sp will create cyclic reference
		if (LevelBase* world = getWorld())
		{
			Transform xform = getTransform();
			glm::mat4 xform_m = xform.getModelMatrix();
			collisionHandle = world->getWorldGrid().insert(*this, getWorldOBB(xform_m));
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

		//TODO this will need to respect local shape offsets when that is in place
		for (sp<SAT::Shape> shape : collisionData->configuredCollisionShapes)
		{
			shape->updateTransform(movedXform_m);
		}

		//update the spatial hash
		if (LevelBase* world = getWorld())
		{
			SH::SpatialHashGrid<WorldEntity>& worldGrid = world->getWorldGrid();
			worldGrid.updateEntry(collisionHandle, getWorldOBB(xform.getModelMatrix()));

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

	const std::array<glm::vec4, 8> Ship::getWorldOBB(const glm::mat4 xform) const
	{
		const std::array<glm::vec4, 8> OBB =
		{
			xform * collisionData->localAABB[0],
			xform * collisionData->localAABB[1],
			xform * collisionData->localAABB[2],
			xform * collisionData->localAABB[3],
			xform * collisionData->localAABB[4],
			xform * collisionData->localAABB[5],
			xform * collisionData->localAABB[6],
			xform * collisionData->localAABB[7],
		};
		return OBB;
	}

}
