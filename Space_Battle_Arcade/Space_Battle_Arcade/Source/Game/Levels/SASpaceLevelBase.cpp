#include "SASpaceLevelBase.h"
#include "../../Rendering/BuiltInShaders.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "GameFramework/SAGameBase.h"
#include "Tools/DataStructures/SATransform.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../GameSystems/SAProjectileSystem.h"
#include "../Team/Commanders.h"
#include "../Environment/StarField.h"
#include "../Environment/Star.h"
#include "../../Rendering/OpenGLHelpers.h"
#include "Game/Environment/Planet.h"
#include "../../Rendering/RenderData.h"
#include "../../GameFramework/SARenderSystem.h"
#include "../../Tools/Algorithms/SphereAvoidance/AvoidanceSphere.h"
#include <fwd.hpp>
#include <vector>
#include "GameFramework/SAGameEntity.h"
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "LevelConfigs/SpaceLevelConfig.h"
#include "../../GameFramework/SAAssetSystem.h"
#include "GameFramework/SALog.h"
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
#include "../Rendering/CustomGameShaders.h"
#include "../AI/GlobalSpaceArcadeBehaviorTreeKeys.h"
#include "../AI/SAShipBehaviorTreeNodes.h"
#include "../../GameFramework/RenderModelEntity.h"
#include "../../GameFramework/SAWorldEntity.h"
#include "../Cameras/SAShipCamera.h"
#include "../Environment/Nebula.h"
#include "../GameEntities/AvoidMesh.h"
#include "../UI/GameUI/SAHUD.h"

namespace SA
{
	using namespace glm;

	void SpaceLevelBase::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		using glm::vec3; using glm::mat4;

		SpaceArcade& game = SpaceArcade::get();

		sj.tick(dt_sec);

		const sp<PlayerBase>& zeroPlayer = game.getPlayerSystem().getPlayer(0);
		const RenderData* FRD = game.getRenderSystem().getFrameRenderData_Read(game.getFrameNumber());

		if (zeroPlayer && FRD)
		{
			const sp<CameraBase>& camera = zeroPlayer->getCamera();

#if !IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
			todo_update_star_FIELD_shader_to_be_deferred;
			todo_update_star_shader_to_be_deferred;
			todo_update_planet_shader_to_be_deferred;
#endif //IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE

			//render space clouds!
			ec(glEnable(GL_BLEND));
			ec(glDisable(GL_DEPTH_TEST)); //don't use depth testing while rendering nebula
			ec(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
			for (const sp<Nebula>& nebulum : nebulae) 
			{
				nebulum->render(*FRD);
			}
			//ec(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
			ec(glEnable(GL_DEPTH_TEST));
			ec(glDisable(GL_BLEND));

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

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// prepare stencil highlight data
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			const std::vector<sp<PlayerBase>>& allPlayers = game.getPlayerSystem().getAllPlayers();
			if (game.bEnableStencilHighlights)
			{
				if (game.bOnlyHighlightTargets)
				{
					for (const sp<PlayerBase>& player : allPlayers)
					{
						if (IControllable* controlTarget = player->getControlTarget())
						{
							if (WorldEntity* controlTarget_we = controlTarget->asWorldEntity())
							{
								if (const BrainComponent* brainComp = controlTarget_we->getGameComponent<BrainComponent>())
								{
									if (const BehaviorTree::Tree* tree = brainComp->getTree())
									{
										BehaviorTree::Memory& memory = tree->getMemory();
										if (const BehaviorTree::ActiveAttackers* attackerMap = memory.getReadValueAs<BehaviorTree::ActiveAttackers>(BT_AttackersKey))
										{
											for (auto& iter : *attackerMap)
											{
												if (iter.second.attacker)
												{
													//dynamic cast sucks, but this will likely only be a few per frame. alternatively could set up a component for this. #nextengine in general, find a design to remove this issue of subclass casting
													if (RenderModelEntity* attacker = dynamic_cast<RenderModelEntity*>(iter.second.attacker.fastGet()))
													{
														stencilHighlightEntities.push_back(attacker);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				else /*Render all ship highlights. */
				{
					//since we're going to be rendering a whole bunch of highlights, reserve the array size to match upper bound of what we're rendering.
					stencilHighlightEntities.reserve(renderEntities.size()); 

					for (const sp<RenderModelEntity>& renderEntity : renderEntities)
					{
						//don't render carrier ships with highlights
						if (renderEntity)
						{
							//ideally I'd would do this this in a more systemic way, but running out of time to finish this. check if this is a spawner (carrier) and don't highlight. 
							//Perhaps a highlight component could be added and that could be checked for a more roboust system.
							if (!renderEntity->hasGameComponent<FighterSpawnComponent>()
								&& ! renderEntity->hasGameComponent<OwningPlayerComponent>()//make sure it isn't player
								&& renderEntity->hasGameComponent<TeamComponent>() //make sure its not an asteroid
							)
							{
								stencilHighlightEntities.push_back(renderEntity.get());
							}
						}
					}
				}
			}


#if !IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
			todo_update_model_shader_to_be_deferred;
#endif //IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
			CustomGameShaders& gameCustomShaders = SpaceArcade::get().getGameCustomShaders();
			gameCustomShaders.forwardModelShader = forwardShadedModelShader;

			/////////////////////////////////////////////////////////////////////////////////////
			// prepare forward shader uniforms
			/////////////////////////////////////////////////////////////////////////////////////
			//#todo a proper system for renderables should be set up; these uniforms only need to be set up front, not during each draw. It may also be advantageous to avoid virtual calls.
			forwardShadedModelShader->use();
			forwardShadedModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(FRD->view));
			forwardShadedModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(FRD->projection));
			forwardShadedModelShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightDiffuseIntensity", glm::vec3(0, 0, 0)); //#TODO remove these from shader if they're not used
			forwardShadedModelShader->setUniform3f("lightSpecularIntensity", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("lightAmbientIntensity", glm::vec3(0, 0, 0)); //perhaps drive this from level information
			forwardShadedModelShader->setUniform1i("renderMode", int(renderMode));
			if (useNormalMappingOverride.has_value())					{ forwardShadedModelShader->setUniform1i("bUseNormalMapping", *useNormalMappingOverride); }
			if (useNormalMappingMirrorCorrectionOverride.has_value())	{ forwardShadedModelShader->setUniform1i("bUseMirrorUvNormalCorrection", *useNormalMappingMirrorCorrectionOverride); }
			if (correctNormalMapSeamsOverride.has_value())				{ forwardShadedModelShader->setUniform1i("bUseNormalSeamCorrection", *correctNormalMapSeamsOverride); }

			for (size_t light = 0; light < FRD->dirLights.size(); ++light)
			{
				FRD->dirLights[light].applyToShader(*forwardShadedModelShader, light);
			}
			forwardShadedModelShader->setUniform3f("cameraPosition", camera->getPosition());
			forwardShadedModelShader->setUniform1i("material.shininess", 32);

			bool bShouldRenderWorldUnits = true;
			bShouldRenderWorldUnits &= !(sj.isStarJumpInProgress());
			if(bShouldRenderWorldUnits)
			{

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// custom target highlighting pass
				//
				// render ship again, but this time writing to stencil buffer
				// render scaled up ship with highlight shader, but only if passes stencil and depth test.
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				uint32_t stencilHighlightBit = 1; //#stencil todo - define this in a global place so that all stencil bits can been seen in one location
				if (stencilHighlightEntities.size() > 0)
				{
					//prepare_stencil_write;
					ec(glEnable(GL_STENCIL_TEST));
					ec(glStencilFunc(GL_ALWAYS, stencilHighlightBit, 0xFF)); //configure the bit to write/read
					ec(glStencilMask(0xFF)); //enable writing to all bits of the stencil buffer
					ec(glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE));
					for (RenderModelEntity* highlightEntity : stencilHighlightEntities)
					{
						//render like normal, but writing to stencil buffer so that highlight will not overwrite object
						forwardShadedModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(highlightEntity->getTransform().getModelMatrix()));
						highlightEntity->render(*forwardShadedModelShader);
					}
					//clear_stencil_write;
					ec(glStencilMask(0)); //disable writing to stencil buffer
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// regular rendering pass
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				for (const sp<RenderModelEntity>& entity : renderEntities) 
				{
					forwardShadedModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(entity->getTransform().getModelMatrix()));
					entity->render(*forwardShadedModelShader);
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// highlight pass
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (stencilHighlightEntities.size() > 0)
				{
					ec(glStencilFunc(GL_NOTEQUAL, stencilHighlightBit, 0xFF)); //only render if we haven't stenciled this area

#if !IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
					todo_update_highlight_shader_to_be_deferred;
#endif //IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
					highlightForwardModelShader->use();

					mat4 highlightScaleUp = glm::scale(mat4(1.f), vec3(2.f));
					highlightForwardModelShader->setUniform1f("vertNormalOffsetScalar", 1.f);
					highlightForwardModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(FRD->view));
					highlightForwardModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(FRD->projection));

					//set highligh color, this will be enemy color unless game is in mode where all highlights are rendered, then we will change based on if we're rendering a teammate or an enemy
					size_t playerTeam = 0;
					if (SAPlayer* player = dynamic_cast<SAPlayer*>(GameBase::get().getPlayerSystem().getPlayer(0).get()))
					{
						playerTeam = player->getCurrentTeamIdx();
					}
					/*vec3 highlightColor = vec3(0.8f);*/
					float highlightHdrMultiplier = GameBase::get().getRenderSystem().isUsingHDR() ? 2.f : 1.f;//@hdr_tweak
					vec3 enemyHighlightColor = vec3(0.5f, 0, 0) * highlightHdrMultiplier;
					vec3 teamHighlightColor = vec3(vec2(0.1f), 0.5f) * highlightHdrMultiplier;
					vec3 highlightColor = enemyHighlightColor;
					highlightForwardModelShader->setUniform3f("color", highlightColor);

					for (RenderModelEntity* highlightEntity : stencilHighlightEntities)
					{
						//set color to team color? will require component or something to get that information
						highlightForwardModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(highlightEntity->getTransform().getModelMatrix()));
						
						if (!game.bOnlyHighlightTargets)
						{
							//if rendering all ships, their color needs to match whether they're an enemy or a friendly
							if (TeamComponent* teamComp = highlightEntity->getGameComponent<TeamComponent>())
							{
								highlightColor = (teamComp->getTeam() == playerTeam) ? teamHighlightColor : enemyHighlightColor;
							}
							else
							{
								//no team component, assume enemy;
								highlightColor = enemyHighlightColor;
							}
							highlightForwardModelShader->setUniform3f("color", highlightColor);
						}

						highlightEntity->render(*highlightForwardModelShader);
					}
					stencilHighlightEntities.clear(); //clear raw pointers so they will be regenerated next frame

					//clean up stencil state so that other features can use stencil buffer
					ec(glStencilMask(0xFF));
					ec(glClear(GL_STENCIL_BUFFER_BIT));
					ec(glStencilFunc(GL_ALWAYS, stencilHighlightBit, 0xFF)); //reset requirement that we haven't stenciled area
					ec(glStencilMask(0x0)); //disable writes
					ec(glDisable(GL_STENCIL_TEST));
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// debug the generated normals from normal mapping
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (bDebugNormals)
				{
					debugNormalMapShader->use();
					debugNormalMapShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(FRD->view));
					debugNormalMapShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(FRD->projection));
					for (size_t light = 0; light < FRD->dirLights.size(); ++light)
					{
						FRD->dirLights[light].applyToShader(*debugNormalMapShader, light);
					}

					//ec(glDisable(GL_DEPTH_TEST));
					for (const sp<RenderModelEntity>& entity : renderEntities)
					{
						debugNormalMapShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(entity->getTransform().getModelMatrix()));
						entity->render(*debugNormalMapShader);
					}
					//ec(glEnable(GL_DEPTH_TEST));
				}
			}
			else
			{
				//special case, we want to still render the player while star jumping
				const std::vector<sp<PlayerBase>>& allPlayers = GameBase::get().getPlayerSystem().getAllPlayers();
				for (const sp<PlayerBase>& player : allPlayers)
				{
					if (RenderModelEntity* playerModel = dynamic_cast<RenderModelEntity*>(player->getControlTarget()))
					{
						playerModel->render(*forwardShadedModelShader);
					}
				}
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
		if (const sp<TimeManager>& worldTimeManager = getWorldTimeManager())
		{
			//won't have a timer if we're ending the game
			getWorldTimeManager()->createTimer(endTransitionTimerDelegate, endParameters.delayTransitionMainmenuSec);
			enableStarJump(true);
		}

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
		///detach players once the timer is up
		//PlayerSystem& playerSystem = SpaceArcade::get().getPlayerSystem();
		//for (size_t playerIdx = 0; playerIdx < playerSystem.numPlayers(); ++playerIdx)
		//{
		//	if (const sp<PlayerBase>& player = playerSystem.getPlayer(playerIdx))
		//	{
		//		//clear out any ships the player may be piloting
		//		player->setControlTarget(nullptr);
		//	}
		//}
	}

	void SpaceLevelBase::enableStarJump(bool bEnable, bool bSkipTransition /*= false*/)
	{
		sj.enableStarJump(bEnable, bSkipTransition);

		starField->enableStarJump(bEnable, bSkipTransition);

		for (const sp<Planet>& planet : planets)
		{
			planet->enableStarJump(bEnable, bSkipTransition);
		}

		for (const sp<Star>& star : localStars)
		{
			star->enableStarJump(bEnable, bSkipTransition);
		}

		for (const sp<Nebula>& nebula : nebulae)
		{
			nebula->enableStarJump(bEnable, bSkipTransition);
		}

		const std::vector<sp<PlayerBase>>& allPlayers = GameBase::get().getPlayerSystem().getAllPlayers();
		for (const sp<PlayerBase>& player : allPlayers)
		{
			const sp<CameraBase>& camera = player ? player->getCamera() : nullptr;

			if (ShipCamera* shipCamera = dynamic_cast<ShipCamera*>(camera.get()))
			{
				shipCamera->enableStarJump(bEnable, bSkipTransition);
			}
		}
	}

	void SpaceLevelBase::staticEnableStarJump(bool bEnable, bool bSkipTransition /*= false*/)
	{
		//star jump
		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel();
		if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(currentLevel.get()))
		{
			spaceLevel->enableStarJump(bEnable, bSkipTransition);
		}
	}

	bool SpaceLevelBase::isStarJumping() const
	{
		//just use the star jump data form the star field, all other things (stars, planets) should match
		return sj.isStarJumpInProgress();
	}

	void SpaceLevelBase::copyPlanetDataToInitData(const PlanetData& planet, Planet::Data& init)
	{
		assert(generationRNG);

		glm::vec3 planetDir = normalize(planet.offsetDir.has_value() ? *planet.offsetDir : makeRandomVec3());
		float planetDist = planet.offsetDistance.has_value() ? *planet.offsetDistance : generationRNG->getFloat(2.0f, 20.f);
		bool bHasCivilization = planet.bHasCivilization.has_value() ? *planet.bHasCivilization : (generationRNG->getInt(0, 8) == 0);

		//init.albedo1_filepath = planet.texturePath.has_value() ? *planet.texturePath : DefaultPlanetTexturesPaths::albedo_terrain; //only copy if there is a value; otherwise use whatever is init as default (prevent crashes in level editor)
		if (planet.texturePath.has_value())
		{
			init.albedo1_filepath = *planet.texturePath;
		}
		else if (generationRNG) 
		{
			std::vector<std::string> defaultTextures = {
				std::string(DefaultPlanetTexturesPaths::albedo1),
				std::string(DefaultPlanetTexturesPaths::albedo2),
				std::string(DefaultPlanetTexturesPaths::albedo3),
				std::string(DefaultPlanetTexturesPaths::albedo4),
				std::string(DefaultPlanetTexturesPaths::albedo5),
				std::string(DefaultPlanetTexturesPaths::albedo6),
				std::string(DefaultPlanetTexturesPaths::albedo7),
				std::string(DefaultPlanetTexturesPaths::albedo8)
			};
			init.albedo1_filepath = defaultTextures[generationRNG->getInt<size_t>(0, defaultTextures.size() - 1)];
		}
		init.xform.position = planetDist * planetDir;

		if (bHasCivilization)
		{
			//if this was randomly generated, then be sure to use the earth-like planet
			if (const bool bWasRandomized = !planet.bHasCivilization.has_value())
			{
				int oneInX_generateCivilization = 5;
				if (generationRNG && (generationRNG->getInt(0, oneInX_generateCivilization) == 0))
				{
					//use the multi-layer material example
					init.albedo1_filepath = DefaultPlanetTexturesPaths::albedo_sea;
					init.albedo2_filepath = DefaultPlanetTexturesPaths::albedo_terrain;
					init.albedo3_filepath = DefaultPlanetTexturesPaths::albedo_terrain;
				}
			}
			else
			{
				init.albedo2_filepath = init.albedo1_filepath; //this may not be necessary, but other code paths for nighttime lit planets set all albedos
				init.albedo3_filepath = init.albedo1_filepath;
			}
			init.nightCityLightTex_filepath = DefaultPlanetTexturesPaths::albedo_nightlight;
			init.colorMapTex_filepath = DefaultPlanetTexturesPaths::colorChanel;	//#future can make this a blend of a few textures for a random placeement effect
		}
	}

	glm::vec3 SpaceLevelBase::makeRandomVec3()
	{
		if (generationRNG)
		{
			vec3 randomVec = vec3(generationRNG->getFloat(-1.f, 1.f), generationRNG->getFloat(-1.f, 1.f), generationRNG->getFloat(-1.f, 1.f));

			//don't let zero vec occur
			if (glm::length2(randomVec) < 0.0001f)
			{
				randomVec = vec3(1, 0, 0);
			}

			return randomVec;
		}
		else
		{
			STOP_DEBUGGER_HERE();
			return vec3(1.f, 0.f, 0.f);
		}
	}

	void SpaceLevelBase::applyLevelConfig()
	{
		if (levelConfig)
		{
			if (const std::optional<size_t>& seed = levelConfig->getSeed())
			{
				if (!generationRNG)
				{
					generationRNG = GameBase::get().getRNGSystem().getSeededRNG(*seed);
				}
			}
			else
			{
				generationRNG = GameBase::get().getRNGSystem().getTimeInfluencedRNG();
			}

			assert(generationRNG);

			////////////////////////////////////////////////////////
			// set up planets
			////////////////////////////////////////////////////////
			planets.clear();
			for (const PlanetData& planet : levelConfig->getPlanets())
			{
				Planet::Data init = {};
				copyPlanetDataToInitData(planet, init);

				planets.push_back(new_sp<Planet>(init));
			}

			////////////////////////////////////////////////////////
			// set up stars
			////////////////////////////////////////////////////////
			localStars.clear();

			for (const StarData& star : levelConfig->getStars())
			{
				glm::vec3 starDir = normalize(star.offsetDir.has_value() ? *star.offsetDir : makeRandomVec3());
				float starDist = star.offsetDistance.has_value() ? *star.offsetDistance : generationRNG->getFloat(10.0f, 40.f);
				vec3 starColor = star.color.has_value() ? *star.color : vec3(1.f);	//perhaps randomize color

				localStars.push_back(new_sp<Star>());
				const sp<Star>& newStar = localStars.back();
				newStar->setLightLDR(starColor);
				newStar->updateXformForData(starDir, starDist);
			}
			refreshStarLightMapping(); // now that stars are set up, recalculate the levels directional lights.

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// set up nebulae
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			nebulae.clear();
			for (const NebulaData& nebulaData : levelConfig->getNebulae())
			{
				Nebula::Data init;
				init.textureRelativePath = nebulaData.texturePath; //todo-- functionify this? not needed for now but want to avoid code duplication
				init.tintColor = nebulaData.tintColor;
				nebulae.push_back(new_sp<Nebula>(init));
				nebulae.back()->setXform(nebulaData.transform);
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// set up avoidance meshes
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			avoidMeshes.clear();
			if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
			{
				const std::map<std::string, sp<SpawnConfig>>& spawnConfigs = activeMod->getSpawnConfigs();
				for(const WorldAvoidanceMeshData& avoidMeshData : levelConfig->getAvoidanceMeshes())
				{
					if (auto findIter = spawnConfigs.find(avoidMeshData.spawnConfigName); findIter != spawnConfigs.end() && findIter->second)
					{
						AvoidMesh::SpawnData asteroidSpawnParams;
						asteroidSpawnParams.spawnTransform = avoidMeshData.spawnTransform;
						asteroidSpawnParams.spawnConfig = findIter->second;
						avoidMeshes.push_back(spawnEntity<AvoidMesh>(asteroidSpawnParams));
					}
				}
			}
		}
	}

	void SpaceLevelBase::transitionToMainMenu_s()
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// detach all players from ships as we're about to do a level transition
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//don't cause bug in main menu letting player fly a ship
		PlayerSystem& playerSystem = SpaceArcade::get().getPlayerSystem();
		for (size_t playerIdx = 0; playerIdx < playerSystem.numPlayers(); ++playerIdx)
		{
			if (const sp<PlayerBase>& player = playerSystem.getPlayer(playerIdx))
			{
				//clear out any ships the player may be piloting
				player->setControlTarget(nullptr);
			}
		}

		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		sp<LevelBase> mainMenuLevel = new_sp<MainMenuLevel>(); //this requires include of entire main menu level, which feels weird (this is mainmenus base). but I suppose it isn't that weird
		levelSystem.loadLevel(mainMenuLevel);
	}

	void SpaceLevelBase::startLevel_v()
	{
		LevelBase::startLevel_v();

		generationRNG = GameBase::get().getRNGSystem().getTimeInfluencedRNG(); //create a default

		forwardShadedModelShader = new_sp<SA::Shader>(spaceModelShader_forward_vs, spaceModelShader_forward_fs, false);
		highlightForwardModelShader = new_sp<SA::Shader>(modelVertexOffsetShader_vs, fwdModelHighlightShader_fs, false);
		debugNormalMapShader = new_sp<SA::Shader>(normalDebugShader_LineEmitter_vs, normalDebugShader_LineEmitter_fs, normalDebugShader_LineEmitter_gs, false);


		//we don't know how many teams there will be after we load gamemode (in apply config), so make all teamcommands now
		//#todo change this. this isn't great because we need team commanders to be around before spawning so they can be aware of when we spawn ships
		//otherwise we won't have the ability to ask a commander for a target. currently just building maximum number of commanders
		for (size_t teamId = 0; teamId < MAX_TEAM_NUM; ++teamId)
		{
			commanders.push_back(new_sp<TeamCommander>(teamId));
		}

		applyLevelConfig();

		enableStarJump(true, true); //skip to being in middle of star jump vfx

		static bool bFirstLevelStarted = false; //don't show VFX on first level (ie main menu)
		enableStarJump(false, !bFirstLevelStarted); //animate in stars vfx
		bFirstLevelStarted = true;

		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			const ModelGlobals& modelGlobals = activeMod->getModelGlobals();
			forwardShadedModelShader->use();
			forwardShadedModelShader->setUniform1i("bUseNormalMapping", modelGlobals.bUseNormalMap);
			forwardShadedModelShader->setUniform1i("bUseNormalSeamCorrection", modelGlobals.bUseNormalMapXSeamCorrection);
			forwardShadedModelShader->setUniform1i("bUseMirrorUvNormalCorrection", modelGlobals.bUseNormalMapTBNFlip);
			forwardShadedModelShader->use(false);
		}
	}

	void SpaceLevelBase::endLevel_v()
	{
		//this means that we're going to generate a new shader between each level transition.
		//this can be avoided with static members or by some other mechanism, but I do not see 
		//transitioning levels being a slow process currently, so each level gets its own shaders.
		forwardShadedModelShader = nullptr;
		highlightForwardModelShader = nullptr;

		LevelBase::endLevel_v();
	}

	void SpaceLevelBase::postConstruct()
	{
		createTypedGrid<AvoidanceSphere>(glm::vec3(128));

		stencilHighlightEntities.reserve(6);
		
		starField = onCreateStarField();
		{ 
			bGeneratingLocalStars = true;
			onCreateLocalStars();
			bGeneratingLocalStars = false;

			onCreateLocalPlanets();
		}

		refreshStarLightMapping();

		//fix bug where hud would not be visible after leaving main menu.
		if (const sp<HUD> hud = !isMenuLevel() ? SpaceArcade::get().getHUD() : nullptr)
		{
			log(__FUNCTION__, LogLevel::LOG, "Enabling Hud Visibility");
			hud->setVisibility(true);
		}
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
		else if (isTestLevel() && !isMenuLevel())
		{
			if (spaceGameMode = createGamemodeFromTag(TAG_GAMEMODE_CARRIER_TAKEDOWN)) //default to carrier take down
			{
				spaceGameMode->setOwningLevel(sp_this());
				spaceGameMode->initialize(ServerGameMode_SpaceBase::LevelKey{});
			}
		}
		return spaceGameMode;
	}

	void SpaceLevelBase::onEntitySpawned_v(const sp<WorldEntity>& spawned)
	{
		if (spawned)
		{
			spawned->onDestroyedEvent->addWeakObj(sp_this(), &SpaceLevelBase::handleEntityDestroyed);
		}
	}

	void SpaceLevelBase::handleEntityDestroyed(const sp<GameEntity>& entity)
	{
		if (sp<RenderModelEntity> renderEntity = std::dynamic_pointer_cast<RenderModelEntity>(entity))
		{
			unspawnEntity<RenderModelEntity>(renderEntity);
		}
	}

	sp<SA::StarField> SpaceLevelBase::onCreateStarField()
	{
		//by default generate a star field, if no star field should be in the level return null in an override;
		StarField::InitData initStarField;
		if (levelConfig)
		{
			initStarField.colorScheme = levelConfig->getDistantStarColorScheme();;
		}
		sp<StarField> defaultStarfield = new_sp<StarField>(initStarField);
		return defaultStarfield; 
	}

	void SpaceLevelBase::refreshStarLightMapping()
	{
		dirLights.clear(); //get rid of extra lights

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
				//dirLights[starIdx].lightIntensity = star->getLightLDR(); //LDR instead of HDR because HDR color saturate image
				dirLights[starIdx].lightIntensity = GameBase::get().getRenderSystem().isUsingHDR() ? star->getLightHDR(): star->getLightLDR(); 
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

	void SpaceLevelBase::debug_correctNormalMapSeamsOverride(std::optional<bool> correctNormalMapSeams)
	{
		this->correctNormalMapSeamsOverride = correctNormalMapSeams;
	}

	void SpaceLevelBase::debug_useNormalMappingOverride(std::optional<bool> useNormalMapping)
	{
		this->useNormalMappingOverride = useNormalMapping;
	}

	void SpaceLevelBase::debug_useNormalMappingMirrorCorrection(std::optional<bool> useNormalMappingMirrorCorrection)
	{
		this->useNormalMappingMirrorCorrectionOverride = useNormalMappingMirrorCorrection;
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

