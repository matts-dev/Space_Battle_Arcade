#include "SAShipAIBrain.h"
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SALevel.h"
#include "../SpaceArcade.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../SAShip.h"
#include "../../Tools/DataStructures/MultiDelegate.h"
#include "SAShipBehaviorTreeNodes.h"

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

	SA::Ship* ShipAIBrain::getControlledTarget()
	{
		const Ship* ship = static_cast<const ShipAIBrain&>(*this).getControlledTarget();
		return const_cast<Ship*>(ship);
	}

	const SA::Ship* ShipAIBrain::getControlledTarget() const
	{
		if (!controlledTarget.expired())
		{
			if (sp<Ship> myTarget = controlledTarget.lock())
			{
				return myTarget.get();
			}
		}
		return nullptr;
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

	/////////////////////////////////////////////////////////////////////////////////////
	// behavior tree brain
	/////////////////////////////////////////////////////////////////////////////////////
	bool BehaviorTreeBrain::onAwaken()
	{
		if (ShipAIBrain::onAwaken())
		{
			behaviorTree->start();

			const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel();
			currentLevel->getWorldTimeManager()->registerTicker(sp_this());
			tickingOnLevel = currentLevel;

			return true;
		}
		return false;
	}

	void BehaviorTreeBrain::postConstruct()
	{
		// subclasses should set behavior tree in an override of this function
		ShipAIBrain::postConstruct();
	}

	bool BehaviorTreeBrain::tick(float dt_sec)
	{
		behaviorTree->tick(dt_sec);
		return true;
	}

	void BehaviorTreeBrain::onSleep()
	{
		ShipAIBrain::onSleep();
		behaviorTree->stop();

		if (!tickingOnLevel.expired())
		{
			tickingOnLevel.lock()->getWorldTimeManager()->removeTicker(sp_this());
		}

	}

	/////////////////////////////////////////////////////////////////////////////////////
	// wander brain
	/////////////////////////////////////////////////////////////////////////////////////
	void WanderBrain::postConstruct()
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
		behaviorTree =
			new_sp<Tree>("tree-root",
				new_sp<Selector>("RootSelector", MakeChildren{
					new_sp<Sequence>("Sequence_MoveToNewLocation", MakeChildren{
						//new_sp<Task_Ship_SaveShipLocation>("ship_loc", "selfBrain"),
						new_sp<Task_FindRandomLocationNearby>("target_loc", "ship_loc", 200.0f),
						new_sp<Task_Ship_MoveToLocation>("selfBrain", "target_loc", 10.0f)
					})
				}),
				MemoryInitializer
				{
					{"selfBrain", sp_this() },
					{ "ship_loc", new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ "target_loc", new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) }
				}
			);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Fighter Brain
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

