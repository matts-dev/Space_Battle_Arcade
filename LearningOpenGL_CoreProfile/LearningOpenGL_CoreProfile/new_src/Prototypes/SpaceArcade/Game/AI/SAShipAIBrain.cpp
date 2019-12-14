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
#include "SADogfightNodes_LargeTree.h"

namespace SA
{

	ShipAIBrain::ShipAIBrain(const sp<Ship>& controlledShip)
	{
		this->controlledTarget = controlledShip;
	}

	bool ShipAIBrain::onAwaken()
	{
		if (BehaviorTreeBrain::onAwaken())
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
		return false;
	}

	void ShipAIBrain::onSleep()
	{
		BehaviorTreeBrain::onSleep();
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

	////////////////////////////////////////////////////////
	// ContinuousFireBrain
	////////////////////////////////////////////////////////

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

	void ContinuousFireBrain::postConstruct()
	{
		ShipAIBrain::postConstruct();

		//dummy brain
		using namespace BehaviorTree;
		behaviorTree = new_sp<Tree>("dummy-tree",
			new_sp<Task_Empty>("empty_task"),
			MemoryInitializer{}
		);
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
				myShip->setMaxSpeed(speed);
				myShip->setVelocityDir(rotDir);
			}

			return true;
		}
		return false;
	}

	void FlyInDirectionBrain::postConstruct()
	{
		ShipAIBrain::postConstruct();
		//dummy brain
		using namespace BehaviorTree;
		behaviorTree = new_sp<Tree>("dummy-tree",
			new_sp<Task_Empty>("empty_task"),
			MemoryInitializer{}
		);
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// behavior tree brain
	/////////////////////////////////////////////////////////////////////////////////////


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
	// Evade test brain
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void EvadeTestBrain::postConstruct()
	{
		const char* const brainKey = "selfBrain";
		const char* const stateKey = "stateKey";
		const char* const originKey = "origin";
		const char* const wanderLocKey = "wander_loc";
		const char* const targetKey = "target";
		const char* const targetLocKey = "target_loc";
		const char* const secondaryTargetsKey = "secondaryTargetsKeys";
		const char* const activeAttackers_MemoryKey = "activeAttackersKey";
		const char* const dogFightLoc_Key = "dogFightLocKey";


		using namespace BehaviorTree;
		behaviorTree =
			new_sp<Tree>("fighter-tree-root",
				new_sp<Service_TargetFinder>("service_targetFinder", 1.0f, true, brainKey, targetKey,
					new_sp<Loop>("fighter-inf-loop", 0,
						new_sp<Selector>("state_selector", MakeChildren{
							new_sp<Decorator_Aborting_Is<MentalState_Fighter>>("dec_evade_state", stateKey, OP::EQUAL, MentalState_Fighter::EVADE, AbortPreference::ABORT_ON_MODIFY,
								new_sp<Sequence>("EvadeToDogfight", MakeChildren{
									new_sp<Task_FindDogfightLocation>("task_findDFLoc", dogFightLoc_Key, brainKey),
									new_sp<Random>("RandomSelector",
										Chances{
											{"spiral_evade", 1},
											{"spiral_spin", 1}
										},
										MakeChildren{
											new_sp<Task_EvadePatternSpiral>("spiral_evade", dogFightLoc_Key, brainKey, 15.0f),
											new_sp<Task_EvadePatternSpin>("spiral_spin", dogFightLoc_Key, brainKey, 15.0f)
										}
									)
								})
							),
						})
					)
				),
				MemoryInitializer
				{
					{ stateKey, new_sp<PrimitiveWrapper<MentalState_Fighter>>(MentalState_Fighter::EVADE)},
					{ brainKey, sp_this() },
					{ originKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ wanderLocKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ dogFightLoc_Key, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ targetLocKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ targetKey, sp<WorldEntity>(nullptr) },
					{ activeAttackers_MemoryKey, new_sp<PrimitiveWrapper<ActiveAttackers>>(ActiveAttackers{})},
					{ secondaryTargetsKey, new_sp<PrimitiveWrapper<std::vector<sp<WorldEntity>>>>(std::vector<sp<WorldEntity>>{}) } //#TODO perhaps should be releasing pointers instead of shared ptr when those are a thing
				}
			);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// dogfight test brain - large tree
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void DogfightTestBrain_VerboseTree::postConstruct()
	{
		const char* const brainKey = "selfBrain";
		const char* const stateKey = "stateKey";
		const char* const originKey = "origin";
		const char* const wanderLocKey = "wander_loc";
		const char* const targetKey = "target";
		const char* const targetLocKey = "target_loc";
		const char* const secondaryTargetsKey = "secondaryTargetsKeys";
		const char* const activeAttackers_MemoryKey = "activeAttackersKey";
		const char* const dogFightLoc_Key = "dogFightLocKey";

		const char* const positionArrangementKey = "posPhaseKey";
		const char* const comboListKey = "comboList";
		const char* const dogfightInputHandler= "dfShipDriver";
		const char* const inputRequestsKey = "inputRequests";

		using namespace BehaviorTree;
		behaviorTree =
			new_sp<Tree>("fighter-tree-root",
				new_sp<Service_TargetFinder>("service_targetFinder", 1.0f, true, brainKey, targetKey,
					new_sp<Loop>("fighter-inf-loop", 0,
						new_sp<Selector>("state_selector", MakeChildren{
							new_sp<Decorator_Aborting_Is<MentalState_Fighter>>("dec_fight_state", stateKey, OP::EQUAL, MentalState_Fighter::ATTACK, AbortPreference::ABORT_ON_MODIFY,
								new_sp<Loop>("dogfight_loop", 0, 
									new_sp<Sequence>("Seq_Dogfight", MakeChildren{
										new_sp<Task_CalculateTargetArrangment>("Task_CalculateTargetArrangment", positionArrangementKey, brainKey, targetKey),
										new_sp<Selector>("Selector_TargetArrangement", MakeChildren{
											new_sp<Decorator_TargetIs>("dec_BehindTarget", positionArrangementKey, TargetIs::BEHIND,
												new_sp<Selector>("sel_WhatToDo_BehindTarget", MakeChildren{
													new_sp<Task_ComboProcessor>("task_ProcCombo_Behind", comboListKey, inputRequestsKey, TargetIs::BEHIND, positionArrangementKey, brainKey, targetKey),
													new_sp<Task_StartDefenseCombo>("task_startAvoidanceCombo", comboListKey),
													new_sp<Task_DefaultDogfightInput>("Task_DefaultDogfightInput", inputRequestsKey, positionArrangementKey, brainKey, targetKey)
												})
											),
											new_sp<Decorator_TargetIs>("dec_TargetInFront", positionArrangementKey, TargetIs::INFRONT,
												new_sp<Selector>("sel_WhatToDo_TargetInFront", MakeChildren{
													new_sp<Task_ComboProcessor>("task_ProcCombo_Front", comboListKey, inputRequestsKey, TargetIs::INFRONT, positionArrangementKey, brainKey, targetKey),
													new_sp<Task_DefaultDogfightInput>("Task_DefaultDogfightInput", inputRequestsKey, positionArrangementKey, brainKey, targetKey)
												})
											),
											new_sp<Decorator_TargetIs>("dec_FacingTarget", positionArrangementKey, TargetIs::FACING,
												new_sp<Selector>("sel_WhatToDo_IsFacingTarget", MakeChildren{
													new_sp<Task_ComboProcessor>("task_ProcCombo_Facing", comboListKey, inputRequestsKey, TargetIs::FACING, positionArrangementKey, brainKey, targetKey),
													new_sp<Task_DefaultDogfightInput>("Task_DefaultDogfightInput", inputRequestsKey, positionArrangementKey, brainKey, targetKey)
												})
											),
											new_sp<Decorator_TargetIs>("dec_TargetInOpposing", positionArrangementKey, TargetIs::OPPOSING,
												new_sp<Selector>("sel_WhatToDo_TargetOpposing", MakeChildren{
													new_sp<Task_ComboProcessor>("task_ProcCombo_Opposing", comboListKey, inputRequestsKey, TargetIs::OPPOSING, positionArrangementKey, brainKey, targetKey),
													new_sp<Task_DefaultDogfightInput>("Task_DefaultDogfightInput", inputRequestsKey, positionArrangementKey, brainKey, targetKey)
												})
											),
										}),
										new_sp<Task_ApplyInputRequest>("Task_ApplyInputRequest", positionArrangementKey, brainKey, targetKey, inputRequestsKey),
										new_sp<Task_WaitForNextFrame>("Task_SleepFrame")
									})
								)
							),
						})
					)
				),
				MemoryInitializer
				{
					{ stateKey, new_sp<PrimitiveWrapper<MentalState_Fighter>>(MentalState_Fighter::ATTACK)},
					{ brainKey, sp_this() },
					{ originKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ wanderLocKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ dogFightLoc_Key, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ targetLocKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ targetKey, sp<WorldEntity>(nullptr) },
					{ activeAttackers_MemoryKey, new_sp<PrimitiveWrapper<ActiveAttackers>>(ActiveAttackers{})},
					{ secondaryTargetsKey, new_sp<PrimitiveWrapper<std::vector<sp<WorldEntity>>>>(std::vector<sp<WorldEntity>>{}) },

					//these use shared pointers so they can be cached to bypass update event notifications for efficiency, perhaps this should be a supported feature
					{ positionArrangementKey,	new_sp<PrimitiveWrapper<sp<TargetIs>>>(new_sp<TargetIs>(TargetIs::BEHIND)) },
					{ comboListKey,				new_sp<PrimitiveWrapper<sp<ComboList>>>(new_sp<ComboList>()) },
					{ inputRequestsKey,			new_sp<PrimitiveWrapper<sp<ShipInputRequest>>>(new_sp<ShipInputRequest>()) }

				}
			);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// dogfight test brain - tree with ticking nodes
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void DogfightTestBrain::postConstruct()
	{
		const char* const brainKey = "selfBrain";
		const char* const stateKey = "stateKey";
		const char* const originKey = "origin";
		const char* const wanderLocKey = "wander_loc";
		const char* const targetKey = "target";
		const char* const targetLocKey = "target_loc";
		const char* const secondaryTargetsKey = "secondaryTargetsKeys";
		const char* const activeAttackers_MemoryKey = "activeAttackersKey";
		const char* const dogFightLoc_Key = "dogFightLocKey";

		const char* const positionArrangementKey = "posPhaseKey";
		const char* const comboListKey = "comboList";
		const char* const dogfightInputHandler= "dfShipDriver";
		const char* const inputRequestsKey = "inputRequests";

		using namespace BehaviorTree;
		behaviorTree =
			new_sp<Tree>("fighter-tree-root",
				new_sp<Service_TargetFinder>("service_targetFinder", 1.0f, true, brainKey, targetKey,
					new_sp<Loop>("fighter-inf-loop", 0,
						new_sp<Selector>("state_selector", MakeChildren{
							new_sp<Decorator_Aborting_Is<MentalState_Fighter>>("dec_fight_state", stateKey, OP::EQUAL, MentalState_Fighter::ATTACK, AbortPreference::ABORT_ON_MODIFY,
								new_sp<Task_DogfightNode>("Task_Dogfight", brainKey, targetKey, secondaryTargetsKey)
							),
						})
					)
				),
				MemoryInitializer
				{
					{ stateKey, new_sp<PrimitiveWrapper<MentalState_Fighter>>(MentalState_Fighter::ATTACK)},
					{ brainKey, sp_this() },
					{ originKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ wanderLocKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ dogFightLoc_Key, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ targetLocKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ targetKey, sp<WorldEntity>(nullptr) },
					{ activeAttackers_MemoryKey, new_sp<PrimitiveWrapper<ActiveAttackers>>(ActiveAttackers{})},
					{ secondaryTargetsKey, new_sp<PrimitiveWrapper<std::vector<sp<WorldEntity>>>>(std::vector<sp<WorldEntity>>{}) },

					//these use shared pointers so they can be cached to bypass update event notifications for efficiency, perhaps this should be a supported feature
					{ positionArrangementKey,	new_sp<PrimitiveWrapper<sp<TargetIs>>>(new_sp<TargetIs>(TargetIs::BEHIND)) },
					{ comboListKey,				new_sp<PrimitiveWrapper<sp<ComboList>>>(new_sp<ComboList>()) },
					{ inputRequestsKey,			new_sp<PrimitiveWrapper<sp<ShipInputRequest>>>(new_sp<ShipInputRequest>()) }

				}
			);
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Fighter Brain
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void FighterBrain::postConstruct()
	{
		const char* const brainKey = "selfBrain";
		const char* const stateKey = "stateKey";
		const char* const originKey = "origin";
		const char* const wanderLocKey = "wander_loc";
		const char* const targetKey = "target";
		const char* const targetLocKey = "target_loc";
		const char* const secondaryTargetsKey = "secondaryTargetsKeys";
		const char* const activeAttackers_MemoryKey = "activeAttackersKey";
		const char* const dogFightLoc_Key = "dogFightLocKey";


		using namespace BehaviorTree;
		behaviorTree =
			new_sp<Tree>("fighter-tree-root",
				new_sp<Decorator_FighterStateSetter>("decor_state_setter", stateKey, targetKey, activeAttackers_MemoryKey,
				new_sp<Service_TargetFinder>("service_targetFinder", 1.0f, true, brainKey, targetKey,
					new_sp<Service_OpportunisiticShots>("service_opportunisiticShots", 0.1f, true, brainKey, targetKey, secondaryTargetsKey, stateKey,
					new_sp<Loop>("fighter-inf-loop", 0,
						new_sp<Selector>("state_selector", MakeChildren{
							new_sp<Decorator_Aborting_Is<MentalState_Fighter>>("dec_evade_state", stateKey, OP::EQUAL, MentalState_Fighter::EVADE, AbortPreference::ABORT_ON_MODIFY,
								new_sp<Sequence>("EvadeToDogfight", MakeChildren{
									new_sp<Task_FindDogfightLocation>("task_findDFLoc", dogFightLoc_Key, brainKey),
									new_sp<Loop>("evade-loop", 0,
										new_sp<Random>("RandomSelector",
											Chances{
												{"spiral_evade", 1},
												{"spiral_spin", 1}
											},
											MakeChildren{
												new_sp<Task_EvadePatternSpiral>("spiral_evade", dogFightLoc_Key, brainKey, 5.0f),
												new_sp<Task_EvadePatternSpin>("spiral_spin", dogFightLoc_Key, brainKey, 5.0f)
											}
										)
									)
								})
							),
							new_sp<Decorator_Aborting_Is<MentalState_Fighter>>("dec_attack_state", stateKey, OP::EQUAL, MentalState_Fighter::ATTACK, AbortPreference::ABORT_ON_MODIFY,
								new_sp<Task_Ship_FollowTarget_Indefinitely>("task_followTarget", brainKey, targetKey, activeAttackers_MemoryKey)
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
					{ dogFightLoc_Key, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ targetLocKey, new_sp<PrimitiveWrapper<glm::vec3>>(glm::vec3{0,0,0}) },
					{ targetKey, sp<WorldEntity>(nullptr) },
					{ activeAttackers_MemoryKey, new_sp<PrimitiveWrapper<ActiveAttackers>>(ActiveAttackers{})},
					{ secondaryTargetsKey, new_sp<PrimitiveWrapper<std::vector<sp<WorldEntity>>>>(std::vector<sp<WorldEntity>>{}) } //#TODO perhaps should be releasing pointers instead of shared ptr when those are a thing
				}
			);
	}



}

