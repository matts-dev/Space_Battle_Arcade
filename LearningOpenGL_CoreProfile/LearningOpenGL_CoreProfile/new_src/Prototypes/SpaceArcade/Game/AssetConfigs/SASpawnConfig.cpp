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

	sp<SA::CollisionData> SpawnConfig::toCollisionInfo() const
	{
		//#optimize: cache some of these matrix operations so that they're not calculated every time
		using glm::vec3; using glm::vec4; using glm::mat4;

		sp<CollisionData> collisionInfo = new_sp<CollisionData>();

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
		for (const CollisionShapeSubConfig& shapeConfig : shapes)
		{
			Transform xform;
			xform.position = shapeConfig.position;
			xform.scale = shapeConfig.scale;
			xform.rotQuat = getRotQuatFromDegrees(shapeConfig.rotationDegrees);
			mat4 shapeXform = rootModelMat * xform.getModelMatrix();

			CollisionData::ShapeData shapeData;
			shapeData.shapeType = static_cast<ECollisionShape>(shapeConfig.shape);
			shapeData.shape = shapeFactory.generateShape(static_cast<ECollisionShape>(shapeConfig.shape), shapeConfig.modelFilePath);
			shapeData.localXform = shapeXform;
			collisionInfo->addNewCollisionShape(shapeData);
		}

		////////////////////////////////////////////////////////
		//AABB / OBB
		////////////////////////////////////////////////////////
		if (sp<Model3D> model = getModel())
		{
			collisionInfo->setAABBtoModelBounds(*model, rootModelMat);
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
							{ "modelPosition" , {modelPosition.x, modelPosition.y, modelPosition.z}}
						}
			},
			//#suggested perhaps rework this so primareFireProjectile has a setter to update the string name copy... this wouldn't follow the normal pattern of treating the config like a struct from within the editor
			{"primaryProjectileConfigName", primaryFireProjectile ? primaryFireProjectile->getName() : primaryProjectileConfigName}, 
			{ "shapes", {} },
			{ "teamData", {} },
			{ "spawnPoints", {} },
			{ "spawnableConfigsByName", {} },
			{ "bRequestsCollisionTests", bRequestsCollisionTests}
		};

		for (CollisionShapeSubConfig& shapeCFG : shapes)
		{
			json s =
			{
				{"scale", {shapeCFG.scale.x, shapeCFG.scale.y, shapeCFG.scale.z}},
				{"rotationDegrees", {shapeCFG.rotationDegrees.x, shapeCFG.rotationDegrees.y, shapeCFG.rotationDegrees.z}},
				{"position" , {shapeCFG.position.x, shapeCFG.position.y, shapeCFG.position.z}},
				{"shape", shapeCFG.shape },
				{"modelFilePath", shapeCFG.shape == int(ECollisionShape::MODEL) ? shapeCFG.modelFilePath : std::string("")}
			};

			spawnData["shapes"].push_back(s);
		}

		for(AvoidanceSphereSubConfig& avoidSphere : avoidanceSpheres)
		{
			json avoidSphereJson = 
			{
				{ "localPosition", {avoidSphere.localPosition.x, avoidSphere.localPosition.y, avoidSphere.localPosition.z} },
				{ "radius", avoidSphere.radius }
			};
			spawnData["avoidanceSpheres"].push_back(avoidSphereJson);
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

		for (const FighterSpawnPoint& spawnPoint : spawnPoints)
		{
			glm::vec3 dir_n = glm::normalize(spawnPoint.direction_ln);	//editor may have made this non-normalized so renormalize.
			glm::vec3 loc_lp = spawnPoint.location_lp;

			json fighterSpawnJson = 
			{
				{"direction_ln", {dir_n.x, dir_n.y, dir_n.z}},
				{"location_lp", {loc_lp.x, loc_lp.y, loc_lp.z}}
			};

			spawnData["spawnPoints"].push_back(fighterSpawnJson);	//created this field in json above	
		}
		for (const std::string& spawnableConfigName : spawnableConfigsByName)
		{
			spawnData["spawnableConfigsByName"].push_back(spawnableConfigName);
		}

		auto serializePlacements = [](const std::vector<PlacementSubConfig>& container, json& outJson)
		{
			for (const PlacementSubConfig& placement : container)
			{
				std::string typeString = lexToString(placement.placementType);
				json obj = {
					{"placementType", typeString},
					{"pos", {placement.position.x, placement.position.y, placement.position.z}},
					{"rotDeg", {placement.rotation_deg.x, placement.rotation_deg.y, placement.rotation_deg.z}},
					{"scale", {placement.scale.x, placement.scale.y, placement.scale.z}},
					{"relativeFilePath", placement.relativeFilePath}
				};
				outJson["placements"].push_back(obj);
			}
		};

		serializePlacements(communicationPlacements,spawnData);
		serializePlacements(turretPlacements,spawnData);
		serializePlacements(defensePlacements,spawnData);

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

				}

				if (spawnData.contains("primaryProjectileConfigName") && spawnData["primaryProjectileConfigName"].is_string())
				{
					//WARNING: this cannot request the projectile config from this code because it will be a race condition; will the 
					// race projectile configs deserialization vs spawn configs deserialization
					primaryProjectileConfigName = spawnData["primaryProjectileConfigName"];
				}

				if (spawnData.contains("bRequestsCollisionTests") && spawnData["bRequestsCollisionTests"].is_boolean())
				{
					bRequestsCollisionTests = spawnData["bRequestsCollisionTests"];
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// load shapes
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				//#TODO use lambda to load vec3s
				json loadedShapes = spawnData["shapes"];
				if (!loadedShapes.is_null())
				{
					for (const json& shape : loadedShapes)
					{
						if (!shape.is_null())
						{
							CollisionShapeSubConfig shapeConfig;
							const json& scale = shape["scale"];
							if (!scale.is_null() && scale.is_array()) { shapeConfig.scale = { scale[0], scale[1], scale[2] }; }

							const json& rot = shape["rotationDegrees"];
							if (!rot.is_null() && rot.is_array()) { shapeConfig.rotationDegrees = { rot[0], rot[1], rot[2] }; }

							const json& pos = shape["position"];
							if (!pos.is_null() && pos.is_array()) { shapeConfig.position = { pos[0], pos[1], pos[2] }; }

							const json& shapeIdx = shape["shape"];
							if (!shapeIdx.is_null() && shapeIdx.is_number_integer()) { shapeConfig.shape = shape["shape"]; }

							if (shape.contains("modelFilePath"))
							{
								const json& modelFilePath = shape["modelFilePath"];
								if (!modelFilePath.is_null() && modelFilePath.is_string()) { shapeConfig.modelFilePath = shape["modelFilePath"]; }
							}

							shapes.push_back(shapeConfig);
						}
					}
 				}
				
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// load avoidance spheres
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (spawnData.contains("avoidanceSpheres"))
				{
					avoidanceSpheres.clear();
					json avSpheres = spawnData["avoidanceSpheres"];
					for (json av : avSpheres)
					{
						AvoidanceSphereSubConfig loadedSphere;
						if (av.contains("radius") && av["radius"].is_number_float())
						{
							loadedSphere.radius = av["radius"];
						}
						if (av.contains("localPosition"))
						{
							json posJson = av["localPosition"];
							if (posJson.is_array())
							{
								loadedSphere.localPosition.x = posJson[0];
								loadedSphere.localPosition.y = posJson[1];
								loadedSphere.localPosition.z = posJson[2];
							}
						}
						avoidanceSpheres.push_back(loadedSphere);
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

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// load team data
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// load spawn points
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (spawnData.contains("spawnPoints"))
				{
					spawnPoints.clear();
					const json& spawnPointsJsonArray = spawnData["spawnPoints"];
					if (spawnPointsJsonArray.is_array() && !spawnPointsJsonArray.is_null())
					{
						for (const json& spJson : spawnPointsJsonArray)
						{
							if (!spJson.is_null()
								&& spJson.contains("direction_ln")
								&& spJson.contains("location_lp")
								)
							{
								spawnPoints.push_back(FighterSpawnPoint{});
								FighterSpawnPoint& loadingPoint = spawnPoints.back();

								loadVec3(loadingPoint.direction_ln, spJson, "direction_ln");
								loadVec3(loadingPoint.location_lp, spJson, "location_lp");
							}
						}
					}
				}

				if (spawnData.contains("spawnableConfigsByName"))
				{
					const json& nameJsonArray = spawnData["spawnableConfigsByName"];
					if (!nameJsonArray.is_null() && nameJsonArray.is_array())
					{
						spawnableConfigsByName.clear();
						for (const std::string& name : nameJsonArray)
						{
							spawnableConfigsByName.push_back(name);
						}
					}
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// load placements
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (spawnData.contains("placements"))
				{
					const json& placements = spawnData["placements"];
					if (placements.is_array() && !placements.is_null())
					{
						for (const json& pjson : placements)
						{
							if (!pjson.is_null() && pjson.contains("placementType") && pjson.contains("pos") && pjson.contains("rotDeg") && pjson.contains("scale") && pjson.contains("relativeFilePath"))
							{
								PlacementSubConfig loadedConfig;
								loadedConfig.placementType = stringToLex(pjson["placementType"]);
								loadVec3(loadedConfig.position, pjson, "pos");
								loadVec3(loadedConfig.rotation_deg, pjson, "rotDeg");
								loadVec3(loadedConfig.scale, pjson, "scale");
								loadedConfig.relativeFilePath = pjson["relativeFilePath"];
								if (loadedConfig.placementType == PlacementType::TURRET)
								{
									turretPlacements.push_back(loadedConfig);
								}
								else if (loadedConfig.placementType == PlacementType::DEFENSE)
								{
									defensePlacements.push_back(loadedConfig);
								}
								else if (loadedConfig.placementType == PlacementType::COMMUNICATIONS)
								{
									communicationPlacements.push_back(loadedConfig);
								}
								else
								{
									log(__FUNCTION__, SA::LogLevel::LOG_WARNING, "Got an invalid placement type when deserializing");
								}
							}
						}
					}
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

	Transform SpawnConfig::getModelXform() const
	{
		Transform xform;
		xform.position = modelPosition;
		xform.scale = modelScale;
		xform.rotQuat = getRotQuatFromDegrees(modelRotationDegrees);
		return xform;
	}

}

