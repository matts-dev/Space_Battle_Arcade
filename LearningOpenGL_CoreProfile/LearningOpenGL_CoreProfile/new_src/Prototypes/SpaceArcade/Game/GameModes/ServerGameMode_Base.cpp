#include "ServerGameMode_Base.h"
#include "../Levels/SASpaceLevelBase.h"
#include "../../Tools/PlatformUtils.h"
#include "../SAShip.h"
#include "../../Tools/SAUtilities.h"
#include "../GameSystems/SAModSystem.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../Tools/Algorithms/AmortizeLoopTool.h"
#include "../../GameFramework/SABehaviorTree.h"
#include "../AI/GlobalSpaceArcadeBehaviorTreeKeys.h"
#include "../SAPlayer.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../AssetConfigs/DifficultyConfig.h"
#include "../SpaceArcade.h"
#include "../../GameFramework/SADebugRenderSystem.h"

namespace SA
{
	using namespace glm;


	/*static*/ std::vector<fwp<PlayerBase>> ServerGameMode_Base::playersNeedingTarget;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// DEBUG_HITCH_NEVER_CLEAR_PLACEMENTS 
	//
	// this will cause issues long term play because same placement can get added multiple times, 
	// but useful for testing hitching at start of match
	// the main purpose of this is to see what worst case of turrets/healers not finding targets each frame is like
	constexpr bool DEBUG_HITCH_NEVER_CLEAR_PLACEMENTS = false; 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// AMORTIZE_ITEMS_IN_SHIP_WALK
	//
	// while all ships will be walked in a frame, the items tested against will be amortized
	constexpr bool AMORTIZE_OBJECTIVE_TARGETING_IN_SHIP_WALK= true;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Debug gamemode hits
#define DEBUG_GAMEMODE_HITS 0
#if DEBUG_GAMEMODE_HITS
	static std::optional<glm::vec3> lastGMHitLoc;
#endif
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////



	//these are made mutable so that we can tweak these in real time for designing.
	//these are stored in a single place for easy tweaking/analysis
	static struct GameModeBaseConstants
	{
		//values intended to be tweaked
		float hardestValue_playerPressureMultiplier = 1; //+1		

		//values not likely to need to be tweaked
		float playerTeamRefreshDelaySec = 15.f;
	} gmConstants;


	void ServerGameMode_Base::setOwningLevel(const sp<SpaceLevelBase>& level)
	{
		owningLevel = level;
	}


	void ServerGameMode_Base::addTurretNeedingTarget(const sp<ShipPlacementEntity>& turret)
	{
		turretsNeedingTarget.push_back(turret);
	}

	void ServerGameMode_Base::addHealerNeedingTarget(const sp<ShipPlacementEntity>& healer)
	{
		healersNeedingTarget.push_back(healer);
	}

	void ServerGameMode_Base::initialize(const LevelKey& key)
	{
		sp<SpaceLevelBase> level = owningLevel.expired() ? nullptr : owningLevel.lock();
		onInitialize(level);

		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			difficultyConfig = activeMod->getDifficulty();
		}
		assert(difficultyConfig);


		initialize_LogDebugWarnings();
	}

	void ServerGameMode_Base::onInitialize(const sp<SpaceLevelBase>& level)
	{
		bBaseInitialized = true;

		if (level)
		{
			level->onSpawnedEntity.addWeakObj(sp_this(), &ServerGameMode_Base::handleEntitySpawned);

			for (const sp<WorldEntity>& worldEntity : level->getWorldEntities())
			{
				//stack up the target data
				handleEntitySpawned(worldEntity);
			}
		}
	}

	void ServerGameMode_Base::tick(float dt_sec, const LevelKey& key)
	{
		accumulatedTimeSec += dt_sec;

		tick_carrierObjectiveBalancing(dt_sec);
		tick_amortizedObjectiveHitUpdate(dt_sec); // silently destroy objectives if player isn't looking - hijacking player princple
		tick_singleShipWalk(dt_sec);
		tick_debug(dt_sec);;
	}

	void ServerGameMode_Base::endGame(const EndGameParameters& endParameters)
	{
		//game mode acts on level because if client/server architecture is ever set up, clients will have access to level but not gamemode
		if (sp<SpaceLevelBase> level = owningLevel.lock())
		{
			level->endGame(endParameters);
		}
		else
		{
			STOP_DEBUGGER_HERE(); //no level, how did this happen?
		}
	}

	void ServerGameMode_Base::handleEntitySpawned(const sp<WorldEntity>& spawned)
	{
		if (sp<Ship> spawnedShip = std::dynamic_pointer_cast<Ship>(spawned))
		{
			ships.push_back(spawnedShip);

			if (spawnedShip->isCarrierShip())
			{
				size_t team = spawnedShip->getTeam();
				if (!Utils::isValidIndex(teamData, team))
				{
					while (teamData.size() < (team+1) && teamData.size() < MAX_TEAM_NUM)
					{
						teamData.emplace_back();
						//player may not yet be assigned to a team, so avoiding assigning here.
					}
				}

				if (Utils::isValidIndex(teamData, team))
				{
					teamData[team].carriers.push_back(spawnedShip);
				}
			}
		}

		//let subclass gamemodes know 
		onWorldEntitySpawned(spawned);
	}

	void ServerGameMode_Base::tick_carrierObjectiveBalancing(float dt_sec)
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// notes about this function
		//
		// This may seem like it should belong in the carrier takedown subclass, but we actually want this behavior for
		// for any gamemode that uses carriers and needs fighters to destroys objectives at a similar rate as the player.
		//
		// Perhaps it isn't appropriate for the base-most gamemode
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//find losing team and apply more pressure to winning teams to equalize out the battle.
		//goal is to have every fight feel like it is hard and not easy, regardless of player skill.
		//using percecntage because some levels may have have a 2 vs 1 set up; in that case numbers will not match up; but percentages will.
		float smallestAlivePerc_Carriers = 1.f;			// [0,1]
		float smallestAlivePerc_Objectives = 1.f;		// [0,1]
		size_t spawnedObjectivesPerCarrier = 30;		// not great to guess 30, but will only use 30 when all carriers are destroyed
		size_t smallestObjectivesAliveOfAllTeams = std::numeric_limits<size_t>::max();

		//players can switch teams, so we need to refresh that at some regualr interval; this requries dynamic cast which is why it is spaced out.
		bool bRefreshPlayerTeams = accumulatedTimeSec > (timestamp_lastPlayerTeamRefresh + gmConstants.playerTeamRefreshDelaySec);
		if (bRefreshPlayerTeams)
		{
			timestamp_lastPlayerTeamRefresh = accumulatedTimeSec;
			for (GameModeTeamData team : teamData)
			{
				team.bIsPlayerTeam = false;
			}
			const std::vector<sp<PlayerBase>>& allPlayers = GameBase::get().getPlayerSystem().getAllPlayers();
			for (const sp<PlayerBase>& playerPtr : allPlayers)
			{
				if (SAPlayer* player = dynamic_cast<SAPlayer*>(playerPtr.get())) //#TODO should player have a team component?
				{
					size_t playerTeam = player->getCurrentTeamIdx();
					if (Utils::isValidIndex(teamData, playerTeam))
					{
						teamData[playerTeam].bIsPlayerTeam = true;
					}
				}
			}
		}

		////////////////////////////////////////////////////////
		// find the weakest team data
		////////////////////////////////////////////////////////
		for (GameModeTeamData& team : teamData)
		{
			size_t numCarriersDestroyed = 0;
			size_t numObjectivesAlive = 0;
			for (const lp<Ship>& carrier : team.carriers)
			{
				if (!carrier)
				{
					++numCarriersDestroyed;
				}
				else
				{
					numObjectivesAlive += carrier->activeObjectives();
					spawnedObjectivesPerCarrier = carrier->numObjectivesAtSpawn(); 
				}
			}

			team.activeHitsAgainstTeam = 0;

			team.numCarriersAlive = team.carriers.size() - numCarriersDestroyed;
			team.numObjectivesAlive = numObjectivesAlive;
			team.percentAlive_Carriers = 1 - (float(numCarriersDestroyed) / team.carriers.size());
			team.numObjectivesAtStart = spawnedObjectivesPerCarrier * team.carriers.size();
			team.percentAlive_Objectives = float(numObjectivesAlive) / team.numObjectivesAtStart;

			//track the worst-off team and balance other teams to match that team
			smallestAlivePerc_Carriers = std::min(team.percentAlive_Carriers, smallestAlivePerc_Carriers);
			smallestAlivePerc_Objectives = std::min(smallestAlivePerc_Objectives, team.percentAlive_Objectives);

		}

		//recalculate how many hits against this team there are
		for (UnfilledObjectiveHit unfilledHit : unfilledObjectiveHits)
		{
			if (Utils::isValidIndex(teamData, unfilledHit.objectiveTeam))
			{
				teamData[unfilledHit.objectiveTeam].activeHitsAgainstTeam += 1;
			}
		}
		for (FilledObjectiveHit filledHit : filledObjectiveHits)
		{
			if (Utils::isValidIndex(teamData, filledHit.objectiveTeam))
			{
				teamData[filledHit.objectiveTeam].activeHitsAgainstTeam += 1;
			}
		}


		/////////////////////////////////////////////////////////////////////////////////////
		// use weakest team data to determine how much pressure to apply to other teams
		/////////////////////////////////////////////////////////////////////////////////////
		size_t spreadCarrierIdx = 0; //basically used just so we don't always pick objectives to hit from same carrier
		for (GameModeTeamData& team : teamData)
		{
			float smallestAlivePerc_Objectives_playerAdjusted = smallestAlivePerc_Objectives;

			bool bIsPlayerTeamWeakestTeam = false;
			if (team.bIsPlayerTeam)
			{
				bIsPlayerTeamWeakestTeam = (smallestAlivePerc_Objectives == team.percentAlive_Objectives);
				if (!bIsPlayerTeamWeakestTeam) //feel free to adjust this with testing -- very adhoc
				{
					float difficulty = difficultyConfig->GameModeObjectiveBalancing(); //[0,1] where 1 is hardest
				 
					//for 0, we leave it unchanged. for 1 we double the pressure.
					float& hardestValue = gmConstants.hardestValue_playerPressureMultiplier;
					float scaleDown = 1.f / (1 + difficulty * hardestValue); //think of this has being 1/2 in worse case
					smallestAlivePerc_Objectives_playerAdjusted = smallestAlivePerc_Objectives * scaleDown;

					//#TODO add variability in difficulty to make game feel more dynamic?
				}
			}

			constexpr size_t MIN_NUM_CONCURRENT_HITS = 1; //there should always be at least this many hits ongoing at a given time so the game progresses
			bool bBelowBaselineHits = (team.activeHitsAgainstTeam) < MIN_NUM_CONCURRENT_HITS;
			
			if (team.percentAlive_Objectives > smallestAlivePerc_Objectives_playerAdjusted || bBelowBaselineHits)
			{
				team.goal_numObjectives = size_t(smallestAlivePerc_Objectives_playerAdjusted * float(team.numObjectivesAtStart)); //calculate this after we've did a pass over all teams to figure out smallest %
				 
				bool bTeamHasObjectives = team.numObjectivesAlive > 0 && team.numCarriersAlive > 0;

				if (bTeamHasObjectives &&
					(team.activeHitsAgainstTeam < MIN_NUM_CONCURRENT_HITS || (team.numObjectivesAlive - team.activeHitsAgainstTeam) > team.goal_numObjectives))
				{
					//if next carrier index isn't valid, start over.
					if (!Utils::isValidIndex(team.carriers, spreadCarrierIdx)){spreadCarrierIdx = 0;}

					std::optional<size_t> carrierIdx = Utils::FindValidIndexLoopingFromIndex<lp<Ship>>(spreadCarrierIdx, team.carriers, [](const lp<Ship>& carrier) {return carrier.get() != nullptr; });
					if (carrierIdx.has_value())
					{
						const lp<Ship> carrierShip = team.carriers[*carrierIdx];
						sp<ShipPlacementEntity> randomObjective = carrierShip->getRandomObjective();
						unfilledObjectiveHits.emplace_back(randomObjective, carrierShip->getTeam());
					}
				}
			}
		}
	}
		
	//static void setShipTarget(const sp<Ship>& ship, const lp<WorldEntity>& target)
	//{
	//	if (BrainComponent* brainComp = ship ? ship->getGameComponent<BrainComponent>() : nullptr)
	//	{
	//		////////////////////////////////////////////////////////
	//		// target objective
	//		////////////////////////////////////////////////////////
	//		if (const BehaviorTree::Tree* behaviorTree = brainComp->getTree())
	//		{
	//			using namespace BehaviorTree;
	//			Memory& memory = behaviorTree->getMemory();
	//			{
	//				sp<WorldEntity> targetSP = target;
	//				memory.replaceValue(BT_TargetKey, targetSP);
	//			}
	//		}
	//	}
	//}

	bool isShipTargetingObjective(const lp<Ship>& ship)
	{
		//this might be a bad perf hit since it involves a dynamic cast. 
		//adding flag to disable check because it isn't so bad (nor probable) that we'll assign a ship to two different objectives
		constexpr bool ENABLE_SHIP_OBJECTIVE_TARGET_CHECK = true; 
		if (BrainComponent* brainComp = ship && ENABLE_SHIP_OBJECTIVE_TARGET_CHECK ? ship->getGameComponent<BrainComponent>() : nullptr)
		{
			////////////////////////////////////////////////////////
			// target objective
			////////////////////////////////////////////////////////
			if (const BehaviorTree::Tree* behaviorTree = brainComp->getTree())
			{
				using namespace BehaviorTree;
				Memory& memory = behaviorTree->getMemory();
				{
					if (memory.getReadValueAs<ShipPlacementEntity>(BT_TargetKey) != nullptr)
					{
						return true;
					}
				}
			}
		}
		return false;
	}


	void ServerGameMode_Base::tick_singleShipWalk(float dt_sec)
	{

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Notes about this function:
		//
		// It is essentially a bandwagon of things to do per frame. Each frame we should ideally only walk over the ships once
		// we can do all work needed per ship in this walk. Truth be told, we're ticking and rendering ships separately so there
		// is more than a single ship walk. But those other walks are very system specific. This is a generic server-side walk
		// for all other requirements.
		//
		// Ideally, this would be structured as a delegate that is broadcast that things can hook into. eg
		// 
		// preShipWalkDelegate.broadcast();		//for filtering data so it doesn't need to happen each ship walk
		// for ship
		//		onWalkedShip.broadcast(ship);
		// 
		// to make this highly portable. But delegates are a bit slower with my implementation (walking a map)
		// so going with this because 1. its simple, 2. its fast.
		// most of this is speculative as I have not profiled a comparisions.
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		/////////////////////////////////////////////////////////////////////////////////////
		// pre filterings that would be expensive to do for each ship
		/////////////////////////////////////////////////////////////////////////////////////

		//preShipWalkDelegate.broadcast();

		//o(n) : clean ships in the event any have been destroyed.	
		auto newEndIterShips = std::remove_if(ships.begin(), ships.end(), [](const lp<Ship>& ship) {return ship.get() == nullptr; });
		ships.erase(newEndIterShips, ships.end());

		auto newEndIterTurrets = std::remove_if(turretsNeedingTarget.begin(), turretsNeedingTarget.end(), [](const lp<ShipPlacementEntity>& turret) {return !turret || turret->isPendingDestroy() || turret->hasTarget(); });
		turretsNeedingTarget.erase(newEndIterTurrets, turretsNeedingTarget.end());

		auto newEndIterHealers = std::remove_if(healersNeedingTarget.begin(), healersNeedingTarget.end(), [](const lp<ShipPlacementEntity>& healer) {return !healer || healer->isPendingDestroy() || healer->hasTarget(); });
		healersNeedingTarget.erase(newEndIterHealers, healersNeedingTarget.end());


		/////////////////////////////////////////////////////////////////////////////////////
		// prepare performance amortization
		/////////////////////////////////////////////////////////////////////////////////////
		if (AMORTIZE_OBJECTIVE_TARGETING_IN_SHIP_WALK)
		{
			amort_turrets.updateStart(turretsNeedingTarget);
			amort_healers.updateStart(healersNeedingTarget);
		}
		amort_fillUnfilledHits.updateStart(unfilledObjectiveHits);

		////////////////////////////////////////////////////////
		// O(~n) : walk over ships
		////////////////////////////////////////////////////////
		for (const lp<Ship>& ship : ships)
		{
			///////////////////////////////////////////////////////////////////////////////////////////
			//no ships are null because we just did a clean of nullships before getting here
			//this is a perf optimization allowing us to avoid a branch checking ship validity
			///////////////////////////////////////////////////////////////////////////////////////////

			//onWalkedShip.broadcast(ship); //probably should do this last as results could kill a ship and null it out?

			bool bIsCarrier = ship->getFighterComp() != nullptr;
			vec3 shipPosition = ship->getWorldPosition();
			size_t shipTeam = ship->getTeam();

			////////////////////////////////////////////////////////
			// handle target player heartbeat
			////////////////////////////////////////////////////////
			if (ServerGameMode_Base::playersNeedingTarget.size() > 0 && !bIsCarrier)
			{
				ship->TryTargetPlayer();
			}

			////////////////////////////////////////////////////////
			// find attacking placements a target
			// o(n*ap)
			////////////////////////////////////////////////////////
			if (turretsNeedingTarget.size() > 0 && !bIsCarrier)
			{
				std::optional<size_t> removeIdx;
				for (size_t attackPlacementIdx = amort_turrets.getStartIdx();
						attackPlacementIdx < turretsNeedingTarget.size() 
						&& !removeIdx.has_value()
						&& (AMORTIZE_OBJECTIVE_TARGETING_IN_SHIP_WALK && attackPlacementIdx < amort_turrets.getStopIdxNonsafe());
					++attackPlacementIdx)
				{
					//turret is guaranteed to be valid due to prefiltering at top of this function
					lp<ShipPlacementEntity>& turret = turretsNeedingTarget[attackPlacementIdx];
					if (turret->getTeamData().team != shipTeam
						&& glm::distance2(turret->getWorldPosition(), shipPosition) < turret->getMaxTargetDistance2())
					{
						sp<Ship> shipAsSP = ship;
						turret->setTarget(shipAsSP);
						removeIdx = attackPlacementIdx;
					}
				}
				if (removeIdx.has_value() && !DEBUG_HITCH_NEVER_CLEAR_PLACEMENTS)
				{
					Utils::swapAndPopback(turretsNeedingTarget, *removeIdx);
				}
			}

			////////////////////////////////////////////////////////
			// find defending placements a target
			// o(n*dp)
			////////////////////////////////////////////////////////
			if (healersNeedingTarget.size() > 0 && !bIsCarrier)
			{
				std::optional<size_t> removeIdx;
				for (size_t defPlacementIdx = amort_healers.getStartIdx(); 
						defPlacementIdx < healersNeedingTarget.size()
						&& !removeIdx.has_value()
						&& (AMORTIZE_OBJECTIVE_TARGETING_IN_SHIP_WALK && defPlacementIdx < amort_healers.getStopIdxNonsafe());
					++defPlacementIdx)
				{
					//healer is guaranteed to be valid due to prefiltering at top of this function
					lp<ShipPlacementEntity>& healer = healersNeedingTarget[defPlacementIdx];

					if (healer->getTeamData().team == shipTeam
						&& glm::distance2(healer->getWorldPosition(), shipPosition) < healer->getMaxTargetDistance2())
					{
						sp<Ship> shipAsSP = ship;
						healer->setTarget(shipAsSP);
						removeIdx = defPlacementIdx;
					}
				}
				if (removeIdx.has_value() && !DEBUG_HITCH_NEVER_CLEAR_PLACEMENTS)
				{
					Utils::swapAndPopback(healersNeedingTarget, *removeIdx);
				}
			}

			////////////////////////////////////////////////////////
			// objective hits
			////////////////////////////////////////////////////////
			if (unfilledObjectiveHits.size() > 0 && !bIsCarrier)
			{
				for (size_t objectiveHitIdx = amort_fillUnfilledHits.getStartIdx();
					objectiveHitIdx < amort_fillUnfilledHits.getStopIdxSafe(unfilledObjectiveHits); 
					++objectiveHitIdx)
				{
					std::optional<size_t> removeIdx;
					bool bValidShipForAssignment = false;

					const UnfilledObjectiveHit& pendingHit = unfilledObjectiveHits[objectiveHitIdx];

					if (bool bAlreadyDestroyed = (pendingHit.objective.get() == nullptr)) //TODO refactor so we don't have to do .get()
					{
						removeIdx = objectiveHitIdx;
					}
					else if (ship->getTeam() != pendingHit.objectiveTeam && pendingHit.objective && !isShipTargetingObjective(ship))
					{
						bValidShipForAssignment = true;
						ShipUtilLibrary::setShipTarget(ship, pendingHit.objective);
						removeIdx = objectiveHitIdx;
					}

					if (removeIdx.has_value())
					{
						if (bValidShipForAssignment && pendingHit.objective ) //only move to filled if object is still in play, otherwise
						{
							filledObjectiveHits.emplace_back(ship, pendingHit);
						}

						Utils::swapAndPopback(unfilledObjectiveHits, *removeIdx);
					}
				}
			}
		}
	}


	void ServerGameMode_Base::tick_debug(float dt_sec)
	{
		static DebugRenderSystem& db = GameBase::get().getDebugRenderSystem();

#if DEBUG_GAMEMODE_HITS
		if (lastGMHitLoc.has_value())
		{
			PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
			if (const sp<PlayerBase>& player = playerSystem.getPlayer(0))
			{
				if (IControllable* controlTarget = player->getControlTarget())
				{
					WorldEntity* playerShip = controlTarget->asWorldEntity();
					glm::vec3 playerPos = playerShip->getWorldPosition();
					db.renderLine(playerPos, *lastGMHitLoc, vec3(0.5f, 1.f, 1.f));
				}
			}
		}
#endif

	}

	bool ServerGameMode_Base::tryGameModeHit(const UnfilledObjectiveHit& hit, const std::vector<sp<class PlayerBase>>& allPlayers)
	{
		if (Utils::isValidIndex(teamData, hit.objectiveTeam))
		{
			const GameModeTeamData& team = teamData[hit.objectiveTeam];
			size_t objectivesAboveGoal = team.numObjectivesAlive - team.goal_numObjectives;

			if (bool bTryGamemodeHit = objectivesAboveGoal > team.discrepancyForGameModeHit)
			{
				float timeSinceLastGameModeHit = accumulatedTimeSec - team.lastGameModeHitSec;
				if (timeSinceLastGameModeHit > team.gameModeHitDelaySec)
				{
					bool bGameModeShouldDestroy = true;

					for (const sp<PlayerBase>& player : allPlayers)
					{
						if (const sp<CameraBase>& camera = player->getCamera())
						{
							vec3 camFront = camera->getFront();
							vec3 camPos = camera->getPosition();
							vec3 toTarget = hit.objective->getWorldPosition() - camPos;

							bool bHitBehindPlayer = glm::dot(toTarget, camFront) < 0.f;
							bGameModeShouldDestroy &= bHitBehindPlayer;
						}
					}

					if (bGameModeShouldDestroy)
					{

#if DEBUG_GAMEMODE_HITS
						lastGMHitLoc = hit.objective->getWorldPosition();
#endif
						team.lastGameModeHitSec = accumulatedTimeSec;
						hit.objective->destroy();
						return true;
					}
				}
			}
		}
		return false;
	}


	void ServerGameMode_Base::initialize_LogDebugWarnings()
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// logging for performance critical debug settings that should be turned off
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (!bBaseInitialized) //runtime checking if super class method was called
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Gamemode did not call super OnInitialize - shutting down");
			STOP_DEBUGGER_HERE();
		}

		if constexpr (DEBUG_HITCH_NEVER_CLEAR_PLACEMENTS)
		{
			for (size_t i = 0; i < 3; ++i)
			{
				log(__FUNCTION__, LogLevel::LOG_WARNING, "HITCH DEBUGING FOR PLACEMENTS ENABLED - THIS WILL DEGRADE PERFORMANCE AND SHOULD BE REMOVED IMMEDIATELY AFTER TESTING");
			}
		}
		if constexpr (!AMORTIZE_OBJECTIVE_TARGETING_IN_SHIP_WALK)
		{
			log(__FUNCTION__, LogLevel::LOG_WARNING, "WARNING - amortization for ship placements is off, this is probably a mistake as that is large performance gain");
		}
#if DEBUG_GAMEMODE_HITS
		log(__FUNCTION__, LogLevel::LOG_WARNING, "WARNING - debug gamemode placement balancing enabled, this should not be enabled in shipping build");
#endif
	}

	void ServerGameMode_Base::tick_amortizedObjectiveHitUpdate(float dt_sec)
	{
		const std::vector<sp<PlayerBase>>& allPlayers = GameBase::get().getPlayerSystem().getAllPlayers();
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Process unfilled hits
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		{
			std::optional<std::size_t> removeIdx;
			for (size_t hitIdx = amort_processUnfilledHits.getStartIdx();
				!removeIdx.has_value() && hitIdx < amort_processUnfilledHits.getStopIdxSafe(unfilledObjectiveHits);
				++hitIdx)
			{
				const UnfilledObjectiveHit& hit = unfilledObjectiveHits[hitIdx];
				if (hit.objective)
				{
					if (tryGameModeHit(hit, allPlayers))
					{
						removeIdx = hitIdx;
					}
				}
				else //no objective, must have been destroyed
				{
					removeIdx = hitIdx;
				}

				if (removeIdx)
				{
					Utils::swapAndPopback(unfilledObjectiveHits, *removeIdx);
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Process Filled Hits
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		{
			std::optional<std::size_t> removeIdx;
			for (size_t hitIdx = amort_processFilledHits.updateStart(filledObjectiveHits);
				 !removeIdx.has_value() && hitIdx < amort_processFilledHits.getStopIdxSafe(filledObjectiveHits);
				++hitIdx)
			{
				const FilledObjectiveHit& hit = filledObjectiveHits[hitIdx];
				if (hit.objective)
				{
					if (tryGameModeHit(hit, allPlayers))
					{
						removeIdx = hitIdx;
					}
				}
				else //no objective -- must have been destroyed
				{
					removeIdx = hitIdx;
				}

				if (bool bNeedsReassignent = !hit.assignedShip && !removeIdx.has_value()) //no ship -- perhaps it got destroyed -- need to fill it with new ship
				{
					unfilledObjectiveHits.push_back(hit); //make copy that is an unfilled version, so it can be filled again
					removeIdx = hitIdx;
				}
			}

			if (removeIdx)
			{
				Utils::swapAndPopback(filledObjectiveHits, *removeIdx);
			}
		}

	}



}

