#pragma once
#include "ServerGameMode_Base.h"
#include "../../Tools/DataStructures/LifetimePointer.h"

namespace SA
{
	class Ship;

	class ServerGameMode_CarrierTakedown : public ServerGameMode_Base
	{
	protected:
		virtual void onInitialize(const sp<SpaceLevelBase>& level);
	private:
		void processLoadedGameData();
	private:
		void handleCarrierDestroyed(const sp<GameEntity>&);
	public:
		struct TeamData
		{
			std::vector<wp<Ship>> carriers;
		};
	private:
		std::vector<TeamData> myTeamData;
	};


}