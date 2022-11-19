#pragma once
#include "GameFramework/SAGameEntity.h"
#include "GameFramework/SAPlayerBase.h"

namespace SA
{
	class SettingsProfileConfig;
	class Mod;
	class LevelBase;

	class SAPlayer : public PlayerBase, public ITickable
	{
	public:
		SAPlayer(int32_t playerIndex) : PlayerBase(playerIndex) {};
		virtual ~SAPlayer() {}
		virtual sp<CameraBase> generateDefaultCamera() const;
		void setSettingsProfile(const sp<SettingsProfileConfig>& newSettingsProfile);
		const sp<SettingsProfileConfig>& getSettingsProfile() { return settings; }
		size_t getCurrentTeamIdx() { return currentTeamIdx; }
	public:
		MultiDelegate<float /*respawn time*/> onRespawnStarted;
		MultiDelegate<bool /* Respawn Success */> onRespawnOver;
		MultiDelegate<const sp<SettingsProfileConfig>& /*oldSettings*/, const sp<SettingsProfileConfig>& /*newSettings*/> onSettingsChanged;
	public:
		bool canDilateTime() const;
		void incrementKillCount();
		void cheat_infiniteTimeDilation();
	protected:
		void postConstruct() override;
		virtual void onNewControlTargetSet(IControllable* oldTarget, IControllable* newTarget) override;
	private:
		void handleControlTargetDestroyed(const sp<GameEntity>& entity);
		void handleRespawnTimerUp();
		void handleActiveModChanging(const sp<Mod>& previous, const sp<Mod>& active);
		void handleShutdownStarted();
		void handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel);
	private://default input
		void handleEscapeKey(int state, int modifier_keys, int scancode);
		void handleControlPressed(int state, int modifier_keys, int scancode);
		void handleTabPressed(int state, int modifier_keys, int scancode);
		virtual bool tick(float dt_sec) override;
	private://impl
		void startDilation();
		void stopDilation();
	private:
		float currentTimeSec = 0.f;
		size_t shipsKilled = 0;
		struct TimeDilationData
		{
			//note: slowmo kind of breaks the normal gameplay as you start to want to use slowmo on everything
			//it either needs to have a long cool down, or based on number of ships downed -- I think the later
			bool bEnablePlayerTimeDilationControl = false;
			bool bUseCooldownMode = false;
			//cooldown logic
			float DILATION_MAX_DURATION = 10.f;//not const to allow cheats
			float DILATION_COOLDOWN_SEC = 30.f;//not const to allow cheats
			float playerTimeDilationMultiplier = 1.f; //#todo splitscreen only player 1 will need to be doing this
			float lastDilationTimestamp = 0.f; //-60.f; //start out a minute behind
			float lastDilationStart = 0.f; //-60.f;
			//kill logic
			size_t NUM_KILLS_FOR_SLOWMO_INCREMENT = 3;
			size_t nextSlomoKillCount = NUM_KILLS_FOR_SLOWMO_INCREMENT;

			//shared logic
			bool bIsDilatingTime = false;
			bool bTimeRestored = false;
		}td;
	private:
		sp<SettingsProfileConfig> settings;
		sp<MultiDelegate<>> respawnTimerDelegate = nullptr;
		fwp<class FighterSpawnComponent> spawnComp_safeCache;
		size_t currentTeamIdx = 0;
	};
}

