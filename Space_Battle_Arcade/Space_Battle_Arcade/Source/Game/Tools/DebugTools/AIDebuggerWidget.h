
#pragma once

#include "GameFramework/SAGameEntity.h"
#include "GameFramework/Interfaces/SATickable.h"
#include "Tools/DataStructures/AdvancedPtrs.h"
#include <functional>
#include "Tools/DataStructures/SATransform.h"

namespace SA
{
	class LevelBase;
	class Ship;
	class WorldEntity;
	class PlayerBase;

	////////////////////////////////////////////////////////
	// An editor widget for testing ai
	////////////////////////////////////////////////////////
	class AIDebuggerWidget : public GameEntity, public ITickable
	{
		using Parent = GameEntity;
	protected:
		virtual void postConstruct() override;
	private:
		virtual bool tick(float dt_sec) override;
		void handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel);
		void handleUIFrameStarted();
		const Ship* getTarget(const Ship& ship) const;
		void swapFollowTargets();
		void refreshAISimulatingPlayer();
		void togglePossessRelease(const sp<PlayerBase>& player, const fwp<const Ship>& follow) const;
		void havePlayerControlShip(const sp<PlayerBase>& player, const fwp<const Ship>& follow) const;
		bool IsControlling(PlayerBase& player, const Ship& following) const;
	private:
		void destroyAllShips();
		void spawnTwoFightingShips(std::function<void(const sp<Ship>&)> brainSpawningFunction);
		void spawnFighterToAttackObjective();
		sp<Ship> helperSpawnShip(const glm::vec3& pos, const glm::vec3& forward_n, size_t teamIdx);
	private:
		float cameraDistance = 50.f;
		bool bCameraFollowTarget = true;
		bool bPlanarCameraMode = true;
		bool bSimulatePlayer = false;
		bool bChangeBackgroundColor = false;
	private:
		fwp<const Ship> following;
	};
}