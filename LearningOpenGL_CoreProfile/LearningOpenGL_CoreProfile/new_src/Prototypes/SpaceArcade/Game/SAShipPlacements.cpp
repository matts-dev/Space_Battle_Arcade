#include "SAShipPlacements.h"
#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SAAssetSystem.h"
#include "../Tools/ModelLoading/SAModel.h"

namespace SA
{
	std::string lexToString(PlacementType enumValue)
	{
		switch (enumValue)
		{
			case PlacementType::COMMUNICATIONS: return "COMMUNICATIONS";
			case PlacementType::DEFENSE: return "DEFENSE";
			case PlacementType::TURRET: return "TURRET";
			default: return "NA";
		}
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

	void ShipPlacementEntity::replacePlacementConfig(const PlacementSubConfig& newConfig)
	{
		bool bModelsAreDifferent = newConfig.filePath != newConfig.filePath;
		config = newConfig;

		if (bModelsAreDifferent)
		{
			sp<Model3D> model = GameBase::get().getAssetSystem().loadModel(config.filePath.c_str());
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
