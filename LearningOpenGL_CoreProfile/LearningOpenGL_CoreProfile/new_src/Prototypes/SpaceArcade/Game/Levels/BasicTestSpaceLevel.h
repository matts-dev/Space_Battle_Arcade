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
	class ProjectileTweakerWidget;

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
		void handleRightMouseButton(int state, int modifier_keys);
		void handleActiveModChanging(const sp<Mod>& previous, const sp<Mod>& active);
		void handleUIFrameStarted();


		void refreshShipContinuousFireState();

	private: //debug variables
#if SA_RENDER_DEBUG_INFO
		bool bRenderCollisionOBB_ui = false;
		bool bRenderCollisionShapes_ui = false;
#endif //SA_RENDER_DEBUG_INFO
		bool bForceShipsToFire_ui = false;
		float forceFireRateSecs_ui = 1.0;
		bool bFreezeTimeOnClick_ui = false;
		float timeDilationFactor_ui = 1.f;
		bool bShowProjectileTweaker_ui = false;

	private:
		//std::multimap<RenderModelEntity*, wp<RenderModelEntity>> cachedSpawnEntities;
		sp<SpawnConfig> fighterSpawnConfig;
		sp<ProjectileClassHandle> laserBoltHandle;
		sp<CollisionDebugRenderer> collisionDebugRenderer;
		sp<ProjectileTweakerWidget> projectileWidget;

		//needs to potentially have O(n) iteration
		std::set<sp<Ship>> spawnedShips;
	};
}
