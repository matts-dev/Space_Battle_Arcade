#include "SASpaceLevelBase.h"
#include "../../Rendering/BuiltInShaders.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../GameSystems/SAProjectileSystem.h"
#include "../Team/Commanders.h"
#include "../Environment/StarField.h"
#include "../Environment/Star.h"
#include "../../Rendering/OpenGLHelpers.h"
#include "../Environment/Planet.h"
#include "../../Rendering/RenderData.h"
#include "../../GameFramework/SARenderSystem.h"
#include "../../Tools/Algorithms/SphereAvoidance/AvoidanceSphere.h"
#include <fwd.hpp>
#include <vector>
#include "../../GameFramework/SAGameEntity.h"
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "LevelConfigs/SpaceLevelConfig.h"
#include "../../GameFramework/SAAssetSystem.h"
#include "../../GameFramework/SALog.h"
#include "../../Tools/PlatformUtils.h"
#include "../SAShip.h"
#include "../Components/FighterSpawnComponent.h"
#include "../GameSystems/SAModSystem.h"
#include "../SpaceArcade.h"
#include "../OptionalCompilationMacros.h"

namespace SA
{
	using namespace glm;

	void SpaceLevelBase::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		using glm::vec3; using glm::mat4;

		GameBase& game = GameBase::get();

		const sp<PlayerBase>& zeroPlayer = game.getPlayerSystem().getPlayer(0);
		const RenderData* FRD = game.getRenderSystem().getFrameRenderData_Read(game.getFrameNumber());

		if (zeroPlayer && FRD)
		{
			const sp<CameraBase>& camera = zeroPlayer->getCamera();

			if (starField)
			{
				starField->render(dt_sec, FRD->view, FRD->projection);
			}

			//stars may be overlapping each other, so only clear depth once we've rendered all the solar system stars.
			ec(glClear(GL_DEPTH_BUFFER_BIT));
			for (const sp<Star>& star : localStars)
			{
				star->render(dt_sec, FRD->view, FRD->projection);
			}

			for (const sp<Planet>& planet : planets)
			{
				planet->render(dt_sec, FRD->view, FRD->projection);
			}
			ec(glClear(GL_DEPTH_BUFFER_BIT));

			//#todo a proper system for renderables should be set up; these uniforms only need to be set up front, not during each draw. It may also be advantageous to avoid virtual calls.
			forwardShadedModelShader->use();
			forwardShadedModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(FRD->view));
			forwardShadedModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(FRD->projection));
			forwardShadedModelShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightDiffuseIntensity", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightSpecularIntensity", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightAmbientIntensity", glm::vec3(0, 0, 0)); //perhaps drive this from level information
			for (size_t light = 0; light < FRD->dirLights.size(); ++light)
			{
				FRD->dirLights[light].applyToShader(*forwardShadedModelShader, light);
			}
			forwardShadedModelShader->setUniform3f("cameraPosition", camera->getPosition());
			forwardShadedModelShader->setUniform1i("material.shininess", 32);
			for (const sp<RenderModelEntity>& entity : renderEntities) 
			{
				forwardShadedModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(entity->getTransform().getModelMatrix()));
				entity->draw(*forwardShadedModelShader);
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "failed to get required resources for rendering");
		}
	}

	TeamCommander* SpaceLevelBase::getTeamCommander(size_t teamIdx)
	{
		return teamIdx < commanders.size() ? commanders[teamIdx].get() : nullptr;
	}

	void SpaceLevelBase::setConfig(const sp<const SpaceLevelConfig>& config)
	{
		levelConfig = config; //store config for when level starts.

	}
	void SpaceLevelBase::applyLevelConfig()
	{
		if (levelConfig)
		{
			sp<RNG> rng = nullptr;
			if (const std::optional<size_t>& seed = levelConfig->getSeed())
			{
				GameBase::get().getRNGSystem().getSeededRNG(*seed);
			}
			else
			{
				rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();
			}

			assert(rng);
			auto makeRandomVec3 = [rng]()
			{
				vec3 randomVec = vec3(rng->getFloat());

				//don't let zero vec occur
				if (glm::length2(randomVec) < 0.0001f)
				{
					randomVec = vec3(1, 0, 0);
				}

				return randomVec;
			};

			////////////////////////////////////////////////////////
			// set up planets
			////////////////////////////////////////////////////////
			planets.clear();
			for (const PlanetData& planet : levelConfig->getPlanets())
			{
				glm::vec3 planetDir = normalize(planet.offsetDir.has_value() ? *planet.offsetDir : makeRandomVec3());
				float planetDist = planet.offsetDistance.has_value() ? *planet.offsetDistance : rng->getFloat(1.0f, 30.f);
				bool bHasCivilization = planet.bHasCivilization.has_value() ? *planet.bHasCivilization : (rng->getInt(0, 8) == 0);

				Planet::Data init = {};
				init.albedo1_filepath = planet.texturePath;
				init.xform.position = planetDist * planetDir;

				if (bHasCivilization)
				{
					//if this was randomly generated, then be sure to use the earth-like planet
					if (const bool bWasRandomized = !planet.bHasCivilization.has_value())
					{
						//use the multi-layer material example
						init.albedo1_filepath = DefaultPlanetTexturesPaths::albedo_sea;
						init.albedo2_filepath = DefaultPlanetTexturesPaths::albedo_terrain;
						init.albedo3_filepath = DefaultPlanetTexturesPaths::albedo_terrain;
					}
					else
					{
						init.albedo2_filepath = init.albedo1_filepath; //this may not be necessary, but other code paths for nighttime lit planets set all albedos
						init.albedo3_filepath = init.albedo1_filepath;
					}
					init.nightCityLightTex_filepath = DefaultPlanetTexturesPaths::albedo_nightlight;
					init.colorMapTex_filepath = DefaultPlanetTexturesPaths::colorChanel;	//#future can make this a blend of a few textures for a random placeement effect
				}

				planets.push_back(new_sp<Planet>(init));
			}

			////////////////////////////////////////////////////////
			// set up stars
			////////////////////////////////////////////////////////
			localStars.clear();

			for (const StarData& star : levelConfig->getStars())
			{
				glm::vec3 starDir = normalize(star.offsetDir.has_value() ? *star.offsetDir : makeRandomVec3());
				float starDist = star.offsetDistance.has_value() ? *star.offsetDistance : rng->getFloat(1.0f, 30.f);
				vec3 starColor = star.color.has_value() ? *star.color : vec3(1.f);	//perhaps randomize color

				localStars.push_back(new_sp<Star>());
				const sp<Star>& newStar = localStars.back();
				newStar->setLightLDR(starColor);
				Transform starXform = {};
				starXform.position = starDir * starDist;
				newStar->setXform(starXform);
			}

			////////////////////////////////////////////////////////
			// game mode
			////////////////////////////////////////////////////////
			if (levelConfig->getGamemodeTag() == TAG_GAMEMODE_EXPLORE)
			{
				//#TODO look up spline points for path and spawn enemies along path
			}
			else //assume TAG_GAMEMODE_CARRIER_TAKEDOWN in all failure cases
			{
				/*TAG_GAMEMODE_CARRIER_TAKEDOWN*/
				applyLevelConfig_CarrierTakedownGameMode(*levelConfig, *rng);
			}
		}

	}

	static sp<SpawnConfig> getSpawnableConfigHelper(size_t idx, const SpawnConfig& sourceConfig, std::vector<sp<SpawnConfig>>& outCacheContainer)
	{
		sp<SpawnConfig> subConfig = nullptr;

		if (idx >= MAX_SPAWNABLE_SUBCONFIGS)
		{
			return subConfig;
		}

		//grow cache container to meet idx
		while (outCacheContainer.size() < (idx + 1) && outCacheContainer.size() != MAX_SPAWNABLE_SUBCONFIGS)
		{
			outCacheContainer.push_back(nullptr);
		}

		assert(outCacheContainer.size() > idx);

		if (outCacheContainer[idx] != nullptr)
		{
			//use the entry we generated last time we wanted a spawn config with this idx
			return outCacheContainer[idx];
		}
		else
		{
			//generate entry for the index position
			if (!subConfig)
			{
				const sp<ModSystem>& modSystem = SpaceArcade::get().getModSystem();
				sp<Mod> activeMod = modSystem->getActiveMod();
				if (!activeMod)
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "No active mod");
					return subConfig;
				}
				else
				{
					const std::map<std::string, sp<SpawnConfig>>& spawnConfigs = activeMod->getSpawnConfigs();

					const std::vector<std::string>& spawnableConfigsNames = sourceConfig.getSpawnableConfigsByName();
					if (idx < spawnableConfigsNames.size())
					{
						if (const auto& iter = spawnConfigs.find(spawnableConfigsNames[idx]); iter != spawnConfigs.end())
						{
							subConfig = iter->second;
						}
					}

					if (const bool bDidNotFindNamedConfig = !subConfig)
					{
						//attempt to load the default fighter config
						if (const auto& iter = spawnConfigs.find("Fighter"); iter != spawnConfigs.end())
						{
							subConfig = iter->second;
						}
					}

					//cache this away for next time
					outCacheContainer[idx] = subConfig;
				}
			}
		}


		return subConfig;
	}



	static void helper_ChooseRandomCarrierPositionAndRotation(
		const size_t teamIdx,
		const size_t numTeams,
		const size_t carrierIdxOnTeam,
		bool bRandomizeElevation,
		bool bRandomizeOffsetLocation,
		RNG& rng,
		glm::vec3& outLoc,
		glm::quat& outRotation)
	{
		using namespace glm;
		glm::vec3 worldUp = vec3(0, 1.f, 0);

		//divide map up into a circle, where all teams are facing towards center.
		float percOfCircle = teamIdx / float(numTeams);
		glm::quat centerRotation = glm::angleAxis(2*glm::pi<float>()*percOfCircle, worldUp);

		vec3 teamOffsetVec_n = glm::normalize(vec3(1, 0, 0) * centerRotation); //normalizing for good measure
		vec3 teamFacingDir_n = -teamOffsetVec_n;	//have entire team face same direction rather than looking at a center point; this will look more like a fleet.

		constexpr float teamOffsetFromMapCenterDistance = 150.f; //TODO perhaps define this in config so it can be customized
		vec3 teamLocation = teamOffsetVec_n * teamOffsetFromMapCenterDistance;
		vec3 teamRight = glm::normalize(glm::cross(worldUp, -teamFacingDir_n));

		constexpr float carrierOffsetFromTeamLocDistance = 50.f;
		float randomizedTeamOffsetFactor = 1.f;
		if (bRandomizeOffsetLocation)
		{
			randomizedTeamOffsetFactor = rng.getFloat(0.5f, 1.5f);
		}
		vec3 carrierBehindTeamLocOffset = (carrierOffsetFromTeamLocDistance * randomizedTeamOffsetFactor) * teamOffsetVec_n
			+ teamLocation;

		constexpr float carrierRightOffsetFactor = 300.f;
		vec3 carrierRightOffset = float(carrierIdxOnTeam) * carrierRightOffsetFactor * teamRight;

		vec3 carrierElevationOffset = vec3(0.f);
		if (bRandomizeElevation)
		{
			constexpr float elevantionMaxDist = 50.f;
			const float randomizedElevantionFactor = rng.getFloat(-1.f, 1.f);
			carrierElevationOffset = randomizedElevantionFactor * elevantionMaxDist * worldUp;
		}

		vec3 carrierLocation = carrierBehindTeamLocOffset + carrierRightOffset;

		outLoc = carrierLocation;
		outRotation = glm::angleAxis(glm::radians(180.f), worldUp) * centerRotation; //just flip center rotation by 180
	}

	void SpaceLevelBase::applyLevelConfig_CarrierTakedownGameMode(const SpaceLevelConfig& LevelConfigRef, RNG& rng)
	{
		const SpaceLevelConfig::GameModeData_CarrierTakedown& gm = LevelConfigRef.getGamemodeData_CarrierTakedown();
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			for (size_t teamIdx = 0; teamIdx < gm.teams.size(); ++teamIdx)
			{
				const SpaceLevelConfig::GameModeData_CarrierTakedown::TeamData& tm = gm.teams[teamIdx];

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				//spawn carrier ships
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				for (size_t carrierIdx = 0; carrierIdx < tm.carrierSpawnData.size(); ++carrierIdx)
				{
					CarrierSpawnData carrierData = tm.carrierSpawnData[carrierIdx];
					auto findCarrierResult = activeMod->getSpawnConfigs().find(carrierData.carrierShipSpawnConfig_name);
					if (findCarrierResult == activeMod->getSpawnConfigs().end())
					{
						std::string errorMsg = "Did not find carrier spawn config for name: " + carrierData.carrierShipSpawnConfig_name;
						log(__FUNCTION__, LogLevel::LOG_ERROR, errorMsg.c_str());
						continue;
					}
					else if (const sp<SpawnConfig>& carrierSpawnConfig = findCarrierResult->second)
					{
						glm::quat randomCarrierRot;
						glm::vec3 randomCarrierLoc;
						if (!carrierData.position.has_value() || !carrierData.rotation_deg.has_value())
						{
							helper_ChooseRandomCarrierPositionAndRotation(teamIdx, numTeams, carrierIdx, true, true, rng, randomCarrierLoc, randomCarrierRot);
						}

						Transform carrierXform;
						carrierXform.position = carrierData.position.has_value() ? *carrierData.position : randomCarrierLoc;
						carrierXform.rotQuat = carrierData.rotation_deg.has_value() ? quat(*carrierData.rotation_deg) : randomCarrierRot;

						Ship::SpawnData carrierShipSpawnData;
						carrierShipSpawnData.team = teamIdx;
						carrierShipSpawnData.spawnConfig = carrierSpawnConfig;
						carrierShipSpawnData.spawnTransform = carrierXform;

						if (sp<Ship> carrierShip = spawnEntity<Ship>(carrierShipSpawnData))
						{
							carrierShip->setSpeedFactor(0); //make carrier stationary

							FighterSpawnComponent::AutoRespawnConfiguration spawnFighterConfig{};
							spawnFighterConfig.bEnabled= carrierData.fighterSpawnData.bEnableFighterRespawns;
							spawnFighterConfig.maxShips = carrierData.fighterSpawnData.maxNumberOwnedFighterShips;
							spawnFighterConfig.respawnCooldownSec = carrierData.fighterSpawnData.respawnCooldownSec;
							uint32_t numInitFighters = carrierData.numInitialFighters;
#ifdef DEBUG_BUILD
							log(__FUNCTION__, LogLevel::LOG, "Scaling down number of spawns because this is a debug build");
							float scaledownFactor = 0.05f;
							spawnFighterConfig.maxShips = uint32_t(spawnFighterConfig.maxShips * scaledownFactor); //scale down for debug builds
							numInitFighters = uint32_t(numInitFighters * scaledownFactor); //scale down for debug builds
#endif 
							if (FighterSpawnComponent* spawnComp = carrierShip->getGameComponent<FighterSpawnComponent>())
							{
								spawnComp->setAutoRespawnConfig(spawnFighterConfig);

								////////////////////////////////////////////////////////////////////////////////////////////////////////////////
								//spawn initial fighters around carrier
								////////////////////////////////////////////////////////////////////////////////////////////////////////////////
								Ship::SpawnData fighterShipSpawnData;
								fighterShipSpawnData.team = teamIdx;

								std::vector<sp<SpawnConfig>> cachedSpawnConfigs;//cache spawn configs so we're not hitting maps over and over, rather we hit them once then use an idx
								fighterShipSpawnData.spawnConfig = getSpawnableConfigHelper(0, *carrierSpawnConfig, cachedSpawnConfigs);

								if (fighterShipSpawnData.spawnConfig)
								{
									for (uint32_t fighterShip = 0; fighterShip < numInitFighters; ++fighterShip)
									{
										const float spawnRange = 50.f;

										glm::vec3 startPos(rng.getFloat<float>(-spawnRange, spawnRange), rng.getFloat<float>(-spawnRange, spawnRange), rng.getFloat<float>(-spawnRange, spawnRange));
										float randomRot_rad = glm::radians(rng.getFloat<float>(0, 360));
										glm::quat rot = glm::angleAxis(randomRot_rad, glm::vec3(0, 1, 0));
										startPos += carrierXform.position;
										fighterShipSpawnData.spawnTransform = Transform{ startPos, rot, glm::vec3(1.f) };

										sp<Ship> fighter = spawnComp->spawnEntity(); //the carrier ship is responsible for assigning brain after a span happens via callback
										fighter->setTransform(fighterShipSpawnData.spawnTransform);
									}
								}
								else { STOP_DEBUGGER_HERE(); }
							}
						}
					}
					else
					{
						log(__FUNCTION__, LogLevel::LOG_WARNING, "No carrier spawn config available; what is the model that should go with this carrier ship?");
						STOP_DEBUGGER_HERE();
					}
				}

			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "No active mod to use as lookup for spawn configs -- aborting application of carrier takedown data");
		}
		
	}

	void SpaceLevelBase::startLevel_v()
	{
		LevelBase::startLevel_v();

		forwardShadedModelShader = new_sp<SA::Shader>(spaceModelShader_forward_vs, spaceModelShader_forward_fs, false);

		for (size_t teamId = 0; teamId < numTeams; ++teamId)
		{
			commanders.push_back(new_sp<TeamCommander>(teamId));
		}

		applyLevelConfig();
	}

	void SpaceLevelBase::endLevel_v()
	{
		//this means that we're going to generate a new shader between each level transition.
		//this can be avoided with static members or by some other mechanism, but I do not see 
		//transitioning levels being a slow process currently, so each level gets its own shaders.
		forwardShadedModelShader = nullptr;

		LevelBase::endLevel_v();
	}

	void SpaceLevelBase::postConstruct()
	{
		createTypedGrid<AvoidanceSphere>(glm::vec3(128));
		
		starField = onCreateStarField();
		{ 
			bGeneratingLocalStars = true;
			onCreateLocalStars();
			bGeneratingLocalStars = false;

			onCreateLocalPlanets();
		}

		refreshStarLightMapping();
	}

	void SpaceLevelBase::tick_v(float dt_sec)
	{
		LevelBase::tick_v(dt_sec);

		for (sp<Planet>& planet : planets)
		{
			planet->tick(dt_sec);
		}
	}

	sp<SA::StarField> SpaceLevelBase::onCreateStarField()
	{
		//by default generate a star field, if no star field should be in the level return null in an override;
		sp<StarField> defaultStarfield = new_sp<StarField>();
		return defaultStarfield; 
	}

	void SpaceLevelBase::refreshStarLightMapping()
	{
		uint32_t MAX_DIR_LIGHTS = GameBase::getConstants().MAX_DIR_LIGHTS;
		if (localStars.size() > MAX_DIR_LIGHTS)
		{
			log(__FUNCTION__,LogLevel::LOG_WARNING, "More local stars than there exist slots for directional lights");
		}

		for (size_t starIdx = 0; starIdx < localStars.size() && starIdx < MAX_DIR_LIGHTS; starIdx++)
		{
			if (sp<Star> star = localStars[starIdx])
			{
				if (dirLights.size() <= starIdx)
				{
					dirLights.push_back({});
				}
				dirLights[starIdx].direction_n = normalize(star->getLightDirection());
				dirLights[starIdx].lightIntensity = star->getLightLDR(); //#TODO #HDR
			}
		}
	}

	void SpaceLevelBase::onCreateLocalStars()
	{
		Transform defaultStarXform;
		defaultStarXform.position = glm::vec3(50, 50, 50);

		sp<Star> defaultStar = new_sp<Star>();
		defaultStar->setXform(defaultStarXform);
		localStars.push_back(defaultStar);
	}

	std::vector<sp<class Planet>> makeRandomizedPlanetArray(RNG& rng)
	{
		//std::string(DefaultPlanetTexturesPaths::albedo_terrain);
		//std::string(DefaultPlanetTexturesPaths::albedo_sea);
		//std::string(DefaultPlanetTexturesPaths::albedo_nightlight);

		std::vector<std::string> defaultTextures = {
			std::string( DefaultPlanetTexturesPaths::albedo1  ),
			std::string( DefaultPlanetTexturesPaths::albedo2  ),
			std::string( DefaultPlanetTexturesPaths::albedo3  ),
			std::string( DefaultPlanetTexturesPaths::albedo4  ),
			std::string( DefaultPlanetTexturesPaths::albedo5  ),
			std::string( DefaultPlanetTexturesPaths::albedo6  ),
			std::string( DefaultPlanetTexturesPaths::albedo7  ),
			std::string( DefaultPlanetTexturesPaths::albedo8  )
		};

		//generate all planets
		std::vector<sp<Planet>> planetArray;

		auto randomizeLoc = [](Planet::Data& init, RNG& rng)
		{
			float distance = rng.getFloat(50.0f, 100.0f);
			vec3 pos{ rng.getFloat(-20.f, 20.f), rng.getFloat(-20.f, 20.f), rng.getFloat(-20.f, 20.f) };
			pos.x = (pos.x == 0) ? 0.1f : pos.x; //make sure normalization cannot divide by 0, which will create nans
			pos = normalize(pos + vec3(0.1f)) * distance;
			init.xform.position = pos;
		};

		//make special first planet
		{
			//use the multi-layer material example
			Planet::Data init;
			init.albedo1_filepath = DefaultPlanetTexturesPaths::albedo_sea;
			init.albedo2_filepath = DefaultPlanetTexturesPaths::albedo_terrain;
			init.albedo3_filepath = DefaultPlanetTexturesPaths::albedo_terrain;
			init.nightCityLightTex_filepath = DefaultPlanetTexturesPaths::albedo_nightlight;
			init.colorMapTex_filepath = DefaultPlanetTexturesPaths::colorChanel;
			randomizeLoc(init, rng);
			planetArray.push_back(new_sp<Planet>(init));
		}

		//use textures to generate array of planets
		for (const std::string& texture : defaultTextures)
		{
			Planet::Data init;
			init.albedo1_filepath = texture;
			randomizeLoc(init, rng);
			planetArray.push_back(new_sp<Planet>(init));
		}

		//randomize order
		for (size_t idx = 0; idx < planetArray.size(); ++idx)
		{
			uint32_t randomSwapIdx = rng.getInt<size_t>(0, planetArray.size() - 1);

			sp<Planet> swapOut = planetArray[idx];
			planetArray[idx] = planetArray[randomSwapIdx];
			planetArray[randomSwapIdx] = swapOut;
		}

		//return randomized order
		return planetArray;
	}
}

