#pragma once
#include "../../GameFramework/SAGameEntity.h"
#include "../../Tools/RemoveSpecialMemberFunctionUtils.h"
#include "../../Tools/DataStructures/LifetimePointer.h"
#include "../SAShip.h" //must include this to use lifetime pointers ATOW #nextengine don't let lifetime points screw up using forward declarations

namespace SA
{
	class SpaceLevelBase;
	class BasicTestSpaceLevel;
	class WorldEntity;
	class Ship;
	class ShipPlacementEntity;
	struct EndGameParameters;

	class ServerGameMode_Base : public GameEntity
	{
	public:
		struct LevelKey : public RemoveMoves {friend SpaceLevelBase; friend BasicTestSpaceLevel; private: LevelKey() {}};
	public: //statics
		static std::vector<fwp<PlayerBase>> playersNeedingTarget;
	public:
		void initialize(const LevelKey& key);
		void tick(float dt_sec, const LevelKey& key);
		void						setOwningLevel(const sp<SpaceLevelBase>& level);
		const wp<SpaceLevelBase>&	getOwningLevel() const { return owningLevel; }
		size_t getNumTeams() const { return numTeams; }
		void addTurretNeedingTarget(const sp<ShipPlacementEntity>& turret);
		void addHealerNeedingTarget(const sp<ShipPlacementEntity>& healer);
	protected:
		virtual void onInitialize(const sp<SpaceLevelBase>& level);
		void endGame(const EndGameParameters& endParameters);
		void onWorldEntitySpawned(const sp<WorldEntity>& spawned) {};
	private:
		void handleEntitySpawned(const sp<WorldEntity>& spawned);
		void tick_SingleShipWalk(float dt_sec);
	protected:
		size_t numTeams = 2;
	private:
		bool bBaseInitialized = false; //perhaps make static and reset since this will ever be created from single thread
		wp<SpaceLevelBase> owningLevel;
		std::vector<lp<Ship>> ships;
		std::vector<lp<ShipPlacementEntity>> turretsNeedingTarget;
		std::vector<lp<ShipPlacementEntity>> healersNeedingTarget;
	};
}
