#include "ServerGameMode_Base.h"
#include "../Levels/SASpaceLevelBase.h"
#include "../../Tools/PlatformUtils.h"
#include "../SAShip.h"
#include "../../Tools/SAUtilities.h"

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
	constexpr bool AMORTIZE_ITEMS_IN_SHIP_WALK= true;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
		if constexpr (!AMORTIZE_ITEMS_IN_SHIP_WALK)
		{
			log(__FUNCTION__, LogLevel::LOG_WARNING, "WARNING - amortization for ship placements is off, this is probably a mistake as that is large performance gain");
		}
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
		tick_SingleShipWalk(dt_sec);


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
		}

		//let subclass gamemodes know 
		onWorldEntitySpawned(spawned);
	}

	void ServerGameMode_Base::tick_SingleShipWalk(float dt_sec)
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
		//data chunks are divided up an processed over multiple ticks, this is an idx representing the tick
		static size_t amoritizedTickIdx_turret = 0; 
		static size_t amoritizedTickIdx_healers = 0;
		constexpr size_t placementChunkSize = 4;
		size_t turretStartIdx = 0; //defines where to start ticking turrets from for the current tick chunk
		size_t healerStartIdx = 0; //see turret start idx note
		if (AMORTIZE_ITEMS_IN_SHIP_WALK)
		{
			auto getWrapIdx = [](size_t containerSize, size_t& amortizedIdx, size_t chunkSize)
			{
				//every tick we do the X number of tests (defined by maxTestsPlacements);
				//so if we multiply the amoritized tick idx by this number, we get the start idx in an infinite sized array
				//since we're using a fixed size array, we use modulus to trim this down into the range of our actual tick group
				// consider array size 7, with processing of 3 items per tick.
				// 0 = [0-2], 1 = [3-5], 2=[6,7,8], 3=[]
				// (3 * 3) % 7 = 2; so we need logic to reset once out of bounds; which means simple modulus isn't going to work
				size_t wrapAroundStartIdx = amortizedIdx * chunkSize;
				if (amortizedIdx > containerSize)
				{
					amortizedIdx = 0;
					wrapAroundStartIdx = 0;
				}
				return wrapAroundStartIdx;
			};

			turretStartIdx = getWrapIdx(turretsNeedingTarget.size(), amoritizedTickIdx_turret, placementChunkSize);
			healerStartIdx = getWrapIdx(healersNeedingTarget.size(), amoritizedTickIdx_healers, placementChunkSize);

			++amoritizedTickIdx_turret;
			++amoritizedTickIdx_healers;
		}


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
			if (turretsNeedingTarget.size() > 0)
			{
				std::optional<size_t> removeIdx;
				for (size_t attackPlacementIdx = turretStartIdx;
						attackPlacementIdx < turretsNeedingTarget.size() 
						&& !removeIdx.has_value()
						&& (AMORTIZE_ITEMS_IN_SHIP_WALK && attackPlacementIdx < turretStartIdx+placementChunkSize);
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
			if (healersNeedingTarget.size() > 0)
			{
				std::optional<size_t> removeIdx;
				for (size_t defPlacementIdx = 0; 
						defPlacementIdx < healersNeedingTarget.size()
						&& !removeIdx.has_value()
						&& (AMORTIZE_ITEMS_IN_SHIP_WALK && defPlacementIdx < healerStartIdx + placementChunkSize);
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
		}
	}

}

