#pragma once

#include "../../../Tools/DataStructures/SATransform.h"
#include "../../../GameFramework/SAGameEntity.h"
#include "../../../GameFramework/SATimeManagementSystem.h"
#include "../../../Tools/DataStructures/LifetimePointer.h"

namespace SA
{
	class WorldEntity;
	class LevelBase;

	class HitboxPicker : public GameEntity, public ITickable
	{
	public:
		inline void setPickTarget(const sp<WorldEntity>& newTarget) { pickedObject = newTarget; }

	protected:
		virtual void postConstruct() override;

	private:
		void handleRightClick(int state, int modifier_keys);
		void handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel);
		void handleUIFrameStarted();
		void debugBehaviorTree();

		/** Rough implementation that may have issues; which is why it isn't in a shared location; but the implemenation is good enough for this niche requirement */
		bool rayHitTest_FastAABB(const glm::vec3& boxLow, const glm::vec3& boxMax, const glm::vec3 rayStart, const glm::vec3 rayDir);
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
