#include "SASpawnConfig.h"
#include "..\..\..\..\Libraries\nlohmann\json.hpp"
#include "..\GameFramework\SALog.h"
#include "SAModSystem.h"
#include "SpaceArcade.h"
#include <fstream>
#include <sstream>


using json = nlohmann::json;

namespace SA
{
	std::string SpawnConfig::serialize()
	{

		json outData = 
		{
			{"name", name},
			{"fullModelFilePath", fullModelFilePath},
			{"bIsDeleteable", bIsDeleteable},
			{"modelAABB", { {"modelScale", {modelScale.x, modelScale.y, modelScale.z}},
							{ "modelRotationDegrees", {modelRotationDegrees.x, modelRotationDegrees.y, modelRotationDegrees.z}},
							{ "modelPosition" , {modelPosition.x, modelPosition.y, modelPosition.z}}
						}
			},
			{ "shapes", {} }
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

			outData["shapes"].push_back(s);
		}

		return outData.dump(4);
	}

	void SpawnConfig::deserialize(const std::string& str)
	{
		json inData = json::parse(str);
		if (!inData.is_null())
		{
			//for backwards compatibility, make sure to null check all fields
			name = !inData["name"].is_null() ? inData["name"] : "";
			fullModelFilePath = !inData["fullModelFilePath"].is_null() ? inData["fullModelFilePath"] : "";
			bIsDeleteable = !inData["bIsDeleteable"].is_null() ? (bool)inData["bIsDeleteable"] : true;

			////////////////////////////////////////////////////////
			//MODEL AABB
			////////////////////////////////////////////////////////
			json modelAABB = inData["modelAABB"];
			if (!modelAABB.is_null())
			{
				json scale = modelAABB["modelScale"];
				if (!scale.is_null() && scale.is_array()) { modelScale = { scale[0], scale[1], scale[2] }; }

				json rot = modelAABB["modelRotationDegrees"];
				if (!rot.is_null() && rot.is_array()) { modelRotationDegrees = { rot[0], rot[1], rot[2] }; }

				json pos = modelAABB["modelPosition"];
				if (!pos.is_null() && pos.is_array()) { modelPosition = { pos[0], pos[1], pos[2] }; }
			}

			json loadedShapes = inData["shapes"];
			if (!loadedShapes.is_null())
			{
				for (const json& shape : loadedShapes)
				{
					if (!shape.is_null())
					{
						CollisionShapeConfig shapeConfig;
						json scale = shape["scale"];
						if (!scale.is_null() && scale.is_array()) { shapeConfig.scale = { scale[0], scale[1], scale[2] }; }

						json rot = shape["rotationDegrees"];
						if (!rot.is_null() && rot.is_array()) { shapeConfig.rotationDegrees = { rot[0], rot[1], rot[2] }; }

						json pos = shape["position"];
						if (!pos.is_null() && pos.is_array()) { shapeConfig.position = { pos[0], pos[1], pos[2] }; }

						json shapeIdx = shape["shape"];
						if (!shapeIdx.is_null() && shapeIdx.is_number_integer()) { shapeConfig.shape = shape["shape"]; }

						shapes.push_back(shapeConfig);
					}
				}
			}
		}
		else
		{
			log(__FUNCTION__, SA::LogLevel::LOG_WARNING, "Failed to deserialize string");
		}
	}

	std::string SpawnConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("Assets/SpawnConfigs/") + name + std::string(".json");
	}

	void SpawnConfig::save()
	{
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			std::string filepath = getRepresentativeFilePath();
			std::ofstream outFile(filepath);
			if (outFile.is_open())
			{
				outFile << serialize();
			}
		}
	}

	/*static*/ sp<SA::SpawnConfig> SpawnConfig::load(std::string filePath)
	{
		//replace windows filepath separators with unix separators
		for (uint32_t charIdx = 0; charIdx < filePath.size(); ++charIdx)
		{
			if (filePath[charIdx] == '\\')
			{
				filePath[charIdx] = '/';
			}
		}

		std::string modPath = "";

		std::string::size_type modCharIdx = filePath.find("mods/");
		if (modCharIdx != std::string::npos && (modCharIdx < filePath.size() - 1))
		{
			std::string::size_type endOfModNameIdx = filePath.find("/", modCharIdx + 5);
			if (endOfModNameIdx != std::string::npos)
			{
				std::string::size_type count = endOfModNameIdx + 1;
				modPath = filePath.substr(0, count);
			}
		}

		//we must have a mod path if we're going to create a spawn config
		//this is not serialized with the spawn config so that configs can
		//be copy pasted across mods
		if (modPath.size() > 0)
		{
			std::ifstream inFile(filePath);
			if (inFile.is_open())
			{
				std::stringstream ss;
				ss << inFile.rdbuf();

				sp<SpawnConfig> newConfig = new_sp<SpawnConfig>();
				newConfig->deserialize(ss.str());
				newConfig->owningModDir = modPath;

				return newConfig;
			}
		}

		return nullptr;
	}

	const char* const shapeToStr(ECollisionShape value)
	{
		switch (value)
		{
			case ECollisionShape::CUBE:
				return "Cube";
			case ECollisionShape::POLYCAPSULE:
				return "PolyCapsule";
			default:
				return "invalid";
		}
	}

}

