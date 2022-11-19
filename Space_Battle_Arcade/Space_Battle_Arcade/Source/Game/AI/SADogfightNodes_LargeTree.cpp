#include "SADogfightNodes_LargeTree.h"
#include "GameFramework/SALog.h"
#include "GameFramework/SAGameBase.h"
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "SAShipAIBrain.h"
#include "../SAShip.h"
#include "SAShipBehaviorTreeNodes.h"
#include "../../GameFramework/SADebugRenderSystem.h"

/*
DEV NOTES

I believe one way to acheive extensible dog fight behavior is through a combo system. A ship will be put into a situation that will
cause it to generate a plan/manuever to get out of that situation. It will express this plan as a combo of actions that depend on how
the ship is facing the current target (is target behind myShip, infront of myShip, are we facing each other, or are we opposing each other.
Different plans will composed of different combo objects. There should be a fail safe abort if things are not going to to plan (such as we
never started facing the target) where combos time out and a new one is generated.

I'm considering two approaches. One approach creates indefinite tasks that are broken with aborts. The other is a more traditional behavior
tree where the tasks are pushed and popped very frequently. 

//////////////////////////////////////////////////////////////
//The nontraditional static nodes that tick and are aborted is something like the following:
//////////////////////////////////////////////////////////////
	is_targeting_each_other
		service_driver_with_avoidance (this tick will read memory, but that can be optimized away with caching intermediate read)
		loop
			selector
				is_behind_with_abort
					process_behind
				is_front_with_abort
					process_front
				is_facing_with_abort
					process_facing
				is_opposing_with_abort
					process_opposing
	memory{
		currentComboList 
		currentComboTimeout 
	}
//////////////////////////////////////////////////////////////
//The traditional approach is something like this
//////////////////////////////////////////////////////////////
	is_targeting_each_other
		loop
			sequence
				selector
					is_target_is_behind_me
						selector
							process_combo_mutator<behind>
							start_avoidance_combo
							default_driver_action<behind>
					is_target_in_front_of_me
						selector
							process_combo_mutator<front>
							start_attack_combo
							default_driver_action<front>
					is_facing_each_other
						selector
							process_combo_mutator<facing>
							default_driver_action<facing>
					is_facing_away_each_other
						selector
							process_combo_mutator<opposing>
							default_driver_action<opposing>
				driveShip_UnderAttackConditions
				sleep_this_frame

	memory{
		currentComboList :
		currentComboTimeout :
	}
////////////////////////////////////////////////////////////////

Each approach has its own issues. For example, if a service is what drives the ship, then the order of its tick isn't determined and may come before
the behavior tree updates the memory that should be informing how the ship should be driven. That is avoided with explicit ordered tasks

The task based approach means a lot of memory will be read each frame, which will cause a lot of dynamic casting if memory is safely read. 
*/



namespace SA
{
	namespace BehaviorTree
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ComboStepBase  - The base class for any custom combo step
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//void ComboStepBase::init()
		//{
		//	onInit();
		//}

		bool ComboStepBase_VerboseTree::isInPhase(TargetIs arrangement) const
		{
			for(TargetIs targetPhase : targetPhases)
			{
				if (targetPhase == arrangement) return true;
			}
			return false;
		}

		void ComboStepBase_VerboseTree::updateTimeAlive(float dt_sec, TargetIs arrangement)
		{
			basePOD.totalActiveTime += dt_sec;
			if (isInPhase(arrangement))
			{
				basePOD.timeInWrongPhase = 0.f;
			}
			else
			{
				basePOD.timeInWrongPhase += dt_sec;
			}
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Combo list
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		sp<SA::BehaviorTree::ComboStepBase_VerboseTree> ComboList::nullopt = sp<SA::BehaviorTree::ComboStepBase_VerboseTree>(nullptr);

		void ComboList::clear()
		{
			for (sp<ComboStepBase_VerboseTree>& step : steps)
			{
				//hmm, how to make it return to typed pools without requiring user implement virtual in each subclass.
				//#TODO
				//step->returnToPool();
			}
			steps.clear();
			activeIdx = 0;
		}


		void ComboList::addStep(const sp<ComboStepBase_VerboseTree>& nextStep)
		{
			steps.push_back(nextStep);
		}

		void ComboList::advanceStep()
		{
			activeIdx += 1;
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// TargetDependentTask
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void DogfightTaskBase::handleTargetReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue)
		{
			updateTargetPointer();
		}

		void DogfightTaskBase::updateTargetPointer()
		{
			using namespace BehaviorTree;
			Memory& memory = getMemory();
			{
				ScopedUpdateNotifier<TargetType> target_writable;
				if (memory.getWriteValueAs(target_Key, target_writable))
				{
					TargetType& targetRef = target_writable.get();

					//this is kinda slow, but we need it as a ship to get the forward directions. Probably should refactor the
					//methods that get forward vectors to be part of a gameplay component so it can by quickly reached.
					myTarget = std::dynamic_pointer_cast<Ship>(targetRef.requestReference().lock());
				}
				else
				{
					myTarget = nullptr;
				}
			}
		}

		void DogfightTaskBase::updateMyShipPtr()
		{
			if (myBrain)
			{
				wp<Ship> myWeakShip = myBrain->getWeakControlledTarget();
				if (!myWeakShip.expired())
				{
					myShip = myWeakShip.lock();
				}
			}
		}

		void DogfightTaskBase::notifyTreeEstablished()
		{
			Task::notifyTreeEstablished();

			Memory& memory = getMemory();
			{
				ScopedUpdateNotifier<sp<TargetIs>> targetOrientation_writable;
				ScopedUpdateNotifier<ShipAIBrain> brain_writable;

				//These memory values should never change for lifetime of the tree, if they do then that is an error in design
				//Thus, it is an invariant that these memory keys exist.
				if (memory.getWriteValueAs(brain_Key, brain_writable)
					&& memory.getWriteValueAs(targetArrangement_Key, targetOrientation_writable)
					)
				{
					sp<TargetIs>& targetOriPtr = targetOrientation_writable.get();
					targetArrangment = targetOriPtr;

					sp<ShipAIBrain> brainPtr = std::static_pointer_cast<ShipAIBrain>(brain_writable.get().requestReference().lock());
					myBrain = brainPtr;

					updateMyShipPtr();
					assert(bool(myShip));
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
					return;
				}
			}

			memory.getReplacedDelegate(target_Key).addWeakObj(sp_this(), &DogfightTaskBase::handleTargetReplaced);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task_ComboProcessorBase
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		void Task_ComboProcessor::notifyTreeEstablished()
		{
			DogfightTaskBase::notifyTreeEstablished();

			using namespace BehaviorTree;
			Memory& memory = getMemory();
			{
				ScopedUpdateNotifier<sp<ShipInputRequest>> inputRequest_writable;
				if (memory.getWriteValueAs(inputRequestKey, inputRequest_writable))
				{
					sp<ShipInputRequest>& inputRequestPtr = inputRequest_writable.get();
					inputRequestLoc = inputRequestPtr;
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
				}
			}
		}

		void Task_ComboProcessor::beginTask()
		{
			using namespace BehaviorTree;

			evaluationResult = false;

			if (!comboList)
			{
				Memory& memory = getMemory();
				{
					ScopedUpdateNotifier<PrimitiveWrapper<sp<ComboList>>> ComboList_writable;
					if (memory.getWriteValueAs(comboListKey, ComboList_writable))
					{
						sp<ComboList>& comboListPtr = ComboList_writable.get().value;
						comboList = comboListPtr;
					}
					else
					{
						log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
						return;
					}
				}
			}

			float dt_sec = getTree().getFrameDeltaTimeSecs();

			assert(comboList);
			if (comboList)
			{
				ComboStepBase_VerboseTree* currentStep = comboList->currentStep().get();
				ComboStepBase_VerboseTree* nextStep = comboList->nextStep().get();
				if (currentStep)
				{
					currentStep->updateTimeAlive(dt_sec, arrangement);
					if (nextStep && nextStep->isInPhase(arrangement) && !currentStep->inGracePeriod())
					{
						comboList->advanceStep();
						nextStep->doAction(dt_sec, arrangement, myTarget, myShip, inputRequestLoc);
						evaluationResult = true;
					}
					else if (currentStep->inGracePeriod())
					{
						currentStep->doAction(dt_sec, arrangement, myTarget, myShip, inputRequestLoc);
						evaluationResult = true;
					}
					else //all steps are done
					{
						comboList->clear();
						evaluationResult = false; //use false so this node isn't selected -- it did not do anything.
					}
				}
				else
				{
					//no current step, combo is either complete or no combo is generated yet
					evaluationResult = false;
				}
			}
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task_StartAvoidanceCombo
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void Task_StartDefenseCombo::beginTask()
		{
			evaluationResult = false;

			if (!comboList)
			{
				Memory& memory = getMemory();
				{
					ScopedUpdateNotifier<PrimitiveWrapper<sp<ComboList>>> ComboList_writable;
					if (memory.getWriteValueAs(comboList_Key, ComboList_writable))
					{
						sp<ComboList>& comboListPtr = ComboList_writable.get().value;
						comboList = comboListPtr;
					}
					else
					{
						log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
						return;
					}
				}
			}

			comboList->clear();
		
			int choice = rng->getInt(0, 0);
			if (choice == 0)
			{
				comboList->addStep(new_sp<SharpTurnStep_VerboseTree>(Phases{ TargetIs::BEHIND}));
				comboList->addStep(new_sp<SlowWhenFacingStep_VerboseTree>(Phases{ TargetIs::FACING }));
				comboList->addStep(new_sp<SharpTurnStep_VerboseTree>(Phases{ TargetIs::OPPOSING, TargetIs::BEHIND }));
				comboList->addStep(new_sp<CompleteStep_VerboseTree>(Phases{ TargetIs::INFRONT }));
				evaluationResult = true;
			}
		}

		void Task_StartDefenseCombo::postConstruct()
		{
			rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task_DefaultDogfightInput
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void Task_DefaultDogfightInput::notifyTreeEstablished()
		{
			using namespace BehaviorTree;
			DogfightTaskBase::notifyTreeEstablished();

			Memory& memory = getMemory();
			{
				ScopedUpdateNotifier<sp<ShipInputRequest>> inputRequests_writable;
				if (memory.getWriteValueAs(inputRequest_key, inputRequests_writable))
				{
					sp<ShipInputRequest>& inputRequestsPtr = inputRequests_writable.get();
					inputRequest = inputRequestsPtr;
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
				}
			}
		}

		void Task_DefaultDogfightInput::beginTask()
		{
			evaluationResult = false;

			if (inputRequest)
			{
				if (myTarget && myShip)
				{
					ShipInputRequest ir;
					ir.targetPoint = myTarget->getWorldPosition();
					*inputRequest = ir;
					evaluationResult = true;
				}
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "required memory not available! no input request container");
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task apply input requests
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		void Task_ApplyInputRequest::notifyTreeEstablished()
		{
			DogfightTaskBase::notifyTreeEstablished();

			using namespace BehaviorTree;
			Memory& memory = getMemory();
			{
				ScopedUpdateNotifier<sp<ShipInputRequest>> inputRequests_writable;
				if (memory.getWriteValueAs(inputRequests_key, inputRequests_writable))
				{
					sp<ShipInputRequest>& inputRequestsPtr = inputRequests_writable.get();
					inputRequest = inputRequestsPtr;
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
				}
			}
			
		}

		void Task_ApplyInputRequest::beginTask()
		{
			evaluationResult = false;

			if (inputRequest )
			{
				if (myShip)
				{
					float dt_sec = getTree().getFrameDeltaTimeSecs();

					if (inputRequest->targetPoint.has_value())
					{
						myShip->moveTowardsPoint(
							*(inputRequest->targetPoint),
							dt_sec,
							inputRequest->speedAdjustment.value_or(1.0f),
							!inputRequest->rollOverride_rad.has_value()
						);
					}

					if (inputRequest->rollOverride_rad.has_value())
					{
						myShip->roll(*inputRequest->rollOverride_rad, dt_sec);
					}

					evaluationResult = true;

					if constexpr (ENABLE_DEBUG_LINES)
					{
						static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
						if (inputRequest->targetPoint.has_value())
						{
							debug.renderLine(glm::vec4(myShip->getWorldPosition(), 1), glm::vec4(*inputRequest->targetPoint, 1), inputRequest->debugColor);
						}
					}
				}
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "required memory not available! no input request container");
			}

			//clear previous request.
			*inputRequest = ShipInputRequest{};
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task_CalculateTargetArrangment
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void Task_CalculateTargetArrangment::beginTask()
		{
			using namespace glm;
			evaluationResult = false;

			if (!targetArrangment || !myBrain)
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
				return;
			}
			
			if (!myTarget) updateTargetPointer();
			if (!myShip) myShip = myBrain->getWeakControlledTarget(); //slightly unsafe, if a brain can ever posses a different target; but currently that isn't the case and I don't plane to make it that way. Design currently somewhat enforces a single brain to ship behavior.
			if (myTarget && myShip)
			{
				vec3 tForward_n = vec3(myTarget->getForwardDir());
				vec3 mForward_n = vec3(myShip->getForwardDir());
				vec3 tarPos = myTarget->getWorldPosition();
				vec3 myPos = myShip->getWorldPosition();

				//no need to normalize as we only care about sign of dot products
				vec3 toMe = myPos - tarPos;
				vec3 toTarget = -(toMe);

				bool imBehindTarget = glm::dot(toMe, tForward_n) < 0.f;
				bool targetBehindMe = glm::dot(toTarget, mForward_n) < 0.f;
				
				TargetIs arrangment;
				if (imBehindTarget)
				{
					if (targetBehindMe) //  <t ------- m>
					{
						arrangment = TargetIs::OPPOSING;
					}
					else				// <t ------- <m
					{
						arrangment = TargetIs::INFRONT;
					}
				}
				else //target is facing me
				{
					if (targetBehindMe) // t> ------ m>
					{
						arrangment = TargetIs::BEHIND;
					}
					else				// t> ------- <m
					{
						arrangment = TargetIs::FACING;
					}
				}

				if (*targetArrangment != arrangment)
				{
					*targetArrangment = arrangment;
				}
				evaluationResult = true;
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Combo building blocks
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		void SharpTurnStep_VerboseTree::doAction(float dt_sec, TargetIs arrangement, const fwp<Ship>& myTarget, const fwp<Ship>& myShip, const fwp<ShipInputRequest>& inputRequestLoc)
		{
			if (myTarget && myShip && inputRequestLoc)
			{
				inputRequestLoc->speedAdjustment = 0.5f;
				inputRequestLoc->targetPoint = myTarget->getWorldPosition();
				inputRequestLoc->debugColor = glm::vec3(0.6, 0.7, 0.3);
			}
			else
			{
				//do nothing? something has went wrong, this should never really happen given conditions of the behavior tree
			}
		}


		void SlowWhenFacingStep_VerboseTree::doAction(float dt_sec, TargetIs arrangement, const fwp<Ship>& myTarget, const fwp<Ship>& myShip, const fwp<ShipInputRequest>& inputRequestLoc)
		{
			using namespace glm;
			static vec3 debugColor = vec3(0.81f, 0.92f, 0.81f);

			if (myTarget && myShip && inputRequestLoc)
			{
				if (arrangement == TargetIs::FACING)
				{
					vec3 toTarget = myTarget->getWorldPosition() - myShip->getWorldPosition();
					float lengthToTarget2 = glm::length2(toTarget);
					if (lengthToTarget2 > slowdownDist2)
					{
						inputRequestLoc->speedAdjustment = 1.0f;
						inputRequestLoc->targetPoint = myTarget->getWorldPosition();
						inputRequestLoc->debugColor = debugColor;
					}
					else
					{
						//don't roll to stablize the right vector
						inputRequestLoc->speedAdjustment = 0.5f;
						inputRequestLoc->targetPoint = myTarget->getWorldPosition() + (vec3(myShip->getRightDir()) * 5.0f);
						inputRequestLoc->bDisableRoll = true;
						inputRequestLoc->debugColor = debugColor * 0.5f;
					}
				}
				else
				{
					//continue with previous velocity
				}
			}
			else
			{
				//do nothing? something has went wrong, this should never really happen given conditions of the behavior tree
			}
		}

	}
}
