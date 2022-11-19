#include "GameFramework/SAAIBrainBase.h"
#include "GameFramework/SALog.h"
#include <assert.h>
#include "Tools/DataStructures/MultiDelegate.h"
#include "GameFramework/SALevelSystem.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SALevel.h"
#include "GameFramework/SABehaviorTree.h"
#include "GameFramework/SATimeManagementSystem.h"

namespace SA
{
	bool AIBrain::awaken()
	{
		bActive = onAwaken();
		return bActive;
	}

	void AIBrain::sleep()
	{
		onSleep();
		bActive = false;
	}

	bool BehaviorTreeBrain::onAwaken()
	{

		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			behaviorTree->start();
			currentLevel->getWorldTimeManager()->registerTicker(sp_this());
			tickingOnLevel = currentLevel;
			return true;
		}
		return false;
	}

	void BehaviorTreeBrain::onSleep()
	{
		behaviorTree->stop();

		if (!tickingOnLevel.expired())
		{
			tickingOnLevel.lock()->getWorldTimeManager()->removeTicker(sp_this());
		}
	}

	bool BehaviorTreeBrain::tick(float dt_sec)
	{
		behaviorTree->tick(dt_sec);
		return true;
	}

}

