#pragma once

#include<cstdint>
#include <vector>

#include "GameFramework/SASystemBase.h"
#include "Tools/DataStructures/MultiDelegate.h"

namespace SA
{
	class PlayerBase;

#define MAX_LOCAL_PLAYERS 2

	class PlayerSystem : public SystemBase
	{
	public:
		size_t numPlayers() const { return players.size(); }
		const sp<PlayerBase>& getPlayer(uint32_t player_idx);
		const std::vector<sp<PlayerBase>>& getAllPlayers() const { return players; }

		template<typename T = PlayerBase>
		sp<T> createPlayer()
		{
			static_assert(std::is_base_of<SA::PlayerBase, T>::value, "Players must be of the base class PlayerBase.");

			sp<T> newPlayer = new_sp<T>(static_cast<int32_t>(players.size()));
			players.push_back(newPlayer);

			//broadcast that player was created with explicit PlayerBase type
			uint32_t playerIdx = players.size() - 1;
			onPlayerCreated.broadcast(getPlayer(playerIdx), playerIdx);

			return newPlayer;
		}
	public: 
		/** returnable null object */
		static const sp<PlayerBase> NULL_PLAYER;

		/** prefer static_cast if player hierarchy is always known */
		MultiDelegate<const sp<PlayerBase>& /*player*/, uint32_t /*idx*/> onPlayerCreated;

	private:
		std::vector<sp<PlayerBase>> players;
	};
}