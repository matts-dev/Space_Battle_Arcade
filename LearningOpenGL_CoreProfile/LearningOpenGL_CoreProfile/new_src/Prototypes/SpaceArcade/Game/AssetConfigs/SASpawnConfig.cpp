#include <detail/type_mat.hpp>
#include <sstream>
#include <fstream>

#include "SASpawnConfig.h"
#include "SAProjectileConfig.h"
#include "../SAModSystem.h"
#include "../SpaceArcade.h"
#include "../../Tools/ModelLoading/SAModel.h"
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SAAssetSystem.h"
#include "../../GameFramework/SACollisionUtils.h"
#include "../../../../../Libraries/nlohmann/json.hpp"


using json = nlohmann::json;

namespace SA
{
	std::string SpawnConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("Assets/SpawnConfigs/") + name + std::string(".json");
	}

	sp<SA::Model3D> SpawnConfig::getModel() const
	{
		static SpaceArcade& game = SpaceArcade::get();
		return game.getAssetSystem().loadModel(fullModelFilePath.c_str());
	}

	sp<SA::ModelCollisionInfo> SpawnConfig::toCollisionInfo()
	{
		//#optimize: cache some of these matrix operations so that they're not calculated every time
		using glm::vec3; using glm::vec4; using glm::mat4;

		sp<ModelCollisionInfo> collisionInfo = new_sp<ModelCollisionInfo>();

		Transform rootXform;
		rootXform.position = modelPosition;
		rootXform.scale = modelScale;
		rootXform.rotQuat = getRotQuatFromDegrees(modelRotationDegrees);
		mat4 rootModelMat = rootXform.getModelMatrix();

		collisionInfo->setRootXform(rootModelMat);

		////////////////////////////////////////////////////////
		//SHAPES
		////////////////////////////////////////////////////////
		CollisionShapeFactory& shapeFactory = SpaceArcade::get().getCollisionShapeFactoryRef();
		for (const CollisionShapeConfig& shapeConfig : shapes)
		{
			Transform xform;
			xform.position = shapeConfig.position;
			xform.scale = shapeConfig.scale;
			xform.rotQuat = getRotQuatFromDegrees(shapeConfig.rotationDegrees);
			mat4 shapeXform = rootModelMat * xform.getModelMatrix();

			ModelCollisionInfo::ShapeData shapeData;
			shapeData.shapeType = static_cast<ECollisionShape>(shapeConfig.shape);
			shapeData.shape = shapeFactory.generateShape(static_cast<ECollisionShape>(shapeConfig.shape));
			shapeData.localXform = shapeXform;
			collisionInfo->addNewCollisionShape(shapeData);
		}

		////////////////////////////////////////////////////////
		//AABB / OBB
		////////////////////////////////////////////////////////
		if (sp<Model3D> model = getModel())
		{
			std::tuple<vec3, vec3> aabbRange = model->getAABB();
			vec3 aabbSize = std::get<1>(aabbRange) - std::get<0>(aabbRange); //max - min

			//correct for model center mis-alignments; this should be cached in game so it isn't calculated each frame
			vec3 aabbCenterPnt = std::get</*min*/0>(aabbRange) + (0.5f * aabbSize);

			//we can now use aabbCenter as a translation vector for the aabb!
			mat4 aabbModel = glm::translate(rootModelMat, aabbCenterPnt);
			aabbModel = glm::scale(aabbModel, aabbSize);
			std::array<glm::vec4, 8>& collisionLocalAABB = collisionInfo->getLocalAABB();
			collisionLocalAABB[0] = aabbModel * SH::AABB[0];
			collisionLocalAABB[1] = aabbModel * SH::AABB[1];
			collisionLocalAABB[2] = aabbModel * SH::AABB[2];
			collisionLocalAABB[3] = aabbModel * SH::AABB[3];
			collisionLocalAABB[4] = aabbModel * SH::AABB[4];
			collisionLocalAABB[5] = aabbModel * SH::AABB[5];
			collisionLocalAABB[6] = aabbModel * SH::AABB[6];
			collisionLocalAABB[7] = aabbModel * SH::AABB[7];

			collisionInfo->setAABBLocalXform(aabbModel);
			collisionInfo->setOBBShape(shapeFactory.generateShape(ECollisionShape::CUBE));
		}
		else
		{
			log("SpawnConfig", LogLevel::LOG_WARNING, "No model available when creating collision info!" __FUNCTION__);
		}

		return collisionInfo;
	}

	void SpawnConfig::onSerialize(json& outData)
	{
		json spawnData =
		{
			{"fullModelFilePath", fullModelFilePath},
			{"modelAABB", { {"modelScale", {modelScale.x, modelScale.y, modelScale.z}},
							{ "modelRotationDegrees", {modelRotationDegrees.x, modelRotationDegrees.y, modelRotationDegrees.z}},
							{ "modelPosition" , {modelPosition.x, modelPosition.y, modelPosition.z}},
							{ "shieldOffset", {shieldOffset.x, shieldOffset.y, shieldOffset.z} }
						}
			},
			//#suggested perhaps rework this so primareFireProjectile has a setter to update the string name copy... this wouldn't follow the normal pattern of treating the config like a struct from within the editor
			{"primaryProjectileConfigName", primaryFireProjectile ? primaryFireProjectile->getName() : primaryProjectileConfigName}, 
			{ "shapes", {} },
			{ "teamData", {} }
		};

		for (CollisionShapeConfig& shapeCFG : shapes)
		{
			json s =
			{
				{"scale", {shapeCFG.scale.x, shapeCFG.scale.y, shapeCFG.scale.z}},
				{"rotationDegrees", {shapeCFG.rotationDegrees.x, shapeCFG.rotationDegrees.y, shapeCFG.rotationDegrees.z}},
				{"position" , {shapeCFG.position.x, shapeCFG.position.y, shapeCFG.position.z}},
				{"shape", shapeCFG.shape }
			};

			spawnData["shapes"].push_back(s);
		}

		for (const TeamData& teamDatum : teamData)
		{
			json t = 
			{
				{ "shieldColor", {teamDatum.shieldColor.x, teamDatum.shieldColor.y, teamDatum.shieldColor.z}},
				{ "teamTint", {teamDatum.teamTint.x, teamDatum.teamTint.y, teamDatum.teamTint.z}},
				{ "projectileColor", {teamDatum.projectileColor.x, teamDatum.projectileColor.y, teamDatum.projectileColor.z} }
			};
			spawnData["teamData"].push_back(t);
		}

		outData.push_back({ "SpawnConfig", spawnData });
	}

	void SpawnConfig::onDeserialize(const json& inData)
	{
		if (!inData.is_null() && inData.contains("SpawnConfig"))
		{
			const json& spawnData = inData["SpawnConfig"];

			if (!spawnData.is_null())
			{
				//for backwards compatibility, make sure to null check all fields
				fullModelFilePath = spawnData.contains("fullModelFilePath") ? spawnData["fullModelFilePath"] : "";

				////////////////////////////////////////////////////////
				//MODEL AABB
				////////////////////////////////////////////////////////
				const json& modelAABB = spawnData["modelAABB"];
				if (!modelAABB.is_null())
				{
					const json& scale = modelAABB["modelScale"];
					if (!scale.is_null() && scale.is_array()) { modelScale = { scale[0], scale[1], scale[2] }; }

					const json& rot = modelAABB["modelRotationDegrees"];
					if (!rot.is_null() && rot.is_array()) { modelRotationDegrees = { rot[0], rot[1], rot[2] }; }

					const json& pos = modelAABB["modelPosition"];
					if (!pos.is_null() && pos.is_array()) { modelPosition = { pos[0], pos[1], pos[2] }; }

					if (modelAABB.contains("shieldOffset"))
					{
						const json& readShieldOffset = modelAABB["shieldOffset"];
						if (!readShieldOffset.is_null() && readShieldOffset.is_array()) { shieldOffset = { readShieldOffset[0], readShieldOffset[1], readShieldOffset[2] }; }
					}

				}

				if (spawnData.contains("primaryProjectileConfigName") && spawnData["primaryProjectileConfigName"].is_string())
				{
					//WARNING: this cannot request the projectile config from this code because it will be a race condition; will the 
					// race projectile configs deserialization vs spawn configs deserialization
					primaryProjectileConfigName = spawnData["primaryProjectileConfigName"];
				}

				//#TODO use lambda to load vec3s
				json loadedShapes = spawnData["shapes"];
				if (!loadedShapes.is_null())
				{
					for (const json& shape : loadedShapes)
					{
						if (!shape.is_null())
						{
							CollisionShapeConfig shapeConfig;
							const json& scale = shape["scale"];
							if (!scale.is_null() && scale.is_array()) { shapeConfig.scale = { scale[0], scale[1], scale[2] }; }

							const json& rot = shape["rotationDegrees"];
							if (!rot.is_null() && rot.is_array()) { shapeConfig.rotationDegrees = { rot[0], rot[1], rot[2] }; }

							const json& pos = shape["position"];
							if (!pos.is_null() && pos.is_array()) { shapeConfig.position = { pos[0], pos[1], pos[2] }; }

							const json& shapeIdx = shape["shape"];
							if (!shapeIdx.is_null() && shapeIdx.is_number_integer()) { shapeConfig.shape = shape["shape"]; }

							shapes.push_back(shapeConfig);
						}
					}
				}

				auto loadVec3 = [](glm::vec3& out, const json& srcJson, const char* const propertyName)
				{
					const json& arrayJson = srcJson[propertyName];
					if (!arrayJson.is_null() && arrayJson.is_array()) 
					{ 
						out.x = arrayJson[0];
						out.y = arrayJson[1];
						out.z = arrayJson[2];
					}
				};

				if (spawnData.contains("teamData"))
				{
					json loadedTeamDataArray = spawnData["teamData"];
					if (!loadedTeamDataArray.is_null() && loadedTeamDataArray.is_array())
					{
						teamData.clear();
						for (const json& teamJson : loadedTeamDataArray)
						{
							if (!teamJson.is_null())
							{
								TeamData deserialized;
								loadVec3(deserialized.shieldColor, teamJson, "shieldColor");
								loadVec3(deserialized.teamTint, teamJson, "teamTint");
								loadVec3(deserialized.projectileColor, teamJson, "projectileColor");
								teamData.push_back(deserialized);
							}
						}
					}
				}
				else 
				{ 
					log(__FUNCTION__, SA::LogLevel::LOG_WARNING, "load spawn config to find team data"); 
					teamData.push_back(TeamData{});
				}
			}
			else
			{
				log(__FUNCTION__, SA::LogLevel::LOG_WARNING, "Failed to deserialize string");
			}
		}
	}

	sp<SA::ProjectileConfig>& SpawnConfig::getPrimaryProjectileConfig()
	{
		//lazy load model since during serialization the projectile config may not have been loaded
		if (!primaryFireProjectile)
		{
			if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
			{
				const std::map<std::string, sp<ProjectileConfig>>& projectileConfigs = activeMod->getProjectileConfigs();
				auto findResult = projectileConfigs.find(primaryProjectileConfigName);
				if (findResult != projectileConfigs.end())
				{
					primaryFireProjectile = findResult->second;
				}
			}
		}

		return primaryFireProjectile;
	}

}

