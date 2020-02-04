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

namespace SA
{
	class SpawnConfig;
	class Model3D;
	class CollisionData;
	class ProjectileConfig;

	struct CollisionShapeConfig
	{
		int shape;		//avoiding strict alias violation, this is int type rather than enum;
		glm::vec3 scale{ 1,1,1 };
		glm::vec3 rotationDegrees{ 0,0,0 };
		glm::vec3 position{ 0,0,0 };
		std::string modelFilePath;
	};

	struct AvoidanceSphereConfig
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
		inline glm::vec3 getShieldOffset() { return shieldOffset; }
		const std::vector<TeamData>& getTeams() const { return teamData; };
		Transform getModelXform() const;
		bool requestCollisionTests() const {return bRequestsCollisionTests;};
		bool getCollisionReflectForward() const { return bCollisionReflectForward; }
		const std::vector<AvoidanceSphereConfig>& getAvoidanceSpheres() const { return avoidanceSpheres; }
		
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
		glm::vec3 shieldOffset = glm::vec3(0.f);
		std::string primaryProjectileConfigName;

		std::vector<CollisionShapeConfig> shapes;
		bool bRequestsCollisionTests = true;
		bool bCollisionReflectForward = true;

		std::vector<AvoidanceSphereConfig> avoidanceSpheres;
		
		//color/material
		//team
		std::vector<TeamData> teamData;
	};


}
