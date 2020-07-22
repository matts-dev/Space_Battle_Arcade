#include "ServerGameMode_CarrierTakedown.h"
#include <string>
#include "../Levels/LevelConfigs/SpaceLevelConfig.h"
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "../GameSystems/SAModSystem.h"
#include "../Levels/SASpaceLevelBase.h"
#include "../SpaceArcade.h"
#include "../../Tools/PlatformUtils.h"
#include "../../GameFramework/SALog.h"
#include "../SAShip.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../Components/FighterSpawnComponent.h"
#include "../../Tools/DataStructures/MultiDelegate.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../../GameFramework/SABehaviorTree.h"
#include "../../GameFramework/Components/GameplayComponents.h"
#include "../AI/SAShipBehaviorTreeNodes.h"
#include <memory>

namespace SA
{
	using namespace glm; 

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// implementation helpers
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
		glm::quat centerRotation = glm::angleAxis(2 * glm::pi<float>()*percOfCircle, worldUp);

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

	

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Carrier Takedown
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void ServerGameMode_CarrierTakedown::onInitialize(const sp<SpaceLevelBase>& level)
	{
		ServerGameMode_SpaceBase::onInitialize(level);

		myRNG = GameBase::get().getRNGSystem().getSeededRNG(11);

		if (level)
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// set up heart beats
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (const sp<TimeManager>& worldTimeManager = level->getWorldTimeManager())
			{
				targetPlayerHeartBeatDelegate = new_sp<MultiDelegate<>>();
				targetPlayerHeartBeatDelegate->addWeakObj(sp_this(), &ServerGameMode_CarrierTakedown::targetPlayerHeartBeat);
				targetPlayerHeartbeatConfig.heartBeatSec = bTargetHeartbeatEveryFrame ? 0.01f : targetPlayerHeartbeatConfig.heartBeatSec; //update value so that calculations in heartbeat are correct
				worldTimeManager->createTimer(targetPlayerHeartBeatDelegate, targetPlayerHeartbeatConfig.heartBeatSec, true);
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// spawn ships for teams
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			const sp<const SpaceLevelConfig>& levelConfig = level->getConfig();
			const sp<RNG>& rngPtr = level->getGenerationRNG();

			if(levelConfig && rngPtr)
			{
				RNG& rng = *rngPtr;

				const SpaceLevelConfig::GameModeData_CarrierTakedown& gmData = levelConfig->getGamemodeData_CarrierTakedown();
				if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
				{
					numTeams = gmData.teams.size();
					for (size_t teamIdx = 0; teamIdx < gmData.teams.size(); ++teamIdx)
					{
						const SpaceLevelConfig::GameModeData_CarrierTakedown::TeamData& teamData = gmData.teams[teamIdx];
						myTeamData.push_back(TeamData{});
						TeamData& team = myTeamData.back();
						team.teamIdx = teamIdx;

						////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						//spawn carrier ships
						////////////////////////////////////////////////////////////////////////////////////////////////////////////////
						for (size_t carrierIdx = 0; carrierIdx < teamData.carrierSpawnData.size(); ++carrierIdx)
						{
							CarrierSpawnData carrierData = teamData.carrierSpawnData[carrierIdx];
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

								if (sp<Ship> carrierShip = level->spawnEntity<Ship>(carrierShipSpawnData))
								{
									team.carriers.push_back(carrierShip);
									carrierShip->setSpeedFactor(0); //make carrier stationary

									FighterSpawnComponent::AutoRespawnConfiguration spawnFighterConfig{};
									spawnFighterConfig.bEnabled = carrierData.fighterSpawnData.bEnableFighterRespawns;
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
			else
			{
				if (!level->isTestLevel()) //test level will not have config, but I need to set it up with a gamemode
				{
					STOP_DEBUGGER_HERE(); //didn't get a level config or generation RNG. 
				}
			}

			processLoadedGameData();
		}
	}

	void ServerGameMode_CarrierTakedown::processLoadedGameData()
	{
		for (size_t teamIdx = 0; teamIdx < myTeamData.size(); ++teamIdx)
		{
			TeamData currentTeam = myTeamData[teamIdx];
			for (const wp<Ship>& wp_carrier : currentTeam.carriers)
			{
				if (const sp<Ship> carrier = wp_carrier.lock())
				{
					//need to be careful here, we're checking if destroyed immediately on destroy event -- should work under current architecture but perhaps less fragile if we deferred a frame
					carrier->onDestroyedEvent->addWeakObj(sp_this(), &ServerGameMode_CarrierTakedown::handleCarrierDestroyed);
				} else {STOP_DEBUGGER_HERE(); }
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// spawn player at carrier of their preferred team
		/////////////////////////////////////////////////////////////////////////////////////
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
		if (activeMod && myTeamData.size() > 0)
		{
			size_t playerTeamIdx = activeMod->getPlayerPreferredTeam();
			playerTeamIdx = clamp<size_t>(playerTeamIdx, 0, myTeamData.size());

			TeamData& team = myTeamData[playerTeamIdx];
			if (sp<Ship> carrierShip = (team.carriers.size() > 0 && !team.carriers[0].expired()) ? team.carriers[0].lock() : nullptr)
			{
				if (FighterSpawnComponent* spawnComp = carrierShip->getGameComponent<FighterSpawnComponent>())
				{
					PlayerSystem& playerSystem = SpaceArcade::get().getPlayerSystem();
					for (size_t playerIdx = 0; playerIdx < playerSystem.numPlayers(); ++playerIdx)
					{
						if (const sp<PlayerBase>& player = playerSystem.getPlayer(playerIdx))
						{
							sp<Ship> playersShip = spawnComp->spawnEntity();
							player->setControlTarget(playersShip);
							if (const sp<CameraBase>& camera = player->getCamera())
							{
								camera->setCursorMode(false); //ensure that player's camera gets out of cursor mode from main menu
							}
						}
					}
				}
			}
		}
	}

	void ServerGameMode_CarrierTakedown::handleCarrierDestroyed(const sp<GameEntity>&)
	{
		EndGameParameters p;
		size_t numTeamsWithCarriers = 0;

		for (size_t teamIdx = 0; teamIdx < myTeamData.size(); ++teamIdx)
		{
			TeamData currentTeam = myTeamData[teamIdx];

			size_t aliveCarriersForThisTeam = 0;
			for (const wp<Ship>& wp_carrier : currentTeam.carriers)
			{
				const sp<Ship> carrier = wp_carrier.lock();
				aliveCarriersForThisTeam += (carrier && !carrier->isPendingDestroy()) ? 1 : 0;
			}

			if (aliveCarriersForThisTeam > 0)
			{
				numTeamsWithCarriers += 1;
				p.winningTeams.push_back(teamIdx);
			}
			else
			{
				p.losingTeams.push_back(teamIdx);
			}
		}

		if (numTeamsWithCarriers <= 1)
		{
			endGame(p);
		}
	}

	void ServerGameMode_CarrierTakedown::targetPlayerHeartBeat()
	{
		std::string BT_TargetKey = "target"; //#TODO need global lookup for these strings
		std::string BT_AttackerKey = "activeAttackersKey";


		targetPlayerHeartbeatData.accumulatedTime += targetPlayerHeartbeatConfig.heartBeatSec;

		if (bool bWaitedLongEnough = targetPlayerHeartbeatData.accumulatedTime >= targetPlayerHeartbeatData.waitSec)
		{
			const std::vector<sp<PlayerBase>>& allPlayers = GameBase::get().getPlayerSystem().getAllPlayers();

			ServerGameMode_SpaceBase::playersNeedingTarget.clear();

			for (const sp<PlayerBase> player : allPlayers)
			{
				if (player->hasControlTarget())
				{
					sp<IControllable> controlTargetBase = player->getControlTargetSP();

					if(WorldEntity* playerControlWE = controlTargetBase->asWorldEntity())
					{
						if (BrainComponent* brain = playerControlWE->getGameComponent<BrainComponent>())
						{
							if (const BehaviorTree::Tree* tree = brain->getTree())
							{
								if (tree->getMemory().hasValue(BT_AttackerKey))
								{
									if (const BehaviorTree::ActiveAttackers* activeAttackers = tree->getMemory().getReadValueAs<BehaviorTree::ActiveAttackers>(BT_AttackerKey))
									{
										//BehaviorTree::cleanActiveAttackers(*activeAttackers);

										if (activeAttackers->size() == 0)
										{
											ServerGameMode_SpaceBase::playersNeedingTarget.push_back(player);
										}
									}
								}
							}
						}
					}
				}
			}

			targetPlayerHeartbeatData.waitSec = myRNG->getFloat<float>(targetPlayerHeartbeatConfig.minWaitSec, targetPlayerHeartbeatConfig.maxWaitSecc);
			targetPlayerHeartbeatData.accumulatedTime = 0.f;
		}
	}

	void ServerGameMode_CarrierTakedown::configureForTestLevel()
	{
		if (sp<SpaceLevelBase> level = getOwningLevel().lock())
		{
			myTeamData.clear();

			//loop through all ships and find the carriers, then set those; this is going to be slow
			const std::set<sp<WorldEntity>>& worldEntities = level->getWorldEntities();
			for (const sp<WorldEntity>& worldEntity : worldEntities)
			{
				sp<Ship> asShip = std::dynamic_pointer_cast<Ship>(worldEntity);
				if (bool bIsCarrier = asShip->getGameComponent<FighterSpawnComponent>() != nullptr)
				{
					size_t team = asShip->getTeam();
					if (myTeamData.size() < team+1)
					{
						myTeamData.resize(team+1);
					}
					myTeamData[team].carriers.push_back(asShip);
				}
			}
		}
	}

}

