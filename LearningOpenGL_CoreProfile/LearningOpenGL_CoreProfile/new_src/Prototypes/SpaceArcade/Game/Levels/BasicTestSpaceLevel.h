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
	class ProjectileConfig;
	class CollisionDebugRenderer;
	class ProjectileTweakerWidget;
	class HitboxPicker;
	class ParticleConfig;

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
		void handleEntityDestroyed(const sp<GameEntity>& entity);


		void refreshShipContinuousFireState();
		void refreshProjectiles();

		void toggleSpawnTestParticles();
		void handleTestParticleSpawn();

	private: //debug variables
#if SA_RENDER_DEBUG_INFO
		bool bRenderCollisionOBB_ui = false;
		bool bRenderCollisionShapes_ui = false;
#endif //SA_RENDER_DEBUG_INFO
		bool bForceShipsToFire_ui = false;

		//time dilation testing
		float forceFireRateSecs_ui = 1.0;
		bool bFreezeTimeOnClick_ui = false;
		float timeDilationFactor_ui = 1.f;

		//particle testing
		bool bShowProjectileTweaker_ui = false;
		bool bContinuousSpawns_ui = true;
		int numParticleSpawns_ui = 10; //don't use directly, use this * multiplier
		int numSpawnMultiplier_ui = 10; 
		int numSpawnBatches = 5;
		int numParticlesInBatch = 0;
		int selectedTestParticleIdx = 0;
		glm::vec2 particleSpawnBounds{ -200, 200 }; //x=min, y=max
		glm::vec3 particleSpawnOffset;
		std::map<std::string, sp<ParticleConfig>> testParticles;
		sp<ParticleConfig> testParticleConfig = nullptr;
		bool bSpawningParticles = false;
		sp<MultiDelegate<>> particleSpawnDelegate = nullptr;

	private: //testing projectile
		sp<ProjectileConfig> testProjectileConfig = nullptr;
		int selectedProjectileConfigIdx = 0;

	private:
		//std::multimap<RenderModelEntity*, wp<RenderModelEntity>> cachedSpawnEntities;
		sp<SpawnConfig> fighterSpawnConfig;
		sp<CollisionDebugRenderer> collisionDebugRenderer;
		sp<ProjectileTweakerWidget> projectileWidget;
		sp<HitboxPicker> hitboxPickerWidget;

		//needs to potentially have O(n) iteration
		std::set<sp<Ship>> spawnedShips;
	};
}
