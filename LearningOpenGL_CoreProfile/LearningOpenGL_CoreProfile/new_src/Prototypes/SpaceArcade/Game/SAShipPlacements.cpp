#include "SAShipPlacements.h"
#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SAAssetSystem.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "AssetConfigs/SAConfigBase.h"

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
			RenderModelEntity::draw(shader);
		}
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
	}

	void ShipPlacementEntity::replacePlacementConfig(const PlacementSubConfig& newConfig, ConfigBase& owningConfig)
	{
		bool bModelsAreDifferent = newConfig.relativeFilePath != newConfig.relativeFilePath;
		config = newConfig;

		if (bModelsAreDifferent)
		{
			sp<Model3D> model = GameBase::get().getAssetSystem().loadModel(config.getFullPath(owningConfig));
			replaceModel(model);
		}

		Transform localTransform;
		localTransform.position = config.position;
		localTransform.rotQuat = getRotQuatFromDegrees(config.rotation_deg);
		localTransform.scale = config.scale;
		setTransform(localTransform);

		updateModelMatrixCache();
	}
}

std::string SA::PlacementSubConfig::getFullPath(ConfigBase& owningConfig) const
{
	return owningConfig.getOwningModDir() + relativeFilePath;
}
