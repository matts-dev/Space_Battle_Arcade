#pragma once
#include <map>

#include "SASpaceLevelBase.h"

namespace SA
{
	class Ship;
	class RenderModelEntity;
	class ProjectileClassHandle;
	class Mod;
	class SpawnConfig;
	class CollisionDebugRenderer;

	class BasicTestSpaceLevel : public SpaceLevelBase
	{
	public:
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;

	protected:
		virtual void onEntitySpawned_v(const sp<WorldEntity>& spawned) override;
		virtual void onEntityUnspawned_v(const sp<WorldEntity>& unspawned) override;

	private:
		virtual void postConstruct() override;
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;

		void handleLeftMouseButton(int state, int modifier_keys);
		void handleActiveModChanging(const sp<Mod>& previous, const sp<Mod>& active);
		void handleUIFrameStarted();

	private: //debug variables
#ifdef SA_RENDER_DEBUG_INFO
		bool bRenderCollisionOBB = false;
		bool bRenderCollisionShapes = false;
#endif SA_RENDER_DEBUG_INFO

	private:
		//std::multimap<RenderModelEntity*, wp<RenderModelEntity>> cachedSpawnEntities;
		sp<SpawnConfig> fighterSpawnConfig;
		sp<ProjectileClassHandle> laserBoltHandle;
		sp<CollisionDebugRenderer> collisionDebugRenderer;

		//needs to potentially have O(n) iteration
		std::set<sp<Ship>> spawnedShips;
	};
}
