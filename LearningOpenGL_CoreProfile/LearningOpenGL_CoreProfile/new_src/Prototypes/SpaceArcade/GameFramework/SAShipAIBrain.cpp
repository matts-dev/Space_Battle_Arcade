#include "SAShipAIBrain.h"
#include "SALog.h"
#include "SALevel.h"
#include "..\Game\SpaceArcade.h"
#include "SAGameBase.h"
#include "SALevelSystem.h"
#include "..\Game\SAShip.h"
#include "..\Tools\DataStructures\MultiDelegate.h"

namespace SA
{

	ShipAIBrain::ShipAIBrain(const sp<Ship>& controlledShip)
	{
		this->controlledTarget = controlledShip;
	}

	bool ShipAIBrain::onAwaken()
	{
		if (const sp<LevelBase>& level = SpaceArcade::get().getLevelSystem().getCurrentLevel())
		{
			this->wpLevel = level;
			return true;
		}
		else
		{
			log("ShipAIBrain", LogLevel::LOG_ERROR, "failed to start; no level." __FUNCTION__);
			return false;
		}
	}

	void ShipAIBrain::onSleep()
	{
	}

	bool ContinuousFireBrain::onAwaken()
	{
		if (ShipAIBrain::onAwaken())
		{
			createTimerForFire();

			return true;
		}
		return false;
	}

	void ContinuousFireBrain::onSleep()
	{
		if (!wpLevel.expired())
		{
			sp<LevelBase> lvl = wpLevel.lock();
			const sp<TimeManager>& worldTM = lvl->getWorldTimeManager();
			removeFireTimer(*worldTM);
		}
		ShipAIBrain::onSleep();
	}

	void ContinuousFireBrain::createTimerForFire()
	{
		if (!wpLevel.expired())
		{
			sp<LevelBase> level = wpLevel.lock();
			const sp<TimeManager>& worldTM = level->getWorldTimeManager();

			//remove previous timers to make way for the new timer
			removeFireTimer(*worldTM);

			//create a fresh delegate, to avoid worrying about any checking of left over state, etc.
			//make a new delegate to avoid issue where trying to create timer with a delegate pending remove
			fireDelegate = new_sp<MultiDelegate<>>();
			fireDelegate->addWeakObj(sp_this(), &ContinuousFireBrain::handleTimeToFire);

			worldTM->createTimer(fireDelegate, fireRateSec, true, timerDelay);
		}
		else
		{
			log("ContinuousFireBrain", LogLevel::LOG_ERROR, __FUNCTION__ "failed to create timer; no level." );
		}
	}

	void ContinuousFireBrain::removeFireTimer(TimeManager& worldTM)
	{
		if (worldTM.hasTimerForDelegate(fireDelegate))
		{
			worldTM.removeTimer(fireDelegate);
		}

		if (fireDelegate)
		{
			//unbind delegate in case timers are broadcasting, which will defer the remove until after timers stop broadcasting
			fireDelegate->removeWeak(sp_this(), &ContinuousFireBrain::handleTimeToFire);
		}
		fireDelegate = nullptr;
	}

	void ContinuousFireBrain::handleTimeToFire()
	{
		if (!controlledTarget.expired())
		{
			sp<Ship> myShip = controlledTarget.lock();
			myShip->fireProjectile(getBrainKey());
		}
	}

	void ContinuousFireBrain::setFireRateSecs(float InFireRateSecs)
	{
		fireRateSec = InFireRateSecs;
		if (isActive())
		{
			createTimerForFire();
		}
	}

	void ContinuousFireBrain::setDelayStartFire(float delayFirstFireSecs)
	{
		timerDelay = delayFirstFireSecs;
	}

	bool FlyInDirectionBrain::onAwaken()
	{
		if (ShipAIBrain::onAwaken())
		{
			if (!controlledTarget.expired())
			{
				sp<Ship> myShip = controlledTarget.lock();
				const Transform& transform = myShip->getTransform();

				glm::vec3 rotDir = glm::vec3(myShip->getForwardDir());
				glm::vec3 velocity = rotDir * speed;
				myShip->setVelocity(velocity);
			}

			return true;
		}
		return false;
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Fighter Brain
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////



	bool FighterBrain::onAwaken()
	{
		if (ShipAIBrain::onAwaken())
		{
			behaviorTree->start();
			return true;
		}
		return false;
	}

	void FighterBrain::onSleep()
	{
		ShipAIBrain::onSleep();
		behaviorTree->stop();
	}

	void FighterBrain::postConstruct()
	{
		/*
			service_find_target
			selector_has_target
				decorator_hastarget
				selector_hastarget
					TaskMoveToTarget
				decorator_notarget
				sequence_randomLocation
					task_findrandomloc
					task_rotateTowardsLoc
					task_moveToLoc
		*/
		using namespace BehaviorTree;
		//sp<Tree> bt = 
		//	new_sp<Tree>("tree-root",
		//		new_sp<Service>("service_find_target", 0.1f, true,
		//			new_sp<Selector>("selector_hasTarget", std::vector<sp<NodeBase>>{
		//				new_sp<Decorator>("decorator_hastarget", 
		//					new_sp<Selector>("selector_hastarget", std::vector<sp<NodeBase>>{
		//						new_sp<Task>("task_move")
		//					})
		//				),
		//				new_sp<Decorator>("decorator_notarget",
		//					new_sp<Sequence>("sequence_moveToRandomLoc", std::vector<sp<NodeBase>>{
		//						new_sp<Task>("task_FindRandomLoc"),
		//						new_sp<Task>("task_RotateTowardsLoc"),
		//						new_sp<Task>("task_move")
		//					})
		//				)
		//			})
		//		)
		//	);
		//behaviorTree = bt;
	}
}

