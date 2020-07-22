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
#include "../GameModes/ServerGameMode_SpaceBase.h"
#include "MainMenuLevel.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../AssetConfigs/SaveGameConfig.h"
#include "../AssetConfigs/CampaignConfig.h"
#include "../SAPlayer.h"
#include <xutility>

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

	SA::ServerGameMode_SpaceBase* SpaceLevelBase::getServerGameMode_SpaceBase()
	{
		if (bool bIsServer = true) //#multiplayer
		{
			return spaceGameMode.get();
		}
		return nullptr;
	}

	void SpaceLevelBase::endGame(const EndGameParameters& endParameters)
	{
		//call virtual onGameEnding() here if we need to add virtual

		onGameEnding.broadcast(endParameters); //let everyone know we're ending

		if (!endTransitionTimerDelegate)
		{
			endTransitionTimerDelegate = new_sp<MultiDelegate<>>();
			endTransitionTimerDelegate->addWeakObj(sp_this(), &SpaceLevelBase::transitionToMainMenu);
		}
		getWorldTimeManager()->createTimer(endTransitionTimerDelegate, endParameters.delayTransitionMainmenuSec);

		bool bWonMatch = false;
		if (SAPlayer* player = dynamic_cast<SAPlayer*>(SpaceArcade::get().getPlayerSystem().getPlayer(0).get()))
		{
			bWonMatch = std::find(endParameters.winningTeams.begin(), endParameters.winningTeams.end(), player->getCurrentTeamIdx()) != endParameters.winningTeams.end();
		}

		///////////////////////////////////////////////////////////////////////////////////////////////
		//mark this level as complete in campaign and save so user can progress to next level
		///////////////////////////////////////////////////////////////////////////////////////////////
		if (bWonMatch)
		{
			const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
			if (levelConfig && levelConfig->transientData.levelIdx.has_value() && activeMod)
			{
				size_t completedLevelIdx = *levelConfig->transientData.levelIdx;
				size_t activeCampaignIdx = activeMod->getActiveCampaignIndex();
				sp<CampaignConfig> campaign = activeMod->getCampaign(activeCampaignIdx);
				sp<SaveGameConfig> saveGameConfig = activeMod->getSaveGameConfig();
				if (saveGameConfig && campaign)
				{
					if (SaveGameConfig::CampaignData* campaignData = saveGameConfig->findCampaignByName_Mutable(campaign->getRepresentativeFilePath()))
					{
						auto& lvls = campaignData->completedLevels; //so annoying std::vector doesn't have a member function find, compressing name into levels
						if (bool bNeedsInsertion = std::find(lvls.begin(), lvls.end(), completedLevelIdx) == lvls.end())
						{
							campaignData->completedLevels.push_back(completedLevelIdx);
							saveGameConfig->requestSave();
						}
					}
				}
			}
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// detach all players from ships as we're about to do a level transition
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		PlayerSystem& playerSystem = SpaceArcade::get().getPlayerSystem();
		for (size_t playerIdx = 0; playerIdx < playerSystem.numPlayers(); ++playerIdx)
		{
			if (const sp<PlayerBase>& player = playerSystem.getPlayer(playerIdx))
			{
				//clear out any ships the player may be piloting
				player->setControlTarget(nullptr);
			}
		}
	}

	void SpaceLevelBase::applyLevelConfig()
	{
		if (levelConfig)
		{
			if (const std::optional<size_t>& seed = levelConfig->getSeed())
			{
				GameBase::get().getRNGSystem().getSeededRNG(*seed);
			}
			else
			{
				generationRNG = GameBase::get().getRNGSystem().getTimeInfluencedRNG();
			}

			assert(generationRNG);
			auto makeRandomVec3 = [this]()
			{
				vec3 randomVec = vec3(generationRNG->getFloat());

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
				float planetDist = planet.offsetDistance.has_value() ? *planet.offsetDistance : generationRNG->getFloat(1.0f, 30.f);
				bool bHasCivilization = planet.bHasCivilization.has_value() ? *planet.bHasCivilization : (generationRNG->getInt(0, 8) == 0);

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
				float starDist = star.offsetDistance.has_value() ? *star.offsetDistance : generationRNG->getFloat(1.0f, 30.f);
				vec3 starColor = star.color.has_value() ? *star.color : vec3(1.f);	//perhaps randomize color

				localStars.push_back(new_sp<Star>());
				const sp<Star>& newStar = localStars.back();
				newStar->setLightLDR(starColor);
				Transform starXform = {};
				starXform.position = starDir * starDist;
				newStar->setXform(starXform);
			}
		}
	}

	void SpaceLevelBase::transitionToMainMenu()
	{
		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		sp<LevelBase> mainMenuLevel = new_sp<MainMenuLevel>(); //this requires include of entire main menu level, which feels weird (this is mainmenus base). but I suppose it isn't that weird
		levelSystem.loadLevel(mainMenuLevel);
	}

	void SpaceLevelBase::startLevel_v()
	{
		LevelBase::startLevel_v();

		forwardShadedModelShader = new_sp<SA::Shader>(spaceModelShader_forward_vs, spaceModelShader_forward_fs, false);

		//we don't know how many teams there will be after we load gamemode (in apply config), so make all teamcommands now
		//#todo change this. this isn't great because we need team commanders to be around before spawning so they can be aware of when we spawn ships
		//otherwise we won't have the ability to ask a commander for a target. currently just building maximum number of commanders
		for (size_t teamId = 0; teamId < MAX_TEAM_NUM; ++teamId)
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

		if (spaceGameMode)
		{
			spaceGameMode->tick(dt_sec, ServerGameMode_SpaceBase::LevelKey{});
		}
	}

	sp<SA::ServerGameMode_Base> SpaceLevelBase::onServerCreateGameMode()
	{
		if (levelConfig)
		{
			if (bool bIsServer = true) //#TODO #multiplayer
			{
				if (spaceGameMode = createGamemodeFromTag(levelConfig->gamemodeTag))
				{
					spaceGameMode->setOwningLevel(sp_this());
					spaceGameMode->initialize(ServerGameMode_SpaceBase::LevelKey{});
				}
			}
		}
		return spaceGameMode;
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

