#include "SAShipPlacements.h"
#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SAAssetSystem.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "AssetConfigs/SAConfigBase.h"
#include "../GameFramework/SACollisionUtils.h"
#include "SpaceArcade.h"
#include "../GameFramework/SALevel.h"
#include "../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../GameFramework/Components/CollisionComponent.h"
#include "OptionalCompilationMacros.h"
#include "../Rendering/RenderData.h"
#include "../GameFramework/SARenderSystem.h"

namespace SA
{
	static const std::string comStr = "COMMUNICATIONS";
	static const std::string turretStr = "TURRET";
	static const std::string defStr = "DEFENSE";
	static const std::string invalidStr = "INVALID";

	std::string lexToString(PlacementType enumValue)
	{
		switch (enumValue)
		{
			case PlacementType::COMMUNICATIONS: return comStr;
			case PlacementType::DEFENSE: return defStr;
			case PlacementType::TURRET: return turretStr;
			default: return invalidStr;
		}
	}

	SA::PlacementType stringToLex(const std::string& strValue)
	{
		if (strValue == comStr)
		{
			return PlacementType::COMMUNICATIONS;
		}
		else if (strValue == turretStr)
		{
			return PlacementType::TURRET;
		}
		else if (strValue == defStr)
		{
			return PlacementType::DEFENSE;
		}

		return PlacementType::INVALID;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ship placement entity
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void ShipPlacementEntity::draw(Shader& shader)
	{
		if (getModel())
		{
			shader.setUniformMatrix4fv(modelMatrixUniform.c_str(), 1, GL_FALSE, glm::value_ptr(cachedModelMat_PxL));
			RenderModelEntity::draw(shader);
		}
#if SA_RENDER_DEBUG_INFO
		if (collisionData)
		{
			GameBase& engine = GameBase::get();
			static RenderSystem& renderSystem = engine.getRenderSystem();

			if (const RenderData* frd = renderSystem.getFrameRenderData_Read(engine.getFrameNumber()))
			{
				collisionData->debugRender(cachedModelMat_PxL, frd->view, frd->projection);
			}
		}
#endif //SA_RENDER_DEBUG_INFO
	}

	void ShipPlacementEntity::setParentXform(glm::mat4 parentXform)
	{
		this->parentXform = parentXform;

		updateModelMatrixCache();

		//TODO_update_collision;
	}

	void ShipPlacementEntity::setTransform(const Transform& inTransform)
	{
		RenderModelEntity::setTransform(inTransform);

		updateModelMatrixCache();
	}

	void ShipPlacementEntity::postConstruct()
	{

	}

	void ShipPlacementEntity::notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc)
	{

	}

	void ShipPlacementEntity::updateModelMatrixCache()
	{
		cachedModelMat_PxL = parentXform * getModelMatrix();

		if (collisionData)
		{
			collisionData->updateToNewWorldTransform(cachedModelMat_PxL);

			LevelBase* world = getWorld();
			if (world && collisionHandle)
			{
				SH::SpatialHashGrid<WorldEntity>& worldGrid = world->getWorldGrid();
				worldGrid.updateEntry(collisionHandle, collisionData->getWorldOBB());
			}
		}
	}

	void ShipPlacementEntity::replacePlacementConfig(const PlacementSubConfig& newConfig, const ConfigBase& owningConfig)
	{
		bool bModelsAreDifferent = config.relativeFilePath != newConfig.relativeFilePath;
		bool bCollisionModelsDifferent = config.relativeCollisionModelFilePath != newConfig.relativeCollisionModelFilePath;

		config = newConfig;

		////////////////////////////////////////////////////////
		// static local positioning
		////////////////////////////////////////////////////////
		Transform localTransform;
		localTransform.position = config.position;
		localTransform.rotQuat = getRotQuatFromDegrees(config.rotation_deg);
		localTransform.scale = config.scale;
		setTransform(localTransform);

		updateModelMatrixCache();

		////////////////////////////////////////////////////////
		// visual model3d
		////////////////////////////////////////////////////////
		if (bModelsAreDifferent)
		{
			sp<Model3D> model = GameBase::get().getAssetSystem().loadModel(config.getFullPath(owningConfig));
			replaceModel(model);
		}

		////////////////////////////////////////////////////////
		// collision
		////////////////////////////////////////////////////////
		if (const sp<const Model3D>& placementModel3D = getModel())
		{
			CollisionShapeFactory& shapeFactory = SpaceArcade::get().getCollisionShapeFactoryRef();

			collisionData = new_sp<CollisionData>();
			collisionData->setAABBtoModelBounds(*placementModel3D);

			bool bUseModelAABBCollision = config.relativeCollisionModelFilePath.length() == 0;
			if (!bUseModelAABBCollision)
			{
				//use custom collision shape
				std::string fullModelCollisionPath = config.getCollisionModelFullPath(owningConfig);

				if (sp<Model3D> model = GameBase::get().getAssetSystem().loadModel(fullModelCollisionPath))
				{
					CollisionData::ShapeData newShapeInfo;
					newShapeInfo.localXform = glm::mat4(1.f);
					newShapeInfo.shape = shapeFactory.generateShape(ECollisionShape::MODEL, fullModelCollisionPath);
					newShapeInfo.shapeType = ECollisionShape::MODEL;
					collisionData->addNewCollisionShape(newShapeInfo);
				}
				else
				{
					//fallback generate using AABB
					bUseModelAABBCollision = true;
				}
			}

			//model loading may fail, so as a fallback use the AABB; mean this should not be an else statement on the model loading branch above
			if (bUseModelAABBCollision)
			{
				//use model bounding box of the entire model since no specific shape is provided
				CollisionData::ShapeData newShapeInfo;
				newShapeInfo.localXform = collisionData->getAABBLocalXform();
				newShapeInfo.shape = shapeFactory.generateShape(ECollisionShape::CUBE);
				newShapeInfo.shapeType = ECollisionShape::CUBE;
				collisionData->addNewCollisionShape(newShapeInfo);
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "loading placement config without a model to base AABB collision - no collision data will be generated for this placement");
		}
		collisionHandle = nullptr;

		if (collisionData)
		{
			CollisionComponent* collComp = getGameComponent<CollisionComponent>();
			if (!collComp)
			{
				//deferring creation of collision component until we have valid collision; that way if modder somehow fails to provide collision data things we won't have a collision component unless there's valid collision
				collComp = createGameComponent<CollisionComponent>();
			}
			collComp->setCollisionData(collisionData);
			collComp->setKinematicCollision(false); //don't attempt to collide with owner

			if (LevelBase* world = getWorld()) //WARNING: caching world sp will create cyclic reference
			{
				updateModelMatrixCache(); //force update of collision data in case there are any logic changes
				collisionHandle = world->getWorldGrid().insert(*this, collisionData->getWorldOBB());
			}
			else
			{
				throw std::logic_error("Attempting to configure ship placement but no world available");
			}
		}
	}
}

std::string SA::PlacementSubConfig::getFullPath(const ConfigBase& owningConfig) const
{
	return owningConfig.getOwningModDir() + relativeFilePath;
}

std::string SA::PlacementSubConfig::getCollisionModelFullPath(const ConfigBase& owningConfig) const
{
	if (relativeCollisionModelFilePath.length() > 0)
	{
		return owningConfig.getOwningModDir() + relativeCollisionModelFilePath;
	}
	else
	{
		//just return empty string
		return relativeCollisionModelFilePath;
	}
}
