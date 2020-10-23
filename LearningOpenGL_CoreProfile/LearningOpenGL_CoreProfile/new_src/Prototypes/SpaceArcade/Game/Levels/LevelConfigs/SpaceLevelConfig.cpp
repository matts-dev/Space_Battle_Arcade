#include "SpaceLevelConfig.h"
#include "../../../GameFramework/SALog.h"
#include "../../AssetConfigs/JsonUtils.h"
#include "../../AssetConfigs/SAConfigBase.h"
#include "../../AssetConfigs/SASpawnConfig.h"
#include "../../GameSystems/SAModSystem.h"
#include "../../SpaceArcade.h"
#include "../../Environment/Planet.h"
#include "../../GameModes/ServerGameMode_CarrierTakedown.h"
#include "../../../Tools/SAUtilities.h"

namespace SA
{
	void SpaceLevelConfig::postConstruct()
	{
		ConfigBase::postConstruct();
		fileName = SYMBOL_TO_STR(SpaceLevelConfig);

		bIsDeletable = false; //not wanting to support deleting more files right now, may change mind on this
	}

	void SpaceLevelConfig::applyDemoDataIfEmpty()
	{
		if (const bool bIsEmpty = (gamemodeTag == ""))
		{
			if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
			{
				owningModDir = activeMod->getModDirectoryPath();

				userFacingName = "TEMPLATE_CONFIG";
				gamemodeTag = TAG_GAMEMODE_CARRIER_TAKEDOWN;

				planets.push_back(PlanetData{});
				PlanetData& demoPlanet = planets.back();
				demoPlanet.offsetDir = glm::vec3(1, 0, 0);
				demoPlanet.offsetDistance = 50.f;
				demoPlanet.bHasCivilization = false;
				demoPlanet.texturePath = DefaultPlanetTexturesPaths::albedo1;

				stars.push_back(StarData{});
				StarData& starData = stars.back();
				starData.offsetDir = glm::vec3(-1, 1, 0);
				starData.offsetDistance = 50.f;
				starData.color = glm::vec3(1.f);

				for (size_t teamIdx = 0; teamIdx < 2; ++teamIdx)
				{
					carrierGamemodeData.teams.push_back({});
					SpaceLevelConfig::GameModeData_CarrierTakedown::TeamData& team = carrierGamemodeData.teams.back();
					for (size_t carrierIdx = 0; carrierIdx < 2; ++carrierIdx)
					{
						team.carrierSpawnData.push_back({});
						CarrierSpawnData& carrierData = team.carrierSpawnData.back();
						carrierData.numInitialFighters = 250;
						carrierData.position = std::nullopt;
						/*carrierData.rotation_deg = glm::vec3(45, 2, 1);*/
						carrierData.rotation_deg = std::nullopt;
						sp<SpawnConfig> deafultCarrierConfigForTeam = activeMod->getDeafultCarrierConfigForTeam(teamIdx);
						carrierData.carrierShipSpawnConfig_name = deafultCarrierConfigForTeam ? deafultCarrierConfigForTeam->getName() : "";
						carrierData.fighterSpawnData.bEnableFighterRespawns = true;
						carrierData.fighterSpawnData.maxNumberOwnedFighterShips = 1000;
						carrierData.fighterSpawnData.respawnCooldownSec = 3.f;
					}
				}
			}
		}
	}

	void SpaceLevelConfig::setNumPlanets(size_t number)
	{
		numPlanets = number;

		//prevent json hacks from doing crazy size
		constexpr size_t planetLimit = 20;
		numPlanets = glm::clamp<size_t>(numPlanets, 0, planetLimit);

		while (numPlanets < number)
		{
			planets.emplace_back();
		}
	}

	bool SpaceLevelConfig::overridePlanetData(size_t idx, const PlanetData& inData)
	{
		if (idx < planets.size())
		{
			planets[idx] = inData;
		}
		else
		{
			//perhaps should just attempt to resize and see if it becomes available? that will be easier API but might create planets people are not expecting.
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Attempting to set planet data but array of planets does not have this index");
		}

		return false;
	}

	void SpaceLevelConfig::setNumStars(size_t number)
	{
		numStars = number;

		//prevent json hacks from doing crazy size
		constexpr size_t starLimit = 20;
		numStars = glm::clamp<size_t>(numStars, 0, starLimit);

		while (numStars < number)
		{
			stars.emplace_back();
		}
	}

	bool SpaceLevelConfig::overrideStarData(size_t idx, const StarData& inData)
	{
		if (idx < stars.size())
		{
			stars[idx] = inData;
		}
		else
		{
			//perhaps should just attempt to resize and see if it becomes available? that will be easier API but might create planets people are not expecting.
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Attempting to set star data but array of planets does not have this index");
		}

		return false;
	}

	//bool SpaceLevelConfig::replaceAvoidMeshData(size_t index, const WorldAvoidanceMeshData& data)
	//{
	//	if (Utils::isValidIndex(avoidanceMeshes, index))
	//	{
	//		avoidanceMeshes[index] = data;
	//		return true;
	//	}
	//	return false;
	//}

	std::string SpaceLevelConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("Assets/Levels/") + fileName + std::string("_") + userFacingName+ std::string(".json");
	}

	void SpaceLevelConfig::onSerialize(json& outData)
	{
		json spaceLevelData;
		spaceLevelData[SYMBOL_TO_STR(gamemodeTag)] = gamemodeTag;
		spaceLevelData[SYMBOL_TO_STR(userFacingName)] = userFacingName;

		auto writeEnvBodyData = [this](const EnvironmentalBodyData& environmentBodyData, json& j) 
		{
			//warning: name of this parameter defines field name!
			JSON_WRITE_OPTIONAL_VEC3(environmentBodyData.offsetDir, j);
			JSON_WRITE_OPTIONAL(environmentBodyData.offsetDistance, j);
		};
		auto writePlanetData = [this, writeEnvBodyData](const PlanetData& planetData, json& j)
		{
			//warning: name of this parameter defines field name!
			JSON_WRITE_OPTIONAL(planetData.texturePath, j);
			JSON_WRITE_OPTIONAL(planetData.bHasCivilization, j);
			writeEnvBodyData(planetData, j);
		};
		auto writeStarData = [this, writeEnvBodyData](const StarData& starData, json& j)
		{
			//warning: name of this parameter defines field name!
			JSON_WRITE_OPTIONAL_VEC3(starData.color, j);
			writeEnvBodyData(starData, j);
		};

		////////////////////////////////////////////////////////
		// environment planets
		////////////////////////////////////////////////////////
		json planetsArray;
		for (const PlanetData& planetData : planets)
		{
			json planetJson;
			writePlanetData(planetData, planetJson);
			planetsArray.push_back(planetJson);
		}
		spaceLevelData.push_back({ SYMBOL_TO_STR(planets), planetsArray });

		////////////////////////////////////////////////////////
		// environment stars
		////////////////////////////////////////////////////////
		json starArray;
		for (const StarData& starData : stars)
		{
			json starJson;
			writeStarData(starData, starJson);
			starArray.push_back(starJson);
		}
		spaceLevelData.push_back({ SYMBOL_TO_STR(stars), starArray });

		////////////////////////////////////////////////////////
		// avoidance mesh data (eg asteroids)
		////////////////////////////////////////////////////////
		json avoidanceArray;
		for (const WorldAvoidanceMeshData& avoidanceDatum : avoidanceMeshes)
		{
			json avoidanceJson;
			JSON_WRITE_TRANSFORM(avoidanceDatum.spawnTransform, avoidanceJson);
			JSON_WRITE(avoidanceDatum.spawnConfigName, avoidanceJson);
			avoidanceArray.push_back(avoidanceJson);
		}
		spaceLevelData[SYMBOL_TO_STR(avoidanceMeshes)] = avoidanceArray;

		////////////////////////////////////////////////////////
		// nebula data
		////////////////////////////////////////////////////////
		json nebulaArrayJson;
		for (const NebulaData& nebula : nebulaData)
		{
			json nebulaJson;
			JSON_WRITE(nebula.texturePath, nebulaJson);
			JSON_WRITE_VEC3(nebula.tintColor, nebulaJson);
			JSON_WRITE_TRANSFORM(nebula.transform, nebulaJson);
			nebulaArrayJson.push_back(nebulaJson);
		}
		spaceLevelData[SYMBOL_TO_STR(nebulaData)] = nebulaArrayJson;

		////////////////////////////////////////////////////////
		// carrier game mode data
		////////////////////////////////////////////////////////
		json json_carrierGamemodeData;
		{
			json json_teamArray;
			for(size_t teamIdx = 0; teamIdx < carrierGamemodeData.teams.size(); ++teamIdx)
			{
				json json_Team;
				GameModeData_CarrierTakedown::TeamData& teamData = carrierGamemodeData.teams[teamIdx];
				json json_carrierSpawnArray;
				for (size_t carrierIdx = 0; carrierIdx < teamData.carrierSpawnData.size(); ++carrierIdx)
				{
					CarrierSpawnData& carrierData = teamData.carrierSpawnData[carrierIdx];

					json json_carrierData;
					JSON_WRITE(carrierData.numInitialFighters, json_carrierData);
					JSON_WRITE_OPTIONAL_VEC3(carrierData.position, json_carrierData);
					JSON_WRITE_OPTIONAL_VEC3(carrierData.rotation_deg, json_carrierData);
					JSON_WRITE(carrierData.carrierShipSpawnConfig_name, json_carrierData);
					JSON_WRITE(carrierData.fighterSpawnData.bEnableFighterRespawns, json_carrierData);
					JSON_WRITE(carrierData.fighterSpawnData.maxNumberOwnedFighterShips, json_carrierData);
					JSON_WRITE(carrierData.fighterSpawnData.respawnCooldownSec, json_carrierData);
					json_carrierSpawnArray.push_back(json_carrierData);
				}
				json_Team[SYMBOL_TO_STR(teamData.carrierSpawnData)] = json_carrierSpawnArray;
				json_teamArray.push_back(json_Team);
			}
			json_carrierGamemodeData[SYMBOL_TO_STR(carrierGamemodeData.teams)] = json_teamArray;
		}
		spaceLevelData.push_back({ SYMBOL_TO_STR(carrierGamemodeData), json_carrierGamemodeData });
	
		outData.push_back({ getName(), spaceLevelData });
	}

	void SpaceLevelConfig::onDeserialize(const json& inData)
	{
		if (JsonUtils::has(inData, fileName))
		{
			const json& spaceLevelConfigJson = inData[fileName];

			READ_JSON_STRING_OPTIONAL(userFacingName, spaceLevelConfigJson);

			////////////////////////////////////////////////////////
			// helper lambdas
			////////////////////////////////////////////////////////
			auto readEnvBodyData = [this](EnvironmentalBodyData& environmentBodyData, const json& j)
			{
				//warning: name of this parameter defines field name!
				READ_JSON_FLOAT_OPTIONAL(environmentBodyData.offsetDistance, j);
				READ_JSON_VEC3_OPTIONAL(environmentBodyData.offsetDir, j);
			};
			auto readPlanetData = [this, readEnvBodyData](PlanetData& planetData, const json& j)
			{
				//warning: name of this parameter defines field name!
				READ_JSON_STRING_OPTIONAL(planetData.texturePath, j);
				READ_JSON_BOOL_OPTIONAL(planetData.bHasCivilization, j);
				readEnvBodyData(planetData, j);
			};
			auto readStarData = [this, readEnvBodyData](StarData& starData, const json& j)
			{
				//warning: name of this parameter defines field name!
				READ_JSON_VEC3_OPTIONAL(starData.color, j);
				readEnvBodyData(starData, j);
			};
			
			/////////////////////////////////////////////////////////////////////////////////////
			// read json
			/////////////////////////////////////////////////////////////////////////////////////

			READ_JSON_STRING_OPTIONAL(gamemodeTag, inData);
			READ_JSON_STRING_OPTIONAL(userFacingName, inData);

			////////////////////////////////////////////////////////
			// read planet data
			////////////////////////////////////////////////////////
			if (JsonUtils::hasArray(spaceLevelConfigJson, SYMBOL_TO_STR(planets)))
			{
				planets.clear(); //make sure the array is empty before we populate
				const json& planetsArray = spaceLevelConfigJson[SYMBOL_TO_STR(planets)];
				for (size_t planetIdx = 0; planetIdx < planetsArray.size(); ++planetIdx)
				{
					const json& planetJson = planetsArray[planetIdx];

					planets.push_back(PlanetData{});
					PlanetData& planetData = planets.back();
					readPlanetData(planetData, planetJson);
				}
			}

			////////////////////////////////////////////////////////
			// read star data
			////////////////////////////////////////////////////////
			if (JsonUtils::hasArray(spaceLevelConfigJson, SYMBOL_TO_STR(stars)))
			{
				stars.clear(); //make sure the array is empty before we populate
				const json& starArray = spaceLevelConfigJson[SYMBOL_TO_STR(stars)];
				for (size_t starIdx = 0; starIdx < starArray.size(); ++starIdx)
				{
					const json& starJson = starArray[starIdx];

					stars.push_back(StarData{});
					StarData& starData = stars.back();
					readStarData(starData, starJson);
				}
			}

			////////////////////////////////////////////////////////
			// read avoidance data
			////////////////////////////////////////////////////////
			if (JsonUtils::hasArray(spaceLevelConfigJson, SYMBOL_TO_STR(avoidanceMeshes)))
			{
				avoidanceMeshes.clear(); //clear so that we start with a fresh array
				const json& avoidanceArray = spaceLevelConfigJson[SYMBOL_TO_STR(avoidanceMeshes)];

				for (size_t meshIdx = 0; meshIdx < avoidanceArray.size(); ++meshIdx)
				{
					avoidanceMeshes.emplace_back();
					WorldAvoidanceMeshData& avoidanceDatum = avoidanceMeshes.back();

					const json& avoidanceJson = avoidanceArray[meshIdx];
					READ_JSON_TRANSFORM_OPTIONAL(avoidanceDatum.spawnTransform, avoidanceJson);
					READ_JSON_STRING_OPTIONAL(avoidanceDatum.spawnConfigName, avoidanceJson);
				}
			}

			////////////////////////////////////////////////////////
			// read nebula data
			////////////////////////////////////////////////////////
			if (JsonUtils::hasArray(spaceLevelConfigJson, SYMBOL_TO_STR(nebulaData)))
			{
				nebulaData.clear(); //reset all data and refresh it

				const json& nebulumArray = spaceLevelConfigJson[SYMBOL_TO_STR(nebulaData)];
				for (size_t nebIdx = 0; nebIdx < nebulumArray.size(); ++nebIdx)
				{
					nebulaData.emplace_back();
					NebulaData& nebula = nebulaData.back();

					const json& nebulaJson = nebulumArray[nebIdx];
					READ_JSON_STRING_OPTIONAL(nebula.texturePath, nebulaJson);
					READ_JSON_VEC3_OPTIONAL(nebula.tintColor, nebulaJson);
					READ_JSON_TRANSFORM_OPTIONAL(nebula.transform, nebulaJson);
				}
			}

			////////////////////////////////////////////////////////
			// read carrier take down game data
			////////////////////////////////////////////////////////
			if (JsonUtils::has(spaceLevelConfigJson, SYMBOL_TO_STR(carrierGamemodeData)))
			{
				const json& json_carrierGamemodeData = spaceLevelConfigJson[SYMBOL_TO_STR(carrierGamemodeData)];
				if(JsonUtils::hasArray(json_carrierGamemodeData, SYMBOL_TO_STR(carrierGamemodeData.teams)))
				{
					carrierGamemodeData.teams.clear(); //clear any previous data; 

					const json& json_teamArray = json_carrierGamemodeData[SYMBOL_TO_STR(carrierGamemodeData.teams)];
					for (size_t teamIdx = 0; teamIdx < json_teamArray.size(); ++teamIdx)
					{
						carrierGamemodeData.teams.push_back({});
						GameModeData_CarrierTakedown::TeamData& teamData = carrierGamemodeData.teams.back();

						const json& json_teamData = json_teamArray[teamIdx];

						if (JsonUtils::hasArray(json_teamData, SYMBOL_TO_STR(teamData.carrierSpawnData)))
						{
							teamData.carrierSpawnData.clear(); //clear any previous data

							const json& json_carrierSpawnArray = json_teamData[SYMBOL_TO_STR(teamData.carrierSpawnData)];
							for (size_t carrierIdx = 0; carrierIdx < json_carrierSpawnArray.size(); ++carrierIdx)
							{
								teamData.carrierSpawnData.push_back({});
								CarrierSpawnData& carrierData = teamData.carrierSpawnData.back();
								const json& json_carrierData = json_carrierSpawnArray[carrierIdx];

								READ_JSON_INT_OPTIONAL(carrierData.numInitialFighters, json_carrierData);
								READ_JSON_VEC3_OPTIONAL(carrierData.position, json_carrierData)
								READ_JSON_VEC3_OPTIONAL(carrierData.rotation_deg, json_carrierData);
								READ_JSON_STRING_OPTIONAL(carrierData.carrierShipSpawnConfig_name, json_carrierData);
								READ_JSON_BOOL_OPTIONAL(carrierData.fighterSpawnData.bEnableFighterRespawns, json_carrierData);
								READ_JSON_INT_OPTIONAL(carrierData.fighterSpawnData.maxNumberOwnedFighterShips, json_carrierData);
								READ_JSON_FLOAT_OPTIONAL(carrierData.fighterSpawnData.respawnCooldownSec, json_carrierData);
							}
						}
					}
				}
			}

			//fix up any dependent fields
			numPlanets = planets.size();
			numStars = stars.size();
		}
	}

	sp<SA::ServerGameMode_SpaceBase> createGamemodeFromTag(const std::string& tag)
	{
		sp<ServerGameMode_SpaceBase> newGamemode = nullptr;

		if (tag == TAG_GAMEMODE_CARRIER_TAKEDOWN)
		{
			return new_sp<ServerGameMode_CarrierTakedown>();
		}
		else if (tag == TAG_GAMEMODE_EXPLORE)
		{
			//TODO
			STOP_DEBUGGER_HERE();
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "passed invalid gamemode tag");
			STOP_DEBUGGER_HERE(); 
		}

		return newGamemode;
	}

}


