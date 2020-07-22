#pragma once
#include "ServerGameMode_SpaceBase.h"
#include "../../Tools/DataStructures/LifetimePointer.h"

constexpr bool bTargetHeartbeatEveryFrame = false;

namespace SA
{
	class Ship;
	class RNG;

	class ServerGameMode_CarrierTakedown : public ServerGameMode_SpaceBase
	{
	public:
		void configureForTestLevel();
	protected:
		virtual void onInitialize(const sp<SpaceLevelBase>& level);
	private:
		void processLoadedGameData();
	private:
		void handleCarrierDestroyed(const sp<GameEntity>&);
		void targetPlayerHeartBeat(); //target player at regular interval if they are not targeted
	public:
		struct TeamData
		{
			std::vector<wp<Ship>> carriers;
			size_t teamIdx = 0;
		};
	private: //config
		struct TargetPlayerHeartbeatConfig
		{
			float heartBeatSec = 2.5f;
			float minWaitSec = 20.f;
			float maxWaitSecc = 30.f;
		} targetPlayerHeartbeatConfig;
	private:
		std::vector<TeamData> myTeamData;
		sp<MultiDelegate<>> targetPlayerHeartBeatDelegate;
		sp<RNG> myRNG;
		struct TargetPlayerHeartbeatData
		{
			float waitSec = 0.f;
			float accumulatedTime = 0.f;
		}targetPlayerHeartbeatData;
	};


}