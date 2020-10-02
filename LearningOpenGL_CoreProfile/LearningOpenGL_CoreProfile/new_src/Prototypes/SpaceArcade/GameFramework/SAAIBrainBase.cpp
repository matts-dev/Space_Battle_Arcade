#include "SAAIBrainBase.h"
#include "SALog.h"
#include <assert.h>
#include "..\Tools\DataStructures\MultiDelegate.h"
#include "SALevelSystem.h"
#include "SAGameBase.h"
#include "SALevel.h"
#include "SABehaviorTree.h"
#include "SATimeManagementSystem.h"

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

