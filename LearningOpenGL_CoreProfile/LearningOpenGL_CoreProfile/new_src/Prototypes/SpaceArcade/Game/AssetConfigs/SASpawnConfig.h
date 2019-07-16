#pragma once
#include <string>
#include <vector>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "../../GameFramework/SAGameEntity.h"
#include "SAConfigBase.h"

namespace SA
{
	class SpawnConfig;
	class Model3D;
	class ModelCollisionInfo;
	class ProjectileConfig;

	struct CollisionShapeConfig
	{
		int shape;		//avoiding strict alias violation, this is int type rather than enum;
		glm::vec3 scale{ 1,1,1 };
		glm::vec3 rotationDegrees{ 0,0,0 };
		glm::vec3 position{ 0,0,0 };
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
		sp<SA::ModelCollisionInfo> toCollisionInfo();
		sp<Model3D> getModel() const;
		sp<ProjectileConfig>& getPrimaryProjectileConfig();

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

		std::vector<CollisionShapeConfig> shapes;
		
		//color/material
		//team
	};


}
