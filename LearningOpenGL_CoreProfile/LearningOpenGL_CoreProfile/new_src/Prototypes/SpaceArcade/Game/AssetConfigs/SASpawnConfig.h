#pragma once
#include <string>
#include <vector>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "../../GameFramework/SAGameEntity.h"
#include "SAConfigBase.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../SAShipPlacements.h"

namespace SA
{
	class SpawnConfig;
	class Model3D;
	class CollisionData;
	class ProjectileConfig;

	struct CollisionShapeSubConfig
	{
		int shape;		//avoiding strict alias violation, this is int type rather than enum;
		glm::vec3 scale{ 1,1,1 };
		glm::vec3 rotationDegrees{ 0,0,0 };
		glm::vec3 position{ 0,0,0 };
		std::string modelFilePath;
	};

	struct AvoidanceSphereSubConfig
	{
		float radius = 1.0f;
		glm::vec3 localPosition{ 0.f };
	};

	struct TeamData final 
	{
		glm::vec3 shieldColor = glm::vec3(0.58, 0.51, 0.99);
		glm::vec3 teamTint = glm::vec3(1.0f);
		glm::vec3 projectileColor = glm::vec3(0.8f, 0.8f, 0);
	};

	struct FighterSpawnPoint final
	{
		glm::vec3 location_lp = glm::vec3(0.f);
		glm::vec3 direction_ln = glm::vec3(1.f, 0, 0); //normalized when serialized
	};

	class SpawnConfig final : public ConfigBase
	{
		friend class ModelConfigurerEditor_Level;
		class PrivateKey
		{
			friend class Mod;
			PrivateKey() {};
		};

	public:
		const std::string& getModelFilePath() const { return fullModelFilePath; };

		virtual std::string getRepresentativeFilePath() override;

	public: //utility functions
		sp<SA::CollisionData> toCollisionInfo() const;
		sp<Model3D> getModel() const;
		sp<ProjectileConfig>& getPrimaryProjectileConfig();
		const std::vector<TeamData>& getTeams() const { return teamData; };
		Transform getModelXform() const;
		bool requestCollisionTests() const {return bRequestsCollisionTests;};
		bool getCollisionReflectForward() const { return bCollisionReflectForward; }
		const std::vector<AvoidanceSphereSubConfig>& getAvoidanceSpheres() const { return avoidanceSpheres; }
		const std::vector<PlacementSubConfig>& getCommuncationPlacements() const { return communicationPlacements; }
		const std::vector<PlacementSubConfig>& getDefensePlacements() const { return defensePlacements; }
		const std::vector<PlacementSubConfig>& getTurretPlacements() const { return turretPlacements; }
		
	protected:
		virtual void onSerialize(json& outData) override;
		virtual void onDeserialize(const json& inData) override;
	
	private: //non-serialized properties
		sp<ProjectileConfig> primaryFireProjectile;

	private: //serialized properties
		std::string fullModelFilePath;
		glm::vec3 modelScale = glm::vec3(1,1,1);
		glm::vec3 modelRotationDegrees = glm::vec3(0,0,0);
		glm::vec3 modelPosition = glm::vec3(0, 0, 0);
		std::string primaryProjectileConfigName;

		bool bRequestsCollisionTests = true;
		bool bCollisionReflectForward = true;
		std::vector<CollisionShapeSubConfig> shapes;

		std::vector<AvoidanceSphereSubConfig> avoidanceSpheres;

		std::vector<PlacementSubConfig> communicationPlacements;
		std::vector<PlacementSubConfig> defensePlacements;
		std::vector<PlacementSubConfig> turretPlacements;

		std::vector<FighterSpawnPoint> spawnPoints;
		std::vector<std::string> fighterSpawnConfigs;

		//color/material
		//team
		std::vector<TeamData> teamData;
	};


}
