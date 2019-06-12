#include "SAPlayerSubsystem.h"

namespace SA
{
	const sp<PlayerBase> PlayerSubsystem::NULL_PLAYER = nullptr;

	const sp<SA::PlayerBase>& PlayerSubsystem::getPlayer(uint32_t idx)
	{
		if (idx < players.size())
		{
			return players[idx];
		}

		//returning just nullptr will become a local temporary -- don't return local temporary reference
		return NULL_PLAYER;
	}
}

