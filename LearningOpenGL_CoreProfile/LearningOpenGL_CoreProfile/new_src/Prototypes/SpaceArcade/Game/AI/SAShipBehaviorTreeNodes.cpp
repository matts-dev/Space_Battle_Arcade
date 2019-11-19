#include "SAShipBehaviorTreeNodes.h"
#include "SAShipAIBrain.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../SAShip.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "../../Tools/SAUtilities.h"
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../../GameFramework/SALevel.h"
#include <gtc/quaternion.hpp>
#include "../../GameFramework/SADebugRenderSystem.h"
#include "../../GameFramework/Components/GameplayComponents.h"
#include "../../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../Levels/SASpaceLevelBase.h"
#include "../Team/Commanders.h"
#include <assert.h>
//#include "../../GameFramework/SAGameBase.h"
//#include "../../GameFramework/SALevelSystem.h"

namespace SA
{
	namespace BehaviorTree
	{
		void LogShipNodeDebugMessage(const Tree& tree, const NodeBase& node, const std::string& msg)
		{
			std::string treeInstance = std::to_string((uint64_t)&tree);
			std::string newMessage = treeInstance + " : " + node.getName() + "\t - " + msg;
			log("ShipNodeDebugMessage", LogLevel::LOG, newMessage.c_str());
		}


		/////////////////////////////////////////////////////////////////////////////////////
		// Task find random location within radius of location
		/////////////////////////////////////////////////////////////////////////////////////
		void Task_FindRandomLocationNearby::postConstruct()
		{
			static RNGSystem& rngSystem = GameBase::get().getRNGSystem();
			//rng = rngSystem.getNamedRNG(typeid(*this).name());
			rng = rngSystem.getTimeInfluencedRNG();
		}

		void Task_FindRandomLocationNearby::beginTask()
		{
			using namespace glm;

			Memory& memory = getMemory();
			const vec3* startLocation = memory.getReadValueAs<vec3>(inputLocation_MemoryKey);
			BehaviorTree::ScopedUpdateNotifier<vec3> writeValue;

			if (startLocation && memory.getWriteValueAs<vec3>(outputLocation_MemoryKey, writeValue))
			{
				float randomRadius = radius * rng->getFloat(0.01f, 1.f);

				glm::vec3 offsetDir(1, 0, 0);
				//rotating around sphere ensures this will be a normal vector after rotation
				float zRotDeg = rng->getFloat(0.f, 360.f);
				float yRotDeg = rng->getFloat(0.f, 360.f);

				glm::quat rot = glm::angleAxis(Utils::DEG_TO_RADIAN_F * zRotDeg, glm::vec3(0.f, 0.f, 1.f));
				rot = glm::angleAxis(Utils::DEG_TO_RADIAN_F * yRotDeg, glm::vec3(0.f, 1.f, 0.f)) * rot;
				mat3 rotMat = glm::toMat3(rot); //no translation, so no need for mat4

				offsetDir = rotMat * offsetDir;
				offsetDir *= randomRadius;

				glm::vec3 randomLocation = offsetDir + (*startLocation);
				writeValue.get() = randomLocation;
				evaluationResult = true;
				return; //early out to prevent hitting fall through false
			} 
			else //one or more memory locations could not be obtained
			{ 
				SA::log("Task_FindRandomLocationNearby", LogLevel::LOG_ERROR, "Failed to get memory location!"); 
			}

			evaluationResult = false;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Saves the current ship location to the behavior tree memory so more generic nodes
		// can use the location.
		/////////////////////////////////////////////////////////////////////////////////////
		void Task_Ship_SaveShipLocation::beginTask()
		{
			using namespace glm;

			Memory& memory = getMemory();

			if (const ShipAIBrain* shipBrain = memory.getReadValueAs<ShipAIBrain>(shipBrain_MemoryKey))
			{
				//get owning ship
				if(const Ship* ship = shipBrain->getControlledTarget())
				{
					ScopedUpdateNotifier<vec3> writeValue_location;
					if (memory.getWriteValueAs(outputLocation_MemoryKey, writeValue_location))
					{
						writeValue_location.get() = ship->getTransform().position;
						evaluationResult = true;
					}
				}
			}

			if (!evaluationResult.has_value())
			{
				evaluationResult = false;
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// task move to location
		/////////////////////////////////////////////////////////////////////////////////////

		void Task_Ship_MoveToLocation::beginTask()
		{
			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				Memory& memory = getMemory();
				{
					ScopedUpdateNotifier<ShipAIBrain> brain_writable;
					if (memory.getWriteValueAs(shipBrain_MemoryKey, brain_writable))
					{
						wp<Ship> myWeakShip = brain_writable.get().getWeakControlledTarget();
						if (myWeakShip.expired())
						{
							log("Task_Ship_MoveToLocation", LogLevel::LOG_ERROR, "could not find controlled ship");
							evaluationResult = false;
							return; //don't start ticking
						}
						else
						{
							//keep the ship alive while this task is working
							myShip = myWeakShip.lock();
						}
					}

					if (const glm::vec3* targetLoc = memory.getReadValueAs<glm::vec3>(targetLoc_MemoryKey))
					{
						moveLoc = *targetLoc;
					}
					else
					{
						evaluationResult = false;
						return; //early out, cannot get the target location
					}
				}
				currentLevel->getWorldTimeManager()->registerTicker(sp_this());
				accumulatedTime = 0;
			}
			else
			{
				log("Task_Ship_MoveToLocation", LogLevel::LOG_ERROR, "could not get current level");
				evaluationResult = false;
			}
		}

		bool Task_Ship_MoveToLocation::tick(float dt_sec)
		{
			using namespace glm;

			if (myShip)
			{
				const Transform& xform = myShip->getTransform();
				accumulatedTime += dt_sec;

				vec3 targetDir = moveLoc - xform.position;
			
				if (glm::length2(targetDir) < atLocThresholdLength2)
				{
					evaluationResult = true;
				}
				else if (accumulatedTime >= timeoutSecs)
				{
					evaluationResult = false;
				}
				else
				{
					myShip->moveTowardsPoint(moveLoc, dt_sec);
				}

				if constexpr (ENABLE_DEBUG_LINES)
				{
					static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
					debug.renderLine(vec4(xform.position, 1), vec4(moveLoc, 1), vec4(0, 0.25f, 0, 1));
				}
			}

			return !evaluationResult.has_value();
		}


		void Task_Ship_MoveToLocation::taskCleanup()
		{
			//ensure this task does not keep the ship it controls alive.
			myShip = nullptr;

			//cancel ticker
			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				currentLevel->getWorldTimeManager()->removeTicker(sp_this());
			}
		}

		void Task_Ship_MoveToLocation::handleNodeAborted()
		{
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Service target finder
		/////////////////////////////////////////////////////////////////////////////////////

		void Service_TargetFinder::serviceTick()
		{
			//this service should always have a brain available while it is ticking; if it doesn't this is a design error.
			assert(owningBrain);
			Memory& memory = getMemory();
			if (!currentTarget)
			{
				tickFindNewTarget_slow();
			}
		}

		void Service_TargetFinder::startService()
		{
			Memory& memory = getMemory();
			owningBrain = memory.getReadValueAs<ShipAIBrain>(brainKey);
			memory.getModifiedDelegate(targetKey).addStrongObj(sp_this(), &Service_TargetFinder::handleTargetModified);

			resetSearchData();

			cachedPrefDist2 = preferredTargetMaxDistance * preferredTargetMaxDistance;
			if (owningBrain)
			{
				if (const Ship* myShip = owningBrain->getControlledTarget())
				{
					if(const TeamComponent* teamCom = myShip->getGameComponent<TeamComponent>())
					{
						cachedTeamIdx = teamCom->getTeam();
					}
				}
			}
		}

		void Service_TargetFinder::stopService()
		{
			owningBrain = nullptr;
			currentTarget.reset();
			getMemory().getModifiedDelegate(targetKey).removeStrong(sp_this(), &Service_TargetFinder::handleTargetModified);
		}

		void Service_TargetFinder::handleTargetModified(const std::string& key, const GameEntity* value)
		{
			if (value == nullptr)
			{
				currentTarget.reset();
				resetSearchData();
				tickFindNewTarget_slow();
			}
			else if (value != currentTarget.get())
			{
				Memory& memory = getMemory();

				ScopedUpdateNotifier<WorldEntity> outWriteAccess;
				if (memory.getWriteValueAs<WorldEntity>(targetKey, outWriteAccess))
				{
					//note: this is going to cause this handler to get re-triggered since we just flagged it as write. But none of the branches will be hit.
					//ATOW there is not an easy way to get a reference from a const game entity. When that becomes a thing,
					//this should use that. 

					//note this will only happen when target was set from outside this node
					//static cast is safe because above returned true, meaning it is at least the requested type
					currentTarget = std::static_pointer_cast<WorldEntity>(outWriteAccess.get().requestReference().lock()); 
				}
				else{log("Service_TargetFinder", LogLevel::LOG_ERROR, "detected target change from outside, but could not acquire handle to it.");}
			}
		}

		void Service_TargetFinder::handleTargetDestroyed(const sp<GameEntity>& entity)
		{
			currentTarget = nullptr;
		}

		void Service_TargetFinder::resetSearchData()
		{
			//currentSearchMethod = SearchMethod::NEARBY_HASH_CELLS;
			//currentSearchMethod = SearchMethod::LINEAR_SEARCH; 
			currentSearchMethod = SearchMethod::COMMANDER_ASSIGNED;
		}

		void Service_TargetFinder::tickFindNewTarget_slow()
		{
			//alternative: search around current cube for nearby enemies
			//alternative: ray trace out front to see if any enemies are in LOS
			//alternative draw debug shapes to visualize work this is doing

			using namespace glm;

			static LevelSystem& LevelSys = GameBase::get().getLevelSystem();
			const sp<LevelBase>& level = LevelSys.getCurrentLevel();
			const Ship* myShip = owningBrain->getControlledTarget();
			if (level && myShip && !currentTarget)
			{
				glm::vec3 myPos = myShip->getWorldPosition();

				sp<WorldEntity> bestSoFar = nullptr;
				std::optional<float> bestSoFarLen2;

				if (currentSearchMethod == SearchMethod::COMMANDER_ASSIGNED)
				{
					if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(level.get()))
					{
						if (TeamCommander* teamCommander = spaceLevel->getTeamCommander(cachedTeamIdx))
						{
							if (sp<WorldEntity> target = teamCommander->getTarget())
							{
								bestSoFar = target;
								setTarget(target);
							}
							else
							{
								//AdvanceState(); #TODO have this check other methods by switching the current search enum or some mechanism
							}
						}
					}
					else
					{
						log("Service_TargetFinder", LogLevel::LOG_ERROR, "Could not cast to correct level type");
					}
				}
				else if(currentSearchMethod == SearchMethod::NEARBY_HASH_CELLS)
				{
					SH::SpatialHashGrid<WorldEntity>& worldGrid = level->getWorldGrid();

					std::vector<sp<const SH::HashCell<WorldEntity>>> nearbyCells;
					glm::vec4 center = glm::vec4(myPos, 1.0f);
					float radius = 20.f;

					const std::array<glm::vec4, 8> worldSpaceBox =
					{
						center + vec4(-radius, radius, radius, 0),
						center + vec4(radius, radius, radius, 0),

						center + vec4(-radius, -radius, radius, 0),
						center + vec4(radius, -radius, radius, 0),

						center + vec4(-radius, radius, -radius, 0),
						center + vec4(radius, radius, -radius, 0),

						center + vec4(-radius, -radius, -radius, 0),
						center + vec4(radius, -radius, -radius, 0),
					};
					worldGrid.lookupCellsForOOB(worldSpaceBox, nearbyCells);

				}
				else if (currentSearchMethod == SearchMethod::LINEAR_SEARCH)
				{
					const std::set<sp<WorldEntity>>& worldEntities = level->getWorldEntities();
					for (const sp<WorldEntity>& entity : worldEntities)
					{
						if(TeamComponent* TeamCom = entity->getGameComponent<TeamComponent>())
						{
							if (cachedTeamIdx != TeamCom->getTeam())
							{
								glm::vec3 targetPos = entity->getWorldPosition();
								float len2 = glm::length2(targetPos - myPos);
							
								if (!bestSoFar)
								{
									bestSoFar = entity;
									bestSoFarLen2 = len2;
								}
								else if (len2 < bestSoFarLen2)
								{
									bestSoFar = entity;
									bestSoFarLen2 = len2;
								}
								if (len2 < cachedPrefDist2)
								{
									//accept this as target
									break;
								}
							}
						}
					}
				}

				if (bestSoFar)
				{
					setTarget(bestSoFar);

					if (BehaviorTree::ENABLE_DEBUG_LINES && DEBUG_TARGET_FINDER)
					{
						static DebugRenderSystem& debugRenderer = GameBase::get().getDebugRenderSystem();
						glm::vec3 targetPos = bestSoFar->getWorldPosition();

						debugRenderer.renderLineOverTime(myPos, targetPos, glm::vec3(0.66, 0.13, 0.74), 1.0f);
					}
				}
			}
		}

		void Service_TargetFinder::setTarget(const sp<WorldEntity>& target)
		{
			if (target == currentTarget)
			{
				return;
			}

			if (currentTarget)
			{
				//#TODO make this a releasing pointer when that gets implemented
				currentTarget->onDestroyedEvent->removeStrong(sp_this(), &Service_TargetFinder::handleTargetDestroyed);
			}
	
			currentTarget = target;

			//make sure write to memory happens AFTER updating current target. This is to prevent the handler for target modified from updating current target twice.
			getMemory().replaceValue(targetKey, currentTarget);

			target->onDestroyedEvent->addStrongObj(sp_this(), &Service_TargetFinder::handleTargetDestroyed);
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Service_OpportunisiticShots 
		/////////////////////////////////////////////////////////////////////////////////////

		void Service_OpportunisiticShots::startService()
		{
			Memory& memory = getMemory();
			owningBrain = memory.getReadValueAs<ShipAIBrain>(brainKey);

			primaryTarget = memory.getMemoryReference<TargetType>(targetKey);
			secondaryTargets = memory.getMemoryReference<const PrimitiveWrapper<std::vector<sp<TargetType>>>>(secondaryTargetsKey);
			
			memory.getModifiedDelegate(targetKey).addStrongObj(sp_this(), &Service_OpportunisiticShots::handleTargetModified);

			//don't listen to modified, just replaced. We only care when the underlying datastructure may have been swapped out.
			memory.getReplacedDelegate(secondaryTargetsKey).addStrongObj(sp_this(), &Service_OpportunisiticShots::handleSecondaryTargetsReplaced);

			if (!owningBrain) { log("Service_OpportunisiticShots", LogLevel::LOG_ERROR, "No owning brain"); exit(-1); }
			if (!secondaryTargets) { log("Service_OpportunisiticShots", LogLevel::LOG_ERROR, "No secondary target array"); exit(-1);}
		}

		void Service_OpportunisiticShots::handleTargetModified(const std::string& key, const GameEntity* value)
		{
			Memory& memory = getMemory();
			primaryTarget = memory.getMemoryReference<TargetType>(targetKey);
		}

		void Service_OpportunisiticShots::handleSecondaryTargetsReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue)
		{
			Memory& memory = getMemory();
			secondaryTargets = memory.getMemoryReference<const PrimitiveWrapper<std::vector<sp<TargetType>>>>(secondaryTargetsKey);
		}

		void Service_OpportunisiticShots::stopService()
		{
			owningBrain = nullptr;
			secondaryTargets = nullptr;
			primaryTarget = nullptr;

			Memory& memory = getMemory();
			memory.getModifiedDelegate(targetKey).removeStrong(sp_this(), &Service_OpportunisiticShots::handleTargetModified);
			memory.getReplacedDelegate(secondaryTargetsKey).removeStrong(sp_this(), &Service_OpportunisiticShots::handleSecondaryTargetsReplaced);
		}

		void Service_OpportunisiticShots::serviceTick()
		{
			using namespace glm;

			if (canShoot())
			{
				if (const Ship* myShip = owningBrain->getControlledTarget())
				{
					vec3 myPos = myShip->getWorldPosition();
					vec4 myForward = myShip->getForwardDir();

					if (primaryTarget )
					{
						bool firedAtTarget = tryShootAtTarget(*primaryTarget, myShip, myPos, myForward);
					}

					//note: secondary targets can be updated between ticks, so do not cache anything within it. It should always be non-null though.
					//assert(secondaryTargets);
					//if (secondaryTargets)
					//{
					//	//note: be careful not to modify this array indirectly while looping over it; modifications should be on same thread so that isn't a concern.
					//	for (const sp<TargetType>& otherTarget : *secondaryTargets)
					//	{

					//	}
					//}
				}
			}
		}

		void Service_OpportunisiticShots::postConstruct()
		{
			GameBase& game = GameBase::get();
			rng = game.getRNGSystem().getTimeInfluencedRNG();
		}

		bool Service_OpportunisiticShots::canShoot() const
		{
			static LevelSystem& levelSys = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSys.getCurrentLevel()) //#TODO #optimize cache level and listen for updates #auto_delegate_listening_handles?
			{
				return currentLevel->getWorldTimeManager()->getTimestampSecs() - lastShotTimestamp > shootCooldown;
			}

			return false;
		}

		bool Service_OpportunisiticShots::tryShootAtTarget(const TargetType& target, const Ship* myShip, const glm::vec3& myPos, const glm::vec3& myForward_n)
		{
			using namespace glm;

			vec3 targetPos = target.getWorldPosition();
			vec3 toTarget_n = glm::normalize(targetPos - myPos);

			float toTarget_cosTheta = glm::dot(toTarget_n, myForward_n);
			if (toTarget_cosTheta >= fireRadius_cosTheta)
			{
				//correct for velocity
				//#TODO, perhaps get a kinematic component or something to get an idea of the velocity
				vec3 correctTargetPos = targetPos;
				vec3 toFirePos_un = correctTargetPos - myPos;

				//construct basis around target direction
				vec3 fireRight_n = glm::normalize(glm::cross(toFirePos_un, vec3(myShip->getUpDir())));
				vec3 fireUp_n = glm::normalize(glm::cross(fireRight_n, toFirePos_un));

				//add some randomness to fire direction, more random of easier AI; randomness also helps correct for twitch movements done by target
				//we could apply this directly the fire direction we pass to ship, but this has a more intuitive meaning and will probably perform better for getting actual hits
				correctTargetPos += fireRight_n * rng->getFloat<float>(-1.f, 1.f) * shootRandomOffsetStrength;
				correctTargetPos += fireUp_n * rng->getFloat<float>(-1.f, 1.f) * shootRandomOffsetStrength;

				//fire in direction
				vec3 toFirePos_n = glm::normalize(correctTargetPos - myPos);
				myShip->fireProjectileInDirection(toFirePos_n);

				static LevelSystem& levelSys = GameBase::get().getLevelSystem();
				if (const sp<LevelBase>& currentLevel = levelSys.getCurrentLevel())
				{
					lastShotTimestamp = currentLevel->getWorldTimeManager()->getTimestampSecs();
				}

				return true;
			}

			return false;
		}

		void Decorator_FighterStateSetter::notifyTreeEstablished()
		{
			//Decorator::notifyTreeEstablished();

			deferredStateChangeTimer = new_sp<MultiDelegate<>>();
			deferredStateChangeTimer->addWeakObj(sp_this(), &Decorator_FighterStateSetter::handleDeferredStateUpdate);

			rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();

			Memory& memory = getMemory();
			memory.getModifiedDelegate(targetKey).addWeakObj(sp_this(), &Decorator_FighterStateSetter::handleTargetModified);
			memory.getModifiedDelegate(activeAttackersKey).addWeakObj(sp_this(), &Decorator_FighterStateSetter::handleActiveAttackersModified);

		}

		void Decorator_FighterStateSetter::handleDeferredStateUpdate()
		{
			reevaluateState();
		}

		void Decorator_FighterStateSetter::startStateUpdateTimer()
		{
			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				if (const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager())
				{
					if (!worldTimeManager->hasTimerForDelegate(deferredStateChangeTimer))
					{
						worldTimeManager->createTimer(deferredStateChangeTimer, deferrStateTime + rng->getFloat(0.f, 0.1f));
					}
				}
			}
		}

		void Decorator_FighterStateSetter::stopStateUpdateTimer()
		{
			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				if (const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager())
				{
					if (worldTimeManager->hasTimerForDelegate(deferredStateChangeTimer))
					{
						worldTimeManager->removeTimer(deferredStateChangeTimer);
					}
				}
			}
		}

		void Decorator_FighterStateSetter::handleTargetModified(const std::string& key, const GameEntity* value)
		{
			DEBUG_SHIP_MSG(__FUNCTION__);
			reevaluateState();
		}

		void Decorator_FighterStateSetter::handleActiveAttackersModified(const std::string& key, const GameEntity* value)
		{
			DEBUG_SHIP_MSG(__FUNCTION__);
			//reevaluateState();
			startStateUpdateTimer();
		}

		void Decorator_FighterStateSetter::reevaluateState()
		{
			//WARNING: Becareful not to call this method when MentalState_Fighter memory changes, that could result in infinite recursion as this also updates that field

			Memory& memory = getMemory();
			const MentalState_Fighter* state = memory.getReadValueAs<MentalState_Fighter>(stateKey);
			const ActiveAttackers* activeAttackers = memory.getReadValueAs<ActiveAttackers>(activeAttackersKey);
			const TargetType* target = memory.getReadValueAs<TargetType>(targetKey);

			bool bRequiredDecisionDataAvailable = state && activeAttackers;
			if (bRequiredDecisionDataAvailable)
			{
				/////////////////////////////////////////////////////////////////////////////////////
				// make state setting decisions, pointers should be safe
				/////////////////////////////////////////////////////////////////////////////////////
				if (target)
				{
					if (activeAttackers->size() > 0)
					{
						if (activeAttackers->find(target) != activeAttackers->end())
						{
							writeState(*state, MentalState_Fighter::ATTACK, memory);
						}
						else
						{
							writeState(*state, MentalState_Fighter::EVADE, memory);
						}
					}
					else
					{
						writeState(*state, MentalState_Fighter::ATTACK, memory);
					}
				}
				else //no target
				{
					if (activeAttackers->size() > 0)
					{
						writeState(*state, MentalState_Fighter::EVADE, memory);
					}
					else
					{
						writeState(*state, MentalState_Fighter::WANDER, memory);
					}
				}
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "Required data for state setting is not available");
			}
		}

		void Decorator_FighterStateSetter::writeState(MentalState_Fighter currentState, MentalState_Fighter newState, Memory& memory)
		{
			ScopedUpdateNotifier<MentalState_Fighter> write_state;
			if (currentState != newState && memory.getWriteValueAs<MentalState_Fighter>(stateKey, write_state))
			{
				write_state.get() = newState;
				DEBUG_SHIP_MSG("writing state");
			}
		}

		void Task_Ship_GetEntityLocation::beginTask()
		{
			Memory& memory = getMemory();
			if (const EntityType* entity = memory.getReadValueAs<EntityType>(entityKey))
			{
				glm::vec3 position = entity->getWorldPosition();

				ScopedUpdateNotifier<glm::vec3> outWriteAccess;
				if (memory.getWriteValueAs<glm::vec3>(writeLocKey, outWriteAccess))
				{
					outWriteAccess.get() = position;
					evaluationResult = true;
					return;
				}
			}

			evaluationResult = false;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Task_TickingTask
		/////////////////////////////////////////////////////////////////////////////////////

		void Task_TickingTaskBase::beginTask()
		{
			//Task::beginTask();

			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				currentLevel->getWorldTimeManager()->registerTicker(sp_this());
			}

		}

		void Task_TickingTaskBase::taskCleanup()
		{
			//Task::taskCleanup();

			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				currentLevel->getWorldTimeManager()->removeTicker(sp_this());
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Task_Ship_FollowTarget_Indefinitely
		/////////////////////////////////////////////////////////////////////////////////////

		void Task_Ship_FollowTarget_Indefinitely::beginTask()
		{
			Task_TickingTaskBase::beginTask();

			prefDistTarget2 = preferredDistanceToTarget * preferredDistanceToTarget;
			if (glm::abs(prefDistTarget2) < 0.0001)
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "Preferred distance too close to zero.");
			}

			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				Memory& memory = getMemory();
				{
					ScopedUpdateNotifier<ShipAIBrain> brain_writable;
					ScopedUpdateNotifier<TargetType> target_Writable;
					{
						if (memory.getWriteValueAs(shipBrain_MemoryKey, brain_writable))
						{
							wp<Ship> myWeakShip = brain_writable.get().getWeakControlledTarget();
							if (myWeakShip.expired())
							{
								log(__FUNCTION__, LogLevel::LOG_ERROR, "could not find controlled ship");
								evaluationResult = false;
								return; //don't start ticking
							}
							else
							{
								myShip = myWeakShip.lock();
							}
						}
						if (memory.getWriteValueAs<TargetType>(target_MemoryKey, target_Writable))
						{
							TargetType& targetRef = target_Writable.get();
							myTarget = std::static_pointer_cast<TargetType>(targetRef.requestReference().lock());

							if (!myTarget)
							{
								log(__FUNCTION__, LogLevel::LOG_ERROR, "target did not provide a reference");
								evaluationResult = false;
								return; //don't start ticking
							}
						}
						else
						{
							log(__FUNCTION__, LogLevel::LOG_ERROR, "could not find target");
							evaluationResult = false;
							return; //don't start ticking
						}
					}
				}
			}
			myTarget->onDestroyedEvent->addWeakObj(sp_this(), &Task_Ship_FollowTarget_Indefinitely::handleTargetDestroyed);

			//lock on to target (this signals to target to potential evade; unrealistic but makes AI simplier)
			if (BrainComponent* brainComp = myTarget->getGameComponent<BrainComponent>())
			{
				assert(static_cast<bool>(myShip));
				if (const BehaviorTree::Tree* targetTree = brainComp->getTree())
				{
					Memory& targetMemory = targetTree->getMemory();
					{
						ScopedUpdateNotifier<ActiveAttackers> outWriteValue;
						if (targetMemory.getWriteValueAs<ActiveAttackers>(activeAttackers_MemoryKey, outWriteValue))
						{
							DEBUG_SHIP_MSG("locking on to target");
							outWriteValue.get().insert({ myShip.get(), CurrentAttackerDatum{ myShip } });
						}
					}
				}
			}
		}

		void Task_Ship_FollowTarget_Indefinitely::taskCleanup()
		{
			Task_TickingTaskBase::taskCleanup();

			if (myTarget != nullptr)
			{
				if (BrainComponent* brainComp = myTarget->getGameComponent<BrainComponent>())
				{
					assert(static_cast<bool>(myShip));
					if (const BehaviorTree::Tree* targetTree = brainComp->getTree())
					{
						Memory& targetMemory = targetTree->getMemory();
						{
							ScopedUpdateNotifier<ActiveAttackers> outWriteValue;
							if (targetMemory.getWriteValueAs<ActiveAttackers>(activeAttackers_MemoryKey, outWriteValue))
							{
								DEBUG_SHIP_MSG("un-locking on to target");
								outWriteValue.get().erase(myShip.get());
							}
						}
					}
				}

				//clean up target
				handleTargetDestroyed(myTarget);
			}

			myShip = nullptr;
		}

		bool Task_Ship_FollowTarget_Indefinitely::tick(float dt_sec)
		{
			using namespace glm;

			if (myTarget) //#concern this may not be necessary depending on how we handle targets getting destroyed
			{
				vec3 targetPos = myTarget->getWorldPosition();
				vec3 myPos = myShip->getWorldPosition();

				float distToTarget2 = glm::length2(targetPos - myPos);

				//if target is really close, do not try to adjust
				if (distToTarget2 > 1.0f)
				{
					//float slowdownCloseFactor = glm::clamp(distToTarget2 / prefDistTarget2, 0.25f, 1.f);  //#TODO renable this after evade is implemented
					float slowdownCloseFactor = 1.f;

					myShip->moveTowardsPoint(targetPos, dt_sec, slowdownCloseFactor);
				}

				if constexpr (ENABLE_DEBUG_LINES)
				{
					static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
					vec4 visibleNudge(0, 0.1f, 0, 0);
					debug.renderLine(vec4(myPos, 1) + visibleNudge, vec4(targetPos, 1), vec4(0.25f, 0, 0, 1));
				}
			}
			return true;
		}

		void Task_Ship_FollowTarget_Indefinitely::handleTargetDestroyed(const sp<GameEntity>& target)
		{
			assert(target == myTarget);
			if (myTarget)
			{
				myTarget->onDestroyedEvent->removeWeak(sp_this(), &Task_Ship_FollowTarget_Indefinitely::handleTargetDestroyed);
				myTarget = nullptr;
				evaluationResult = false; //#todo, perhaps need to return false or listen for new target -- not sure on best design yet. Not return requires nullchecking target
			}
		}

		////////////////////////////////////////////////////////
		// Task_FindDogfightLocation
		////////////////////////////////////////////////////////
		void Task_FindDogfightLocation::beginTask()
		{
			using namespace BehaviorTree;
			Memory& memory = getMemory();
			{
				const ShipAIBrain* myBrain = memory.getReadValueAs<ShipAIBrain>(brainKey);
				ScopedUpdateNotifier<PrimitiveWrapper<glm::vec3>> location_writable;
				if (
					myBrain &&
					memory.getWriteValueAs(writeLocationKey, location_writable)
					)
				{
					glm::vec3& dogFightLoc = location_writable.get().value;

					static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
					SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(levelSystem.getCurrentLevel().get());

					const Ship* myShip = myBrain->getControlledTarget();
					if (myShip && spaceLevel)
					{
						if (const TeamComponent* teamComp = myShip->getGameComponent<TeamComponent>())
						{
							if (TeamCommander* teamCommander = spaceLevel->getTeamCommander(teamComp->getTeam()))
							{
								glm::vec3 dogfightLocation = getRandomPointNear(teamCommander->getCommanderPosition());
								glm::vec3 myShipLoc = myShip->getWorldPosition();
								glm::vec3 toPoint = dogfightLocation - myShipLoc;
								if (glm::length2(toPoint) > cacheMaxTravelDistance2)
								{
									toPoint = glm::normalize(toPoint) * maxTravelDistance;
								}
								dogFightLoc = myShipLoc + toPoint;
								evaluationResult = true;
								return;
							}
							else
							{
								log(__FUNCTION__, LogLevel::LOG_ERROR, "no commander.");
							}
						}
						else
						{
							log(__FUNCTION__, LogLevel::LOG_ERROR, "no team component.");
						}
					}
					else
					{
						log(__FUNCTION__, LogLevel::LOG_ERROR, "no control target or level.");
					}
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory keys.");
				}
			}
			evaluationResult = false;
		}

		void Task_FindDogfightLocation::postConstruct()
		{
			Task::postConstruct();
			rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();
			cacheMaxTravelDistance2 = maxTravelDistance * maxTravelDistance;
		}

		glm::vec3 Task_FindDogfightLocation::getRandomPointNear(glm::vec3 point)
		{
			glm::vec3 randomDir;
			randomDir.x = rng->getFloat(-1.f, 1.f);
			randomDir.y = rng->getFloat(-1.f, 1.f);
			randomDir.z = rng->getFloat(-1.f, 1.f);

			glm::vec3 offsetVec = glm::normalize(randomDir) * rng->getFloat(minRandomPointRadius, maxRandomPointRadius);
			return point + offsetVec;
		}

		////////////////////////////////////////////////////////
		// Task_EvadePattern1
		////////////////////////////////////////////////////////
		void Task_EvadePatternBase::beginTask()
		{
			Task_TickingTaskBase::beginTask();
			accumulatedTime = 0.f;

			using namespace BehaviorTree;
			Memory& memory = getMemory();
			{
				const glm::vec3* targetLoc = memory.getReadValueAs<glm::vec3>(targetLocKey);
				ScopedUpdateNotifier<ShipAIBrain> brain_writable;
				if (	
					memory.getWriteValueAs(brainKey, brain_writable) &&
					targetLoc 
				)
				{
					ShipAIBrain& brain = brain_writable.get();
					myShip = brain.getWeakControlledTarget();
					base.myTargetLoc = *targetLoc;
					if (myShip)
					{
						base.myShipStartLoc = myShip->getWorldPosition();
						base.startToTarget_n = base.myTargetLoc - base.myShipStartLoc;
						base.totalTravelDistance = glm::length(base.startToTarget_n);
						base.startToTarget_n /= base.totalTravelDistance; //normalize

						childSetup();
					}
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
					evaluationResult = false; //end this task asap
				}
			}

		}

		void Task_EvadePatternBase::taskCleanup()
		{
			myShip = nullptr;
			base = MyPOD{};

			Task_TickingTaskBase::taskCleanup();
		}

		bool Task_EvadePatternBase::tick(float dt_sec)
		{
			accumulatedTime += dt_sec;
			if (accumulatedTime > timeoutSecs)
			{
				evaluationResult = false;
			}
			else //not timed out
			{
				tickPattern(dt_sec);
			}
			return true;
		}

		void Task_EvadePatternBase::postConstruct()
		{
			rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Evade pattern spiral
		/////////////////////////////////////////////////////////////////////////////////////

		void Task_EvadePatternSpiral::childSetup()
		{
			using namespace glm;
			vec3 tempUp = Utils::getDifferentVector(base.startToTarget_n);
			spiral.right_n = glm::normalize(cross(base.startToTarget_n, tempUp));
			cylinderRotDirection = (rng->getFloat(-1.f, 1.f) > 0) ? 1.0f : -1.0f; //use sign as binary choice between -1.f and 1.f, don't allow non-1 values
		}

		void Task_EvadePatternSpiral::tickPattern(float dt_sec)
		{
			using namespace glm;

			if (myShip)
			{
				vec3 worldPosition = myShip->getWorldPosition();
				vec3 forwardDir = vec3(myShip->getForwardDir());

				float directionProjection = glm::dot(worldPosition - base.myShipStartLoc, base.startToTarget_n);

				if (directionProjection >= base.totalTravelDistance)
				{
					evaluationResult = true;
					return;
				}

				if (directionProjection >= spiral.nextPointCalculationDistance || spiral.bFirstTick)
				{
					spiral.bFirstTick = false;

					spiral.nextPointCalculationDistance += pointDistanceIncrement;
					spiral.cylinderRotation_rad += cylinderRotationIncrement_rad * cylinderRotDirection;

					quat rotate_q = glm::angleAxis(spiral.cylinderRotation_rad, base.startToTarget_n);
					vec3 rotatedOffset = rotate_q * spiral.right_n;

					spiral.nextPnt = base.startToTarget_n * spiral.nextPointCalculationDistance + base.myShipStartLoc;
					spiral.nextPnt += rotatedOffset * cylinderRadius;
					NAN_BREAK(spiral.nextPnt);

					//gamification - make spirals just as fast as flying in straight line. 
					//project spiral dir onto main_dir to find out fraction moved in main_dir, bump speed to make this 1.0f
					vec3 pntOnDirLine = base.startToTarget_n * directionProjection;
					vec3 toNextPnt_n = glm::normalize(spiral.nextPnt - pntOnDirLine);
					float speedScaleDownFactor = glm::dot(base.startToTarget_n, toNextPnt_n);
					spiral.speedBoost = glm::clamp( 1/speedScaleDownFactor, 1.f, 2.f);
				}

				myShip->moveTowardsPoint(spiral.nextPnt, dt_sec, spiral.speedBoost, true);

				if constexpr (ENABLE_DEBUG_LINES)
				{
					static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
					debug.renderLine(
						vec4(base.myShipStartLoc + base.startToTarget_n * directionProjection, 1),
						vec4(base.myTargetLoc, 1),
						vec4(0.76f, 0.76f, 0.14f, 1));

					debug.renderLine(
						vec4(worldPosition, 1),
						vec4(spiral.nextPnt, 1),
						vec4(0.f, 0.4f, 0.f, 1));
				}

			}
		}

		void Task_EvadePatternSpiral::taskCleanup()
		{
			Task_EvadePatternBase::taskCleanup();
			spiral = SpiralData{};
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Evade pattern spin
		/////////////////////////////////////////////////////////////////////////////////////
		void Task_EvadePatternSpin::childSetup()
		{
			using namespace glm;

			cylinderDir = (rng->getFloat(-1.f, 1.f) > 0) ? 1.0f : -1.0f; //use sign as binary choice between -1.f and 1.f, don't allow non-1 values
			spinDir = (rng->getFloat(-1.f, 1.f) > 0) ? 1.0f : -1.0f;

			vec3 tempUp = Utils::getDifferentVector(base.startToTarget_n);
			spin.right_n = glm::normalize(cross(base.startToTarget_n, tempUp));
		}

		void Task_EvadePatternSpin::tickPattern(float dt_sec)
		{
			using namespace glm;

			if (myShip)
			{
				vec3 worldPosition = myShip->getWorldPosition();
				vec3 forwardDir = vec3(myShip->getForwardDir());

				float directionProjection = glm::dot(worldPosition - base.myShipStartLoc, base.startToTarget_n);

				if (directionProjection >= base.totalTravelDistance)
				{
					evaluationResult = true;
					return;
				}

				if (directionProjection >= spin.nextPointCalculationDistance || spin.bFirstTick)
				{
					spin.bFirstTick = false;

					spin.nextPointCalculationDistance += pointDistanceIncrement;
					spin.cylinderRotation_rad += cylinderRotationIncrement_rad * cylinderDir;

					quat rotate_q = glm::angleAxis(spin.cylinderRotation_rad, base.startToTarget_n);
					vec3 rotatedOffset = rotate_q * spin.right_n;

					spin.nextPnt = base.startToTarget_n * spin.nextPointCalculationDistance + base.myShipStartLoc;
					spin.nextPnt += rotatedOffset * cylinderRadius;
					NAN_BREAK(spin.nextPnt);
				}

				myShip->moveTowardsPoint(spin.nextPnt, dt_sec, 1.f, true);
				myShip->roll(spin.rollspinIncrement_rad * spinDir, dt_sec);

				if constexpr (ENABLE_DEBUG_LINES)
				{
					static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
					debug.renderLine(
						vec4(base.myShipStartLoc + base.startToTarget_n * directionProjection, 1),
						vec4(base.myTargetLoc, 1),
						vec4(0.76f, 0.76f, 0.14f, 1));

					debug.renderLine(
						vec4(worldPosition, 1),
						vec4(spin.nextPnt, 1),
						vec4(0.f, 0.4f, 0.f, 1));
				}

			}
		}

		void Task_EvadePatternSpin::taskCleanup()
		{
			Task_EvadePatternBase::taskCleanup();
			spin = SpinData{};
		}

	}



}

