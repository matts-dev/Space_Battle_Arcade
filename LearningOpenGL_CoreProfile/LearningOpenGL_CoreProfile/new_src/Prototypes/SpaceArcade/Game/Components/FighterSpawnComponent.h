#pragma once
#include <vector>

#include "../AssetConfigs/SASpawnConfig.h"
#include "../SAShip.h"
#include "../../GameFramework/Components/SAComponentEntity.h"
#include "../../Tools/DataStructures/SATransform.h"
#include <functional>

namespace SA
{
	class RNG;

	/** Passing sp so that entity can be cached through this callback; otherwise would be referene to avoid need for nullcheck*/
	using PostSpawnCustomizationFunc = std::function<void(const sp<Ship>&)>;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Entity Spawning Component
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class FighterSpawnComponent : public GameComponentBase
	{
		using SpawnType = Ship;
	public:
		struct AutoRespawnConfiguration
		{
			size_t maxShips = 2500;
			float respawnCooldownSec = 1.0f;
			bool bEnabled = true;
		};
	public:
		void loadSpawnPointData(const SpawnConfig& spawnData);
		void updateParentTransform(const glm::mat4& inParentXform) { parentXform = inParentXform; }
		void tick(float dt_sec);
		void setPostSpawnCustomization(const PostSpawnCustomizationFunc& inFunc);
		void setTeamIdx(size_t inTeamIdx) { teamIdx = inTeamIdx; }
		void setAutoRespawnConfig(const AutoRespawnConfiguration& newConfig) { autoSpawnConfiguration = newConfig; }
		sp<SpawnType> spawnEntity();
	protected:
		void postConstruct();
	private:
		void handleOwnedEntityDestroyed(const sp<GameEntity>& destroyed);
	public:
		MultiDelegate<const sp<SpawnType>&> onSpawnedEntity;
	private:
		static sp<RNG> rng;
	private:
		std::vector<sp<SpawnConfig>> validSpawnables;
		std::vector<FighterSpawnPoint> mySpawnPoints;
		std::unordered_set<sp<GameEntity>> spawnedEntities;
		PostSpawnCustomizationFunc customizationFunc;
		glm::mat4 parentXform{1.f}; 
		size_t teamIdx = 0;
		AutoRespawnConfiguration autoSpawnConfiguration;
		float timeSinceLastSpawnSec = 0.f;
	};
}
