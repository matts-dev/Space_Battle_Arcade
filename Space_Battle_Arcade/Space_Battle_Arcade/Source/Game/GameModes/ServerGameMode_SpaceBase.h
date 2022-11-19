#pragma once
#include "GameFramework/SAGameEntity.h"
#include "Tools/RemoveSpecialMemberFunctionUtils.h"
#include "Tools/DataStructures/LifetimePointer.h"
#include "Game/SAShip.h" //must include this to use lifetime pointers ATOW #nextengine don't let lifetime points screw up using forward declarations
#include "Tools/Algorithms/AmortizeLoopTool.h"
#include "GameFramework/GameMode/ServerGameMode_Base.h"

namespace SA
{
	class SpaceLevelBase;
	class BasicTestSpaceLevel;
	class WorldEntity;
	class Ship;
	class ShipPlacementEntity;
	class DifficultyConfig;
	struct EndGameParameters;

	struct GameModeTeamData
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// constants #TODO move out of object instance
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		size_t discrepancyForGameModeHit = 4;
		float gameModeHitDelaySec = 4.0;

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// persistent data
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//lifetime pointers(lp) are not cleared to give an idea of how many carriers have been destroyed
		std::vector<lp<Ship>> carriers;


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//mutable data that is updated each frame for decision making
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		float percentAlive_Carriers = 1.f;
		float percentAlive_Objectives = 1.f;
		size_t numObjectivesAtStart = 0;
		size_t numCarriersAlive = 0;
		size_t numObjectivesAlive = 0;
		size_t activeHitsAgainstTeam = 0;
		bool bIsPlayerTeam = false;

		//mutable data that is set to apply pressure to this team to match other teams
		size_t goal_numObjectives = 0;
		float lastGameModeHitSec = 0.f;
	};

	////////////////////////////////////////////////////////
	// used to track an objective that needs destroying, but
	// no ship has been assigned to it yet.
	////////////////////////////////////////////////////////
	struct UnfilledObjectiveHit
	{
		//ctor to ensure that this data is provided at construction
		UnfilledObjectiveHit(const sp<ShipPlacementEntity>& inObjective, size_t team)
			:objective(inObjective), objectiveTeam(team)
		{}

		lp<ShipPlacementEntity> objective;
		size_t objectiveTeam;
	};

	////////////////////////////////////////////////////////
	// used to track an objective hit that has had a ship
	// assigned to destroy (hit) the objective
	////////////////////////////////////////////////////////
	struct FilledObjectiveHit : public UnfilledObjectiveHit
	{
		FilledObjectiveHit(const lp<Ship>& ship, const UnfilledObjectiveHit& unfilledHit) 
			: assignedShip(ship), UnfilledObjectiveHit(unfilledHit.objective, unfilledHit.objectiveTeam)
		{}
		lp<Ship> assignedShip;
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The server game mode base class
	//
	// #TODO #refactor started refactor to ServerGameMode_Base, but haven't moved common functionality like getting level
	// there yet. leaving that as TODO
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class ServerGameMode_SpaceBase : public ServerGameMode_Base
	{
	public:
		struct LevelKey : public RemoveMoves {friend SpaceLevelBase; friend BasicTestSpaceLevel; private: LevelKey() {}};
	public: //statics
		static std::vector<fwp<PlayerBase>> playersNeedingTarget; //#TODO refactor this to be object oriented and not static
	public:
		void initialize(const LevelKey& key);
		void tick(float dt_sec, const LevelKey& key);
		void						setOwningLevel(const sp<SpaceLevelBase>& level);
		const wp<SpaceLevelBase>&	getOwningLevel() const { return weakOwningLevel; }
		size_t getNumTeams() const { return numTeams; }
		void addTurretNeedingTarget(const sp<ShipPlacementEntity>& turret);
		void addHealerNeedingTarget(const sp<ShipPlacementEntity>& healer);
		const std::vector<GameModeTeamData>& getTeamData() { return teamData; }
		size_t getNumberOfCurrentTeams() const override;
	protected:
		virtual void onInitialize(const sp<SpaceLevelBase>& level);
		void endGame(const EndGameParameters& endParameters);
		void onWorldEntitySpawned(const sp<WorldEntity>& spawned) {};
	private:
		void handleEntitySpawned(const sp<WorldEntity>& spawned);
		void tick_carrierObjectiveBalancing(float dt_sec);
		void tick_singleShipWalk(float dt_sec);
		void tick_debug(float dt_sec);
		void tick_amortizedObjectiveHitUpdate(float dt_sec);
		bool tryGameModeHit(const UnfilledObjectiveHit& hit, const std::vector<sp<class PlayerBase>>& allPlayers);
		void initialize_LogDebugWarnings();
	private: //cache
		sp<DifficultyConfig> difficultyConfig;
	private: //transient data
		AmortizeLoopTool amort_turrets;
		AmortizeLoopTool amort_healers;
		AmortizeLoopTool amort_fillUnfilledHits;
		AmortizeLoopTool amort_processUnfilledHits;
		AmortizeLoopTool amort_processFilledHits;
	private: //time
		float accumulatedTimeSec = 0.f;
		float timestamp_lastPlayerTeamRefresh = 0.f;
	protected:
		size_t numTeams = 2;
	private:
		bool bBaseInitialized = false; //perhaps make static and reset since this will ever be created from single thread
		wp<SpaceLevelBase> weakOwningLevel;
		std::vector<lp<Ship>> ships;
		std::vector<GameModeTeamData> teamData;
		std::vector<lp<ShipPlacementEntity>> turretsNeedingTarget;
		std::vector<lp<ShipPlacementEntity>> healersNeedingTarget;
		std::vector<UnfilledObjectiveHit> unfilledObjectiveHits; //objective hits are like contracts between ships and an object to destroy
		std::vector<FilledObjectiveHit> filledObjectiveHits;
	};
}
