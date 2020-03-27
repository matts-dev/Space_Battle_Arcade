#pragma once
#include "AssetConfigs/SAConfigBase.h"
#include "../Tools/DataStructures/SATransform.h"
#include "../GameFramework/SAWorldEntity.h"
#include "../GameFramework/RenderModelEntity.h"
#include "SAProjectileSystem.h"
#include <optional>
#include "../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../Tools/DataStructures/AdvancedPtrs.h"

namespace SA
{
	class ConfigBase;
	class ActiveParticleGroup;

	enum class PlacementType
	{
		COMMUNICATIONS, DEFENSE, TURRET, INVALID
	};
	std::string lexToString(PlacementType enumValue);
	PlacementType stringToLex(const std::string& strValue);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents the data side of the placeable object
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct PlacementSubConfig
	{
		glm::vec3 position{ 0.f };
		glm::vec3 rotation_deg{ 0.f }; //yaw, pitch, roll
		glm::vec3 scale{ 1.f };
		std::string relativeFilePath;
		std::string relativeCollisionModelFilePath;
		PlacementType placementType;
			
		std::string getFullPath(const ConfigBase& owningConfig) const;
		std::string getCollisionModelFullPath(const ConfigBase& owningConfig) const;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents the in world object that is associated with a ship. eg cannon, satellite, etc.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class ShipPlacementEntity : public RenderModelEntity, public IProjectileHitNotifiable
	{
		using Parent = RenderModelEntity;
	public:
		ShipPlacementEntity() : RenderModelEntity(nullptr, Transform{})
		{}
		glm::vec3 ShieldColor() const { return shieldColor; }
		void ShieldColor(glm::vec3 val) { shieldColor = val; }
	protected:
		virtual void postConstruct() override;
		virtual void onDestroyed() override;
	public:
		virtual void draw(Shader& shader) override;
		void setParentXform(glm::mat4 parentXform); //#TODO #scenenodes
		const glm::mat4& getParentXform() const { return parentXform; }
		virtual void setTransform(const Transform& inTransform) override;
		/** returns the model matrix considering the parent's transform*/
		const glm::mat4& getParentXLocalModelMatrix(){ return cachedModelMat_PxL; }
		void replacePlacementConfig(const PlacementSubConfig& newConfig, const ConfigBase& owningConfig);
		void adjustHP(int amount);
	private:
		virtual void notifyProjectileCollision(const Projectile& hitProjectile, glm::vec3 hitLoc) override;
		void doShieldFX();
		void doDestroyFX();
		void updateModelMatrixCache();
	private: //model editor special access
		friend class ModelConfigurerEditor_Level;
	public:
		std::string modelMatrixUniform = "model";
	private:
		up<SH::HashEntry<WorldEntity>> collisionHandle = nullptr;
		sp<CollisionData> collisionData = nullptr;
		fwp<ActiveParticleGroup> activeShieldEffect;
		PlacementSubConfig config;
		glm::mat4 cachedModelMat_PxL{ 1.f };
		glm::mat4 parentXform{ 1.f };
		glm::vec3 shieldColor = glm::vec3(1.f);
	};
}
