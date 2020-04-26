#pragma once
#include "../GameFramework/SAGameEntity.h"
#include "../GameFramework/SAPlayerBase.h"

namespace SA
{
	class SAPlayer : public PlayerBase
	{
	public:
		SAPlayer(int32_t playerIndex) : PlayerBase(playerIndex) {};
		virtual ~SAPlayer() {}
		virtual sp<CameraBase> generateDefaultCamera() const;
	public:
		MultiDelegate<float /*respawn time*/> onRespawnStarted;
		MultiDelegate<bool /* Respawn Success */> onRespawnOver;
	protected:
		void postConstruct() override;
		virtual void onNewControlTargetSet(IControllable* oldTarget, IControllable* newTarget) override;
	private:
		void handleControlTargetDestroyed(const sp<GameEntity>& entity);
		void handleRespawnTimerUp();
	private:
		sp<MultiDelegate<>> respawnTimerDelegate = nullptr;
		fwp<class FighterSpawnComponent> spawnComp_safeCache;
		size_t currentTeamIdx = 0;
	};
}

