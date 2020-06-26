#pragma once
#include "../GameFramework/SAGameEntity.h"
#include "../GameFramework/SAPlayerBase.h"

namespace SA
{
	class SettingsProfileConfig;
	class Mod;

	class SAPlayer : public PlayerBase
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
	protected:
		void postConstruct() override;
		virtual void onNewControlTargetSet(IControllable* oldTarget, IControllable* newTarget) override;
	private:
		void handleControlTargetDestroyed(const sp<GameEntity>& entity);
		void handleRespawnTimerUp();
	private://default input
		void handleEscapeKey(int state, int modifier_keys, int scancode);
		void handleActiveModChanging(const sp<Mod>& previous, const sp<Mod>& active);
	private:
		sp<SettingsProfileConfig> settings;
		sp<MultiDelegate<>> respawnTimerDelegate = nullptr;
		fwp<class FighterSpawnComponent> spawnComp_safeCache;
		size_t currentTeamIdx = 0;
	};
}

