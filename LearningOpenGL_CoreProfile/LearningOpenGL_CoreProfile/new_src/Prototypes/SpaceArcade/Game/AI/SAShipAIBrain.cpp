#include "SAShipAIBrain.h"
#include "SAShipBehaviorTreeNodes.h"
#include "../SAShip.h"
#include "../SpaceArcade.h"
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SALevel.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../../Tools/DataStructures/MultiDelegate.h"
#include "../../GameFramework/SABehaviorTreeHelpers.h"
#include "../../GameFramework/BehaviorTree_ProvidedNodes.h"
#include "../../GameFramework/SAWorldEntity.h"

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
		using namespace BehaviorTree;
		behaviorTree =
			new_sp<Tree>("tree-root",
				new_sp<Selector>("RootSelector", MakeChildren{
					new_sp<Sequence>("Sequence_MoveToNewLocation", MakeChildren{
						//new_sp<Task_Ship_SaveShipLocation>("ship_loc", "selfBrain"),
						new_sp<Task_FindRandomLocationNearby>("target_loc", "ship_loc", 400.0f),
						new_sp<Task_Ship_MoveToLocation>("selfBrain", "target_loc", 45.0f)
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
		notes:
			-need "opportunistic shots" in that if a non-target but attacker gets in crosshairs, then it will fire
			-need "friendly friend accidents collection" so if a friendly fire happens it will line trace to avoid that

		tree:
			service_targetFinder		//tries to find a target by looking around spatial hash (or for friends that have a target for "mob" behavior
			service_opportunisitic_shots	//shoot at target/attackers if within crosshairs
				loop
					select
						evade
							select-random
								corkscrew-spiral
								corkscrew-turn
								turn-back180
						attack
							orient_towards(target)		//opportunistic shots service will take care of firing
						decorator_abort_on_targetFound
						decorator_abort_on_attackerFound
						wander
							-find random location
								-move to random location

		*/

		const char* const brainKey = "selfBrain";
		const char* const stateKey = "stateKey";
		const char* const originKey = "origin";
		const char* const wanderLocKey = "wander_loc";
		const char* const targetKey = "target";
		const char* const targetLocKey = "target_loc";
		const char* const secondaryTargetsKey = "secondaryTargetsKeys";


		using namespace BehaviorTree;
		behaviorTree =
			new_sp<Tree>("fighter-tree-root",
				new_sp<Decorator_FighterStateSetter>("decor_state_setter", stateKey, targetKey,
				new_sp<Service_TargetFinder>("service_targetFinder", 1.0f, true, brainKey, targetKey,
				new_sp<Service_OpportunisiticShots>("service_opportunisiticShots", 0.1f, true, brainKey, targetKey, secondaryTargetsKey,
					new_sp<Loop>("fighter-inf-loop", 0,
						new_sp<Selector>("state_selector", MakeChildren{
							new_sp<Decorator_Aborting_Is<MentalState_Fighter>>("dec_evade_state", stateKey, OP::EQUAL, MentalState_Fighter::EVADE, AbortPreference::ABORT_ON_MODIFY,
								new_sp<Task_PlaceHolder>("task_evade", true)
							),
							new_sp<Decorator_Aborting_Is<MentalState_Fighter>>("dec_attack_state", stateKey, OP::EQUAL, MentalState_Fighter::FIGHT, AbortPreference::ABORT_ON_MODIFY,
								new_sp<Task_Ship_FollowTarget_Indefinitely>("task_followTarget", brainKey, targetKey)
								//new_sp<Sequence>("seq_move_to_target", MakeChildren{
								//	new_sp<Task_Ship_GetEntityLocation>("get_target_loc", targetKey, targetLocKey),
								//	new_sp<Task_Ship_MoveToLocation>(brainKey, targetLocKey, 0.1f)
								//})
							),
							new_sp<Decorator_Aborting_Is<MentalState_Fighter>>("dec_wander_state", stateKey, OP::EQUAL, MentalState_Fighter::WANDER, AbortPreference::ABORT_ON_MODIFY,
								new_sp<Sequence>("Sequence_MoveToNewLocation", MakeChildren{
									new_sp<Task_FindRandomLocationNearby>(wanderLocKey, originKey, 400.0f),
									new_sp<Task_Ship_MoveToLocation>(brainKey, wanderLocKey, 45.0f)
								})
							)
						})
					)
				))),
				MemoryInitializer
				{
					{ stateKey, new_sp<PrimitiveWrapper<MentalState_Fighter>>(MentalState_Fighter::WANDER)},
					{ brainKey, sp_this() },
					{ originKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ wanderLocKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ targetLocKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ targetKey, sp<WorldEntity>(nullptr) },
					{ secondaryTargetsKey, new_sp<PrimitiveWrapper<std::vector<sp<WorldEntity>>>>(std::vector<sp<WorldEntity>>{}) } //#TODO perhaps should be releasing pointers instead of shared ptr when those are a thing
				}
			);
	}


}

