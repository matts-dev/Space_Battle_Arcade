#pragma once

#include "../../../Tools/DataStructures/SATransform.h"
#include "../../../GameFramework/SAGameEntity.h"
#include "../../../GameFramework/SATimeManagementSystem.h"
#include "../../../Tools/DataStructures/LifetimePointer.h"

namespace SA
{
	class WorldEntity;
	class LevelBase;
	class IControllable;

	class HitboxPicker : public GameEntity, public ITickable
	{
	public:
		inline void setPickTarget(const sp<WorldEntity>& newTarget) { pickedObject = newTarget; }
		inline sp<WorldEntity> getPickTarget() { return pickedObject; }

	protected:
		virtual void postConstruct() override;

	private:
		void handleRightClick(int state, int modifier_keys);
		void handlePlayerControlTargetSet(IControllable* oldTarget, IControllable* newTarget);
		void handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel);
		void handleUIFrameStarted();
		void debugBehaviorTree();

	private: //debug rendering
		float accumulatedTime = 0.f;
		float renderDuration = 5.0f;
		float cameraDistance = 5.0f;
		bool bDrawPickRays = true;
		bool bClearPickingAfterSuccessfulPick = false;
		bool bCameraFollowTarget = true;
		bool bCameraFollowTarget_behind = false;
		glm::vec3 rayStart;
		glm::vec3 rayEnd;
		virtual bool tick(float dt_sec) override;

	private:
		bool bPickingObject = false;
		float maxRayDistance = 1000.0f;
		lp<WorldEntity> pickedObject;
	};


}
