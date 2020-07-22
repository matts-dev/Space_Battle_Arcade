#pragma once
#include "../SAGameEntity.h"

namespace SA
{
	class ServerGameMode_Base : public GameEntity
	{
	public:
		virtual size_t getNumberOfCurrentTeams() const {return 2;};
		//#TODO delegate to broadcast if number of teams change?
	};
}