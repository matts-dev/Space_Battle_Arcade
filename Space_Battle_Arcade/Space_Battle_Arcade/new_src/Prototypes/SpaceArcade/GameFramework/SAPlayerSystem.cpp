#include "SAPlayerSystem.h"

namespace SA
{
	const sp<PlayerBase> PlayerSystem::NULL_PLAYER = nullptr;

	const sp<SA::PlayerBase>& PlayerSystem::getPlayer(uint32_t idx)
	{
		if (idx < players.size())
		{
			return players[idx];
		}

		//returning just nullptr will become a local temporary -- don't return local temporary reference
		return NULL_PLAYER;
	}
}

