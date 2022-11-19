#include "SAShipBehaviorTreeNodes.h"
#include "SAShipAIBrain.h"
#include "Game/AI/SAShipBehaviorTreeNodes.h"
#include "Game/Levels/SASpaceLevelBase.h"
#include "Game/SAShip.h"
#include "Game/Team/Commanders.h"
#include "GameFramework/Components/GameplayComponents.h"
#include "GameFramework/CurveSystem.h"
#include "GameFramework/SADebugRenderSystem.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SALevel.h"
#include "GameFramework/SALevelSystem.h"
#include "GameFramework/SALog.h"
#include "GameFramework/SARandomNumberGenerationSystem.h"
#include "ReferenceCode/OpenGL/Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "Tools/DataStructures/ChoiceChoosingHelper.h"
#include "Tools/DataStructures/SATransform.h"
#include "Tools/PlatformUtils.h"
#include "Tools/SAUtilities.h"
#include "Tools/color_utils.h"
#include <assert.h>
#include <gtc/quaternion.hpp>
//#include "GameFramework/SAGameBase.h"
//#include "GameFramework/SALevelSystem.h"


namespace SA
{
	constexpr bool EVADE_STATE_ENABLED = false;

	namespace BehaviorTree
	{
		void LogShipNodeDebugMessage(const Tree& tree, const NodeBase& node, const std::string& msg)
		{
			std::string treeInstance = std::to_string((uint64_t)&tree);
			std::string newMessage = treeInstance + " : " + node.getName() + "\t - " + msg;
			log("ShipNodeDebugMessage", LogLevel::LOG, newMessage.c_str());
		}

		void cleanActiveAttackers(BehaviorTree::ActiveAttackers& attackers)
		{
			auto iter = attackers.begin();
			while(iter != attackers.end())
			{
				if (!iter->second.attacker || iter->second.attacker->isPendingDestroy())
				{
					iter = attackers.erase(iter);
				}
				else
				{
					++iter;
				}
			}
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
			using namespace glm;
			timeTicked += tickSecs;
			//if (timeTicked > nextAttackerRefreshTimeStamp)
			//{
			//	nextAttackerRefreshTimeStamp += attackerRefreshRateSec;
			//	handleActiveAttackersModified(activeAttackersKey, nullptr);
			//}

			//this service should always have a brain available while it is ticking; if it doesn't this is a design error.
			assert(owningBrain);
			assert(attackers); //there is not a race condition here; service removes callback from timer when stopping
			assert(stateRef);

			bool bIsAttackingObjective = (stateRef && stateRef->value == MentalState_Fighter::ATTACK_OBJECTIVE);
			bool bShouldTargetAttackers = !bTargetIsPlayer && !bIsAttackingObjective;

			if (bEvaluateActiveAttackersOnNextTick && TARGET_ATTACKERS_FEATURE_ENABLED && bShouldTargetAttackers)
			{
				bEvaluateActiveAttackersOnNextTick = false;

				if (attackers && myShip)
				{
					vec3 myWorldPos = myShip->getWorldPosition();
					fwp<TargetType> bestSoFar = nullptr;
					float bestSoFarDist2 = std::numeric_limits<float>::infinity();

					//find best target
					for (auto& attacker_pair : *attackers)
					{
						CurrentAttackerDatum& attackerDatum = attacker_pair.second;
						if (attackerDatum.attacker)
						{
							vec3 toAttacker = attackerDatum.attacker->getWorldPosition() - myWorldPos;
							float attackerDist2 = glm::length2(toAttacker);
							if (attackerDist2 < bestSoFarDist2)
							{
								bestSoFarDist2 = attackerDist2;
								bestSoFar = attackerDatum.attacker;
							}
						}
					}

					if (bestSoFar)
					{
						attackerToTarget = bestSoFar;
					}
				}
			}

			//if someone is targeting, try to use them as a target if appropriate
			if (attackerToTarget && myShip)
			{
				//if our current target has engaged us, the handler for attackers changing will have cleared the staged attacker to target
				vec3 toAttacker = attackerToTarget->getWorldPosition() - myShip->getWorldPosition();

				//if attacker has gotten in range, target it to engage a dogfight
				if (glm::length2(toAttacker) < cachedPrefDist2)
				{
					wp<TargetType> weakNewTarget = attackerToTarget;
					sp<TargetType> newTarget = weakNewTarget.lock();
					attackerToTarget = nullptr; 
					setTarget(newTarget); 
				}
			}

			if (!currentTarget)
			{
				tickFindNewTarget_slow();
			}
		}

		void Service_TargetFinder::startService()
		{
			Memory& memory = getMemory();
			memory.getModifiedDelegate(targetKey).addStrongObj(sp_this(), &Service_TargetFinder::handleTargetModified);
			memory.getModifiedDelegate(activeAttackersKey).addStrongObj(sp_this(), &Service_TargetFinder::handleActiveAttackersModified);
			memory.getModifiedDelegate(stateKey).addStrongObj(sp_this(), &Service_TargetFinder::handleStateModified);

			stateRef = memory.getMemoryReference<PrimitiveWrapper<MentalState_Fighter>>(stateKey);

			resetSearchData();
			{
				ScopedUpdateNotifier<TargetType> target_writable;
				ScopedUpdateNotifier<ShipAIBrain> brain_writable;
				ScopedUpdateNotifier<PrimitiveWrapper<ActiveAttackers>> attackers_writable;
				if (memory.getWriteValueAs(brainKey, brain_writable)
					&& memory.getWriteValueAs(activeAttackersKey, attackers_writable))
				{
					attackers = &attackers_writable.get().value;
					owningBrain = &brain_writable.get();
					if (Ship* myShipRaw = owningBrain->getControlledTarget())
					{
						myShip = myShipRaw->requestTypedReference_Nonsafe<Ship>().lock();
					}

					if(memory.getWriteValueAs(targetKey, target_writable))
					{
						TargetType& target = target_writable.get();
						wp<TargetType> weakTarget = target.requestTypedReference_Nonsafe<TargetType>();
						if (!weakTarget.expired())
						{
							setTarget(weakTarget.lock());
						}
					}
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory");
				}
			}

			cachedPrefDist2 = preferredTargetMaxDistance * preferredTargetMaxDistance;
			if (owningBrain)
			{
				if (const Ship* myShipRaw = owningBrain->getControlledTarget())
				{
					if(const TeamComponent* teamCom = myShipRaw->getGameComponent<TeamComponent>())
					{
						cachedTeamIdx = teamCom->getTeam();
					}
				}
			}

			//if we were already assigned a target at spawn/start, then run through the modified logic
			tryApplyPlayerSpecialCases();
		}

		void Service_TargetFinder::stopService()
		{
			//when stopping service, remove delegates first so we don't hit target changed events when clearing target
			Memory& memory = getMemory();
			memory.getModifiedDelegate(targetKey).removeStrong(sp_this(), &Service_TargetFinder::handleTargetModified);
			memory.getModifiedDelegate(activeAttackersKey).removeStrong(sp_this(), &Service_TargetFinder::handleActiveAttackersModified);
			memory.getModifiedDelegate(stateKey).removeStrong(sp_this(), &Service_TargetFinder::handleStateModified);

			//#TODO it is a little backwards that we give the command back a target we the ship is destroyed
			//ideally the commander would pass out handles, and then on destroy that handle would give
			//the target back to the commander if it is still around. This is similar to how the spatial hashing 
			//handles work, but slightly different. Spatial hashing handles release some internal state when destroyed
			//anyways, there is very little gain in engineering that right now, so just having this clean its own resources up.
			setTarget(nullptr);
			owningBrain = nullptr;
			attackers = nullptr;
			stateRef = nullptr;
		}

		void Service_TargetFinder::handleTargetModified(const std::string& key, const GameEntity* value)
		{

			if (value == nullptr)
			{
				clearPlayerSpecialCases(); //only clear if modification was a replacement
				currentTarget.reset();
				resetSearchData();
				tickFindNewTarget_slow();
			}
			else if (value != currentTarget.get())
			{
				clearPlayerSpecialCases(); //only clear if modification was a replacement

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

					//avoiding doing this every tick
					tryApplyPlayerSpecialCases();

				}
				else{log("Service_TargetFinder", LogLevel::LOG_ERROR, "detected target change from outside, but could not acquire handle to it.");}
			}
		}

		void Service_TargetFinder::handleActiveAttackersModified(const std::string& key, const GameEntity* value)
		{
			if (!bEvaluateActiveAttackersOnNextTick && myShip)
			{
				if (!XisTargetingY(currentTarget, myShip))
				{
					//our target isn't attacking us, prepare to target a current attacker based on distance
					bEvaluateActiveAttackersOnNextTick = true;
				}
				else
				{
					//our current target is now targeting us, abandon any attackers we were about to target
					attackerToTarget = nullptr;
				}
			}
		}

		void Service_TargetFinder::handleTargetDestroyed(const sp<GameEntity>& entity)
		{
			setTarget(nullptr);
			bEvaluateActiveAttackersOnNextTick = true;
			clearPlayerSpecialCases();
		}

		void Service_TargetFinder::handleStateModified(const std::string& key, const GameEntity* value)
		{
			Memory& memory = getMemory();
			stateRef = memory.getMemoryReference<PrimitiveWrapper<MentalState_Fighter>>(stateKey);
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
								setTarget(target, true);
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

					std::vector<sp<const SH::HashCell<WorldEntity>>> nearbyCells; //#optimize constructing vectors has been seen to be slow before, perhaps make this static if it is only going to be used within a single thread
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

		void Service_TargetFinder::setTarget(const sp<WorldEntity>& target, bool bCommanderAssignment /*= false*/)
		{
			if (target == currentTarget)
			{
				return;
			}

			if (currentTarget)
			{
				currentTarget->onDestroyedEvent->removeWeak(sp_this(), &Service_TargetFinder::handleTargetDestroyed);

				if (bCommanderProvidedTarget && !currentTarget->isPendingDestroy())
				{
					//return this target to the list of targets the commander wants destroyed
					static LevelSystem& LevelSys = GameBase::get().getLevelSystem();
					const sp<LevelBase>& level = LevelSys.getCurrentLevel();
					SpaceLevelBase* spaceLevel = level ? dynamic_cast<SpaceLevelBase*>(level.get()) : nullptr;
					TeamCommander* teamCommander = spaceLevel ? spaceLevel->getTeamCommander(cachedTeamIdx) : nullptr;
					if (teamCommander)
					{
						teamCommander->queueTarget(currentTarget);
					}
				}
			}

			currentTarget = target;
			bCommanderProvidedTarget = bCommanderAssignment;

			//make sure write to memory happens AFTER updating current target. This is to prevent the handler for target modified from updating current target twice.
			getMemory().replaceValue(targetKey, currentTarget);

			if (target)
			{
				target->onDestroyedEvent->addWeakObj(sp_this(), &Service_TargetFinder::handleTargetDestroyed);
			}
		}

		bool Service_TargetFinder::XisTargetingY(const sp<TargetType>& x, const lp<const TargetType>& y)
		{
			if (x && y) 
			{ 
				if (const BrainComponent* brainComp = x->getGameComponent<BrainComponent>())
				{
					if (const BehaviorTree::Tree* xTree = brainComp->getTree())
					{
						Memory& memory = xTree->getMemory();
						if (const TargetType* targetOfX = memory.getReadValueAs<TargetType>(targetKey))
						{
							return targetOfX == y.get();
						}
					}
				}
			}

			return false;
		}

		void Service_TargetFinder::clearPlayerSpecialCases()
		{
			//before we clear this flag, restore normal set up
			if (bTargetIsPlayer)
			{
				if (myShip)
				{
					myShip->setAvoidanceSensitivity(1.0f);
				}
			}
			//clear flag after so we only do this if previously we were targeting player (makes debugging a lot easier, we're not tripping breakpoints unnecessarily)
			bTargetIsPlayer = false; 
		}

		void Service_TargetFinder::tryApplyPlayerSpecialCases()
		{
			if (!bTargetIsPlayer && currentTarget)
			{
				if (const OwningPlayerComponent* playerComp = currentTarget->getGameComponent<OwningPlayerComponent>())
				{
					bTargetIsPlayer = playerComp->hasOwningPlayer();
					if (bTargetIsPlayer && myShip)
					{
						myShip->setAvoidanceSensitivity(0.0f);
					}
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Service_AttackerSetter
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		void modifyAttackers(lp<TargetType>& target, bool bAdd, sp<TargetType>& myShip, const std::string& attackersKey)
		{
			if (BrainComponent* brainComp = target->getGameComponent<BrainComponent>())
			{
				if (const BehaviorTree::Tree* tree = brainComp->getTree())
				{
					Memory& targetMemory = tree->getMemory();
					ScopedUpdateNotifier<PrimitiveWrapper<ActiveAttackers>> attackers_writable;
					if (targetMemory.getWriteValueAs(attackersKey, attackers_writable))
					{
						ActiveAttackers& targetsAttackers = attackers_writable.get().value;
						if (bAdd)
						{
							targetsAttackers.insert({ myShip.get(), CurrentAttackerDatum{ myShip } });
						}
						else
						{
							targetsAttackers.erase(myShip.get());
						}
					}
					else
					{
						//target does not track attackers (eg placement), that's okay and not an error
					}
				}
			}
		};


		void Service_AttackerSetter::startService()
		{
			Memory& memory = getMemory();
			memory.getReplacedDelegate(targetKey).addStrongObj(sp_this(), &Service_AttackerSetter::handleTargetReplaced);
			{
				ScopedUpdateNotifier<PrimitiveWrapper<ActiveAttackers>> activeAttackers_writable;
				ScopedUpdateNotifier<ShipAIBrain> brain_writable;
				
				//memory that must be present
				if (memory.getWriteValueAs(attackersKey, activeAttackers_writable)
					&& memory.getWriteValueAs(brainKey, brain_writable)
					)
				{
					data.attackers = &(activeAttackers_writable.get().value);
					data.brain = &brain_writable.get();
					if (Ship* myShipRaw = data.brain->getControlledTarget())
					{
						data.myShip = myShipRaw->requestTypedReference_Nonsafe<Ship>().lock();
					}
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
					throw std::runtime_error("behavior tree node failed to get invariant memory");
				}
				
				//memory that may be present
				ScopedUpdateNotifier<TargetType> target_writable;
				if (memory.getWriteValueAs(targetKey, target_writable))
				{
					handleTargetReplaced(targetKey, nullptr, &target_writable.get());
				}
			}
		}

		void Service_AttackerSetter::stopService()
		{
			//make sure we clean up any attackers strutures
			if (data.myShip)
			{
				sp<TargetType> ship = data.myShip;
				if (data.currentTarget){modifyAttackers(data.currentTarget, false, ship, attackersKey);}
				if (data.lastTarget) { modifyAttackers(data.lastTarget, false, ship, attackersKey); }
			}

			//unbinding handlers before anything else is generally a good idea, as write won't cause handlers to be invoked.
			Memory& memory = getMemory();
			memory.getReplacedDelegate(targetKey).removeStrong(sp_this(), &Service_AttackerSetter::handleTargetReplaced);

			data = Data{};
		}

		void Service_AttackerSetter::handleTargetReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue)
		{
			Memory& memory = getMemory();

			data.lastTarget = data.currentTarget;

			ScopedUpdateNotifier<TargetType> target_writable;
			if (memory.getWriteValueAs(targetKey, target_writable))
			{
				data.currentTarget = target_writable.get().requestTypedReference_Nonsafe<TargetType>().lock();
			}

			//this will remove attackers form datastructure on next service tick
			data.bNeedsRefresh = true;
		}

		void Service_AttackerSetter::serviceTick()
		{
			if (data.bNeedsRefresh)
			{
				data.bNeedsRefresh = false;

				if (data.myShip)
				{
					sp<TargetType> myShipSp = data.myShip;

					if (data.lastTarget)
					{
						modifyAttackers(data.lastTarget, false, myShipSp, attackersKey);
					}
					if (data.currentTarget)
					{
						modifyAttackers(data.currentTarget, true, myShipSp, attackersKey);
					}

					data.lastTarget = nullptr;
				}
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// Service_OpportunisiticShots 
		/////////////////////////////////////////////////////////////////////////////////////

		void Service_OpportunisiticShots::startService()
		{
			Memory& memory = getMemory();

			//owningBrain = const_cast<ShipAIBrain*>(memory.getReadValueAs<ShipAIBrain>(brainKey));
			{ //avoiding const cast on brain, cache a writable version.
				ScopedUpdateNotifier<ShipAIBrain> brain_writable;
				if (memory.getWriteValueAs(brainKey, brain_writable)) { owningBrain  = &brain_writable.get();}
				else{log(__FUNCTION__, LogLevel::LOG_ERROR, "could not get mutable brain."); STOP_DEBUGGER_HERE();}
			}

			primaryTarget = memory.getMemoryReference<TargetType>(targetKey);
			secondaryTargets = memory.getMemoryReference<const PrimitiveWrapper<SecondaryTargetContainer>>(secondaryTargetsKey);
			handleStateModified("directly calling handler to init state", nullptr);
			
			memory.getModifiedDelegate(targetKey).addStrongObj(sp_this(), &Service_OpportunisiticShots::handleTargetModified);
			memory.getModifiedDelegate(stateKey).addStrongObj(sp_this(), &Service_OpportunisiticShots::handleStateModified);

			//don't listen to modified, just replaced. We only care when the underlying datastructure may have been swapped out.
			memory.getReplacedDelegate(secondaryTargetsKey).addStrongObj(sp_this(), &Service_OpportunisiticShots::handleSecondaryTargetsReplaced);

			if (!stateRef) { log("Service_OpportunisiticShots", LogLevel::LOG_ERROR, "no mental state acquired brain"); exit(-1); }
			if (!owningBrain) { log("Service_OpportunisiticShots", LogLevel::LOG_ERROR, "No owning brain"); exit(-1); }
			if (!secondaryTargets) { log("Service_OpportunisiticShots", LogLevel::LOG_ERROR, "No secondary target array"); exit(-1);}
		}

		void Service_OpportunisiticShots::stopService()
		{
			owningBrain = nullptr;
			secondaryTargets = nullptr;
			primaryTarget = nullptr;

			Memory& memory = getMemory();
			memory.getModifiedDelegate(targetKey).removeStrong(sp_this(), &Service_OpportunisiticShots::handleTargetModified);
			memory.getModifiedDelegate(stateKey).removeStrong(sp_this(), &Service_OpportunisiticShots::handleStateModified);
			memory.getReplacedDelegate(secondaryTargetsKey).removeStrong(sp_this(), &Service_OpportunisiticShots::handleSecondaryTargetsReplaced);
		}

		void Service_OpportunisiticShots::handleTargetModified(const std::string& key, const GameEntity* value)
		{
			Memory& memory = getMemory();
			primaryTarget = memory.getMemoryReference<TargetType>(targetKey);
		}

		void Service_OpportunisiticShots::handleStateModified(const std::string& key, const GameEntity* value)
		{
			Memory& memory = getMemory();
			stateRef = memory.getMemoryReference<PrimitiveWrapper<MentalState_Fighter>>(stateKey);
		}

		void Service_OpportunisiticShots::handleSecondaryTargetsReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue)
		{
			Memory& memory = getMemory();
			secondaryTargets = memory.getMemoryReference<const PrimitiveWrapper<SecondaryTargetContainer>>(secondaryTargetsKey);
		}


		void Service_OpportunisiticShots::serviceTick()
		{
			using namespace glm;

			if (canShoot())
			{
				if (Ship* myShip = owningBrain->getControlledTarget())
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

			assert(stateRef);
			if (stateRef->value == MentalState_Fighter::ATTACK_FIGHTER)
			{
				return false;
			}

			if (const sp<LevelBase>& currentLevel = levelSys.getCurrentLevel()) //#TODO #optimize cache level and listen for updates #auto_delegate_listening_handles?
			{
				return (currentLevel->getWorldTimeManager()->getTimestampSecs() - lastShotTimestamp) > shootCooldown;
			}

			return false;
		}

		bool Service_OpportunisiticShots::tryShootAtTarget(const TargetType& target, Ship* myShip, const glm::vec3& myPos, const glm::vec3& myForward_n)
		{
			using namespace glm;

			//vec3 targetPos = target.getWorldPosition();
			//vec3 toTarget_n = glm::normalize(targetPos - myPos);

			//float toTarget_cosTheta = glm::dot(toTarget_n, myForward_n);
			//if (toTarget_cosTheta >= fireRadius_cosTheta)
			//{
			//	//correct for velocity
			//	//#TODO, perhaps get a kinematic component or something to get an idea of the velocity
			//	vec3 correctTargetPos = targetPos;
			//	vec3 toFirePos_un = correctTargetPos - myPos;

			//	//construct basis around target direction
			//	vec3 fireRight_n = glm::normalize(glm::cross(toFirePos_un, vec3(myShip->getUpDir())));
			//	vec3 fireUp_n = glm::normalize(glm::cross(fireRight_n, toFirePos_un));

			//	//add some randomness to fire direction, more random of easier AI; randomness also helps correct for twitch movements done by target
			//	//we could apply this directly the fire direction we pass to ship, but this has a more intuitive meaning and will probably perform better for getting actual hits
			//	correctTargetPos += fireRight_n * rng->getFloat<float>(-1.f, 1.f) * shootRandomOffsetStrength;
			//	correctTargetPos += fireUp_n * rng->getFloat<float>(-1.f, 1.f) * shootRandomOffsetStrength;

			//	//fire in direction
			//	vec3 toFirePos_n = glm::normalize(correctTargetPos - myPos);
			//	myShip->fireProjectileInDirection(toFirePos_n);

			if (myShip->fireProjectileAtShip(target, fireRadius_cosTheta, shootRandomOffsetStrength))
			{

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
					if (targetIsAnObjective(*target))
					{
						writeState(*state, MentalState_Fighter::ATTACK_OBJECTIVE, memory);
					}
					else if (bool bShouldConsiderEvading = activeAttackers->size() > 0)
					{
						//attack or evade(if enabled)
						if (activeAttackers->find(target) != activeAttackers->end() || !EVADE_STATE_ENABLED)
						{
							writeState(*state, MentalState_Fighter::ATTACK_FIGHTER, memory);
						}
						else if (EVADE_STATE_ENABLED)
						{
							writeState(*state, MentalState_Fighter::EVADE, memory);
						}
					}
					else
					{
						writeState(*state, MentalState_Fighter::ATTACK_FIGHTER, memory);
					}
				}
				else //no target
				{
					if (activeAttackers->size() > 0 && EVADE_STATE_ENABLED)
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

		bool Decorator_FighterStateSetter::targetIsAnObjective(const TargetType& target) const
		{
			return dynamic_cast<const ShipPlacementEntity*>(&target) != nullptr;
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



		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// DogfightNode and helpers
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		void AiVsPlayer_MoveArgAdjustments(BehaviorTree::DogfightNodeTickData& p, Ship::MoveTowardsPointArgs& moveArgs)
		{
			if (uint8_t(DF_ComboStepFlags::FIGHTING_PLAYER) & p.flags)
			{
				const float variabilityTimeIntervalSec = p.owningDFNode.getVariabilityIntervalSec();
				const float currentTime = p.owningDFNode.getAccumulatedTime();
				float variabilityMultiplier = std::fmod(currentTime, variabilityTimeIntervalSec);
				variabilityMultiplier = variabilityMultiplier / variabilityTimeIntervalSec; //make range [0,1]

				if constexpr (constexpr bool bDisableVariability_DEBUG = false) { variabilityMultiplier = 1.f; }

				moveArgs.viscosity = getAIVsPlayer_Viscosity(variabilityMultiplier, p);
				moveArgs.speedMultiplier = getAIVsPlayer_MoveSpeed(variabilityMultiplier, p);
			}
		}

		float getAIVsPlayer_Viscosity(float variabilityMultiplier, DogfightNodeTickData& p)
		{
			if (uint8_t(DF_ComboStepFlags::FIGHTING_PLAYER) & p.flags)
			{
				float viscosity = 1.f;

				//TODO perhaps a shared location? curve system forces copying so memory corrupting isn't possible, but maybe shared among BT nodes
				static Curve_highp curve = GameBase::get().getCurveSystem().generateSigmoid_medp(); //do not use local static if this becomes a wider thing; thread safety issues
				viscosity *= curve.eval_smooth(variabilityMultiplier);

				//100% viscosity means the ship cannot turn, scale that down
				viscosity *= 0.8f;

				float shipDifficulty = p.myShip.getAISkillLevel(); //[0,1], 1 means no viscosity
				viscosity *= glm::clamp((1.f-shipDifficulty),0.f,1.f); 

				//scale based on distance
				const SA::BehaviorTree::DogfightNodeTickData::TargetStats& t = p.targetStats();
				float invMaxDistPercent = (1 - glm::clamp(t.toTargetDistance / DogFightConstants::AiVsPlayer_ViscosityRange, 0.f, 1.f));
				viscosity *= invMaxDistPercent;

				return viscosity; 
			}
			else
			{
				return 0;
			}
		}


		float getAIVsPlayer_MoveSpeed(float variabilityMultiplier, DogfightNodeTickData& p)
		{
			constexpr bool bEnableAISpeedupWithViscosity = true;

			if (uint8_t(DF_ComboStepFlags::FIGHTING_PLAYER) & p.flags && bEnableAISpeedupWithViscosity)
			{
				//this more viscosity we're applying to an AI, the more we need to boost away.
				//this is because it has reduced turn speed, but player is still having short distance between it and the ship.
				const float maxSpeedupFactor = DogFightConstants::AiVsPlayer_MaxSpeedBoost; //must be at least 1

				float speedup = 1 + (maxSpeedupFactor - 1) * variabilityMultiplier; //variability should be on range [0,1]

				//scale based on distance
				const SA::BehaviorTree::DogfightNodeTickData::TargetStats& t = p.targetStats();
				float invMaxDistPercent = (1 - glm::clamp(t.toTargetDistance / DogFightConstants::AiVsPlayer_SpeedBoostRange, 0.f, 1.f));
				speedup *= invMaxDistPercent;

				//don't let factor go under 1, otherwise this will end up slowing the ship down
				speedup = glm::max(speedup, 1.f);
				return speedup;
			}
			else
			{
				return 1.f;
			}
		}

		void ComboStepBase::updateTimeAlive(DogfightNodeTickData& p)
		{
			basePOD.totalActiveTime += p.dt_sec;
			if (isInArrangement(p.arrangement))
			{
				basePOD.timeInWrongPhase = 0.f;
			}
			else
			{
				basePOD.timeInWrongPhase += p.dt_sec;
			}
		}

		bool ComboStepBase::isInArrangement(TargetDirection arrangement) const
		{
			return data.arrangements_bitvector & uint8_t(arrangement);
		}

		void ComboStepBase::setComboData(const ComboStepData& inComboData)
		{
			basePOD = BasePOD{};
			data = inComboData;
		}

		bool ComboStepBase::inGracePeriod() const
		{
			bool bTimedOut = (inPhaseTimeout.has_value() && inPhaseTimeout.value() < basePOD.totalActiveTime);

			return (basePOD.timeInWrongPhase < wrongPhaseTimeout)
					&& !basePOD.bForceStepEnd
					&& !bTimedOut;
		}

		void SharpTurnStep::tickStep(DogfightNodeTickData& p)
		{
			using namespace glm;
			vec3 targetPos = p.myTarget.getWorldPosition();
			vec3 myPos = p.myShip.getWorldPosition();

			p.myShip.adjustSpeedFraction(p.dt_sec, 0.5f);

			Ship::MoveTowardsPointArgs moveArgs{ targetPos, p.dt_sec };
			AiVsPlayer_MoveArgAdjustments(p, moveArgs);
			p.myShip.moveTowardsPoint(moveArgs);

			if constexpr (ENABLE_DEBUG_LINES)
			{
				static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
				debug.renderLine(vec4(myPos + vec3(0, 0.25f,0), 1),vec4(targetPos, 1), vec3(0.6, 0.7, 0.3));
			}
		}

		void SlowWhenFacingStep::tickStep(DogfightNodeTickData& p)
		{
			using namespace glm;
			vec3 debugColor = vec3(0.81f, 0.92f, 0.81f);

			vec3 targetPos = p.myTarget.getWorldPosition();
			vec3 myPos = p.myShip.getWorldPosition();

			if (p.arrangement == TargetDirection::FACING)
			{
				vec3 toTarget = targetPos - myPos;
				float lengthToTarget2 = glm::length2(toTarget);

				//vec3 targetPoint;
				if (lengthToTarget2 > slowdownDist2)
				{
					//targetPoint = targetPos; //#cleanup delete after refactor
					p.myShip.adjustSpeedFraction(p.dt_sec, 1.0f);

					Ship::MoveTowardsPointArgs moveArgs{ targetPos, p.dt_sec };
					AiVsPlayer_MoveArgAdjustments(p, moveArgs);
					p.myShip.moveTowardsPoint(moveArgs);
				}
				else
				{
					vec3 besideTargetPos = targetPos + (vec3(p.myShip.getRightDir()) * 5.0f); //move slightly beside target

					p.myShip.adjustSpeedFraction(p.dt_sec, 0.5f);

					Ship::MoveTowardsPointArgs moveArgs{ besideTargetPos, p.dt_sec };
					moveArgs.bRoll = false; //don't roll to stabilize the right vector
					AiVsPlayer_MoveArgAdjustments(p, moveArgs);
					p.myShip.moveTowardsPoint(moveArgs);

					if constexpr (ENABLE_DEBUG_LINES) { debugColor *= 0.5f; }
				}

				if constexpr (ENABLE_DEBUG_LINES)
				{
					static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
					debug.renderLine(vec4(myPos + vec3(0.f,0.25f, 0.f), 1), glm::vec4(targetPos, 1), debugColor);
				}
			}
		}

		void FollowAndAttackStep::tickStep(DogfightNodeTickData& p)
		{

		}


		////////////////////////////////////////////////////////
		// Faceoff collision avoidance
		////////////////////////////////////////////////////////
		void FaceoffCollisionAvoidanceStep::tickStep(DogfightNodeTickData& p)
		{
			using namespace glm;
			wrongPhaseTimeout = data.optionalFloats.a;

			vec3 myPos = p.myShip.getWorldPosition();
			vec3 targetPos = p.myTarget.getWorldPosition();
			float dist2 = glm::length2(targetPos - myPos);

			vec3 debugColor(0.5f, 0.f, 0.9f);

			if (p.arrangement == TargetDirection::FACING)
			{
				if (dist2 > startAvoidDist2)
				{
					Ship::MoveTowardsPointArgs moveArgs{ targetPos, p.dt_sec };
					AiVsPlayer_MoveArgAdjustments(p, moveArgs);
					p.myShip.moveTowardsPoint(moveArgs);
				}
				else 
				{
					//don't roll so right direction stays relatively stable
					targetPos += normalize(vec3(p.myShip.getRightDir())) * 1.f;

					Ship::MoveTowardsPointArgs moveArgs{ targetPos, p.dt_sec };
					moveArgs.bRoll = false;
					AiVsPlayer_MoveArgAdjustments(p, moveArgs);
					p.myShip.moveTowardsPoint(moveArgs); 

					debugColor *= 0.75f;
				}
			}
			else /* out of appropriate arrangement*/
			{
				float turnLimit = (basePOD.timeInWrongPhase + 0.001f) / wrongPhaseTimeout;

				//do not let full turn happen, otherwise combo step will reengage when again facing
				const float maximumTurnFactor = 0.25f;
				turnLimit *= maximumTurnFactor; 

				Ship::MoveTowardsPointArgs moveArgs{ targetPos, p.dt_sec };
				AiVsPlayer_MoveArgAdjustments(p, moveArgs);
				moveArgs.turnMultiplier = turnLimit;
				p.myShip.moveTowardsPoint(moveArgs);

				debugColor *= 0.25f;
			}

			if constexpr (ENABLE_DEBUG_LINES)
			{
				static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
				debug.renderLine(glm::vec4(myPos, 1), glm::vec4(targetPos, 1), debugColor);
			}
		}

		////////////////////////////////////////////////////////
		// Faceoff boost after avoidance
		////////////////////////////////////////////////////////

		void BoostAwayStep::tickStep(DogfightNodeTickData& p)
		{
			using namespace glm;
			static vec3 debugColor = color::brightGreen();

			inPhaseTimeout = data.optionalFloats.a;

			float boostTargetSpeed = 4.0f;

			//if ai is fighting player, then make sure to do move adjustments so that ship can create some distance between itself and player
			if (uint8_t(DF_ComboStepFlags::FIGHTING_PLAYER) & p.flags)
			{
				Ship::MoveTowardsPointArgs moveArgs{ vec3(p.myShip.getForwardDir())*5.f + p.myShip.getWorldPosition(), p.dt_sec};
				AiVsPlayer_MoveArgAdjustments(p, moveArgs);
				p.myShip.moveTowardsPoint(moveArgs);
			}

			p.myShip.setNextFrameBoost(boostTargetSpeed);

			if constexpr (ENABLE_DEBUG_LINES)
			{
				static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();

				vec3 myPos = p.myShip.getWorldPosition();
				vec3 myForward = vec3(p.myShip.getForwardDir());
				debug.renderLine(vec4(myPos, 1), myPos + myForward*5.f, debugColor);
			}
		}


		////////////////////////////////////////////////////////
		// combo processor
		////////////////////////////////////////////////////////
		size_t DogFightComboProccessor::currentGeneratedStepIdx = 0;

		void DogFightComboProccessor::resetComboList()
		{
			activeStepIdx = 0;
			activeSteps.clear();
			stepData.clear();
		}

		void DogFightComboProccessor::advanceToState(size_t idx)
		{
			assert(idx < stepData.size() && idx < activeSteps.size());
			activeStepIdx = idx;
			activeSteps[idx]->setComboData(stepData[idx]);

			//if we have another step, set its data too as it needs it for the transition
			size_t nextIdx = idx + 1;
			if (nextIdx < activeSteps.size())
			{
				assert(nextIdx < stepData.size()); //if nextIdx < activeSteps.size() is true -- then this should be true too. 
				assert(activeSteps[nextIdx] != activeSteps[idx]); //did you add the same step in a row? we can't separate their passed data if so.
				activeSteps[nextIdx]->setComboData(stepData[nextIdx]);

			}
		}

		void DogFightComboProccessor::tickStep(DogfightNodeTickData& p)
		{
			ComboStepBase* currentStep = activeStepIdx < activeSteps.size() ? activeSteps[activeStepIdx] : nullptr;
			ComboStepBase* nextStep = activeStepIdx + 1 < activeSteps.size() ? activeSteps[activeStepIdx+1] : nullptr;
			if (currentStep)
			{
				currentStep->updateTimeAlive(p);
				if (nextStep && nextStep->isInArrangement(p.arrangement) && !currentStep->inGracePeriod())
				{
					advanceToState(activeStepIdx + 1);
					nextStep->tickStep(p);
				}
				else if (currentStep->inGracePeriod())
				{
					currentStep->tickStep(p);
				}
				else //all steps are done
				{
					resetComboList();
				}
			}
		}

		DogFightComboProccessor::DogFightComboProccessor()
		{
			activeSteps.reserve(10);
			stepData.reserve(10);
		}

		////////////////////////////////////////////////////////
		// dog fight node
		////////////////////////////////////////////////////////

		namespace DogFightConstants //non-constant so values can be tweaked at runtime
		{
			//50.f, 50.f for Viscosity/speed boost is a comfortable range, but the AI doesn't shoot a lot
			//70visc and 70speedDist make it so that AI gets on average at least 1 shot in
			float AiVsPlayer_ViscosityRange = 70.f; 
			float AiVsPlayer_SpeedBoostRange = 70.f;
			extern float AiVsPlayer_MaxSpeedBoost = 4.f;
		}

		void Task_DogfightNode::notifyTreeEstablished()
		{
			rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();

			Memory& memory = getMemory();
			memory.getReplacedDelegate(target_key).addWeakObj(sp_this(), &Task_DogfightNode::handleTargetReplaced);
		}

		void Task_DogfightNode::beginTask()
		{
			Task_TickingTaskBase::beginTask();

			accumulatedTime_sec = 0.f;
			fireData = FireLaserAbilityData();

			using namespace BehaviorTree;
			Memory& memory = getMemory();
			{
				ScopedUpdateNotifier<ShipAIBrain> brain_writable;
				ScopedUpdateNotifier<TargetType> target_writable;
				if (
					memory.getWriteValueAs(brain_key, brain_writable) &&
					memory.getWriteValueAs(target_key, target_writable)
					)
				{
					ShipAIBrain& brain = brain_writable.get();
					TargetType& targetBase = target_writable.get();

					wp<GameEntity> weakTarget = targetBase.requestReference();
					if (!weakTarget.expired())
					{
						myTarget_Cache = std::dynamic_pointer_cast<Ship>(weakTarget.lock());
						assert(myTarget_Cache);
						refreshCachedTargetState();
					}

					if (Ship* myShipRaw = brain.getControlledTarget())
					{
						myShip_Cache = myShipRaw->requestTypedReference_Nonsafe<Ship>();
						assert(myShip_Cache);
					}
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
				}
			}
		}

		bool Task_DogfightNode::tick(float dt_sec)
		{
			accumulatedTime_sec += dt_sec;

			if (myTarget_Cache && myShip_Cache)
			{
				// note checking FWP may not be valid after operations are performed. EG if you destroy the target inbetween usage, the target may a dangling pointer.
				// This shouldn't happen since AI is all within the same thread and nothing done within this function should cause the ship to be destroyed. 
				// but if this node is modified to include firing, then target should be re-checked for validity after the shot sicne the shot may destroy the target.
				Ship& myTarget = *myTarget_Cache;
				Ship& myShip = *myShip_Cache;

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// Handle movement
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				const TargetDirection arrangement = calculateShipArangement(myTarget, myShip);
				if (arrangement == TargetDirection::FACING)
				{
					if (!comboProcessor.hasActiveCombo())
					{
						generateFacingCombo();
					}
				}
				//else if (arrangement == TargetDirection::BEHIND) {}
				//else if (arrangement == TargetArrangement::OPPOSING){}
				//else if (arrangement == TargetArrangement::INFRONT){}

				/*DF_ComboStepFlags*/ uint8_t flags;
				flags |= bTargetIsPlayer ? uint8_t(DF_ComboStepFlags::FIGHTING_PLAYER) : 0;
				DogfightNodeTickData tickData{ dt_sec, myTarget, myShip, arrangement, flags, *this };

				if (comboProcessor.hasActiveCombo())
				{
					comboProcessor.tickStep(tickData);
				}
				else
				{
					defaultBehavior(tickData);
				}

				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				//// Handle firing projectiles
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				//static const uint8_t fireArrangement = (uint8_t(TargetDirection::FACING) | uint8_t(TargetDirection::INFRONT));
				//if (uint8_t(arrangement) & fireArrangement)
				//{
				//	float fireCooldown = fireData.state == FireState::BURST ? fireData.MIN_TIME_BETWEEN_BURST_SHOTS() : fireData.MIN_TIME_BETWEEN_NORMAL_SHOTS();
				//	bool bCooldownUp = accumulatedTime_sec - fireData.lastFire_TimeStamp > fireCooldown;
				//	if (bCooldownUp)
				//	{
				//		myShip.fireProjectileAtShip(myTarget);
				//		fireData.lastFire_TimeStamp = accumulatedTime_sec;
				//		fireData.currentShotInBurst += fireData.state == FireState::BURST ? 1 : 0;
				//	}
				//}

				////post firing state update - update regardless if firing so se can switch between burst/normal fire states
				//if (fireData.state == FireState::BURST)
				//{
				//	if (fireData.currentShotInBurst >= fireData.burstShotsNum)
				//	{
				//		fireData.currentShotInBurst = 0;
				//		fireData.lastBurst_TimeStamp = accumulatedTime_sec;
				//		fireData.state = FireState::NORMAL;
				//		fireData.randomizeControllingParameters(*rng);
				//	}
				//}
				//else //normal fire state
				//{
				//	if (accumulatedTime_sec - fireData.lastBurst_TimeStamp > fireData.burstTimeoutDuration)
				//	{
				//		fireData.state = FireState::BURST;
				//	}
				//}

				fireData.tick(dt_sec, myShip, myTarget, arrangement, accumulatedTime_sec, *rng);
			}
			return true;
		}

		void Task_DogfightNode::handleTargetReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue)
		{
			updateTargetPointer();
		}

		void Task_DogfightNode::updateTargetPointer()
		{
			using namespace BehaviorTree;
			Memory& memory = getMemory();
			{
				ScopedUpdateNotifier<TargetType> target_writable;
				if (memory.getWriteValueAs(target_key, target_writable))
				{
					TargetType& targetRef = target_writable.get();

					//this is kinda slow, but we need it as a ship to get the forward directions. Probably should refactor the
					//methods that get forward vectors to be part of a gameplay component so it can by quickly reached.
					myTarget_Cache = std::dynamic_pointer_cast<Ship>(targetRef.requestReference().lock());
				}
				else
				{
					myTarget_Cache = nullptr;
				}
			}

			refreshCachedTargetState();
		}

		void Task_DogfightNode::generateDefensiveCombo()
		{
			static class PerfChoiceChooserHelper chooser;
			static size_t choice_SharpSix = chooser.add(1);

			comboProcessor.resetComboList();

			size_t randomChoice = chooser.getChoice(*rng);
			if (chooser.inRange(randomChoice, choice_SharpSix))
			{
				comboProcessor.addStep<SharpTurnStep>({ uint8_t(TargetDirection::BEHIND) });
				comboProcessor.addStep<SlowWhenFacingStep>({ uint8_t(TargetDirection::FACING) });
				comboProcessor.addStep<SharpTurnStep>({ uint8_t(TargetDirection::OPPOSING) });
			}
		}

		void Task_DogfightNode::generateFacingCombo()
		{
			using namespace glm;

			static class PerfChoiceChooserHelper chooser;
			static size_t choice_FaceoffDefaultRange = chooser.add(1);
			static size_t choice_boostSeparate = chooser.add(1);

			comboProcessor.resetComboList();

			size_t randomChoice = chooser.getChoice(*rng);
			if (chooser.inRange(randomChoice, choice_FaceoffDefaultRange))
			{
				comboProcessor.addStep<FaceoffCollisionAvoidanceStep>({ uint8_t(TargetDirection::FACING), vec4{0,0,0, 1.0f} }); //1.5 timeout has smooth turn, but it restores facing direction preventing combo from ending
			}
			else if (chooser.inRange(randomChoice, choice_boostSeparate))
			{
				comboProcessor.addStep<FaceoffCollisionAvoidanceStep>({ uint8_t(TargetDirection::FACING), vec4{0,0,0, 0.1f} });
				comboProcessor.addStep<BoostAwayStep>({ uint8_t(TargetDirection::OPPOSING), vec4{0,0,0, 2.f} });
			}
			else
			{
				assert(false); //didn't make a choice, what went wrong?
			}
		}

		void Task_DogfightNode::defaultBehavior(DogfightNodeTickData& p)
		{
			using namespace glm;
			vec3 myPos = p.myShip.getWorldPosition();
			vec3 targetPos = p.myTarget.getWorldPosition();

			//slow down when close to enemy to simulate aiming -- main reason is that it helps keep distance between fighters
			const float slowdownDistFar2 = 10 * 10;
			float speedFactor = glm::length2(targetPos - myPos) / slowdownDistFar2;
			speedFactor = glm::clamp<float>(speedFactor, 0.25f, 1.f);

			p.myShip.setNextFrameBoost(speedFactor); //allow gradual slow down

			Ship::MoveTowardsPointArgs moveArgs{ targetPos, p.dt_sec };
			AiVsPlayer_MoveArgAdjustments(p, moveArgs);
			p.myShip.moveTowardsPoint(moveArgs);

			if constexpr (ENABLE_DEBUG_LINES)
			{
				static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
				debug.renderLine(vec4(myPos, 1), vec4(targetPos, 1), vec4(0.25f, 0.25f, 0.25f, 1));
			}
		}

		void Task_DogfightNode::refreshCachedTargetState()
		{
			if (myTarget_Cache)
			{
				bTargetIsPlayer = myTarget_Cache->getGameComponent<OwningPlayerComponent>() && myTarget_Cache->getGameComponent<OwningPlayerComponent>()->hasOwningPlayer();
			}
			else
			{
				bTargetIsPlayer = false;
			}
		}

		TargetDirection calculateShipArangement(Ship& myTarget, Ship& myShip)
		{
			using namespace glm;

			TargetDirection arrangement = TargetDirection::FAILURE;
			vec3 tForward_n = vec3(myTarget.getForwardDir());
			vec3 mForward_n = vec3(myShip.getForwardDir());
			vec3 tarPos = myTarget.getWorldPosition();
			vec3 myPos = myShip.getWorldPosition();

			//no need to normalize as we only care about sign of dot products
			vec3 toMe = myPos - tarPos;
			vec3 toTarget = -(toMe);

			bool imBehindTarget = glm::dot(toMe, tForward_n) < 0.f;
			bool targetBehindMe = glm::dot(toTarget, mForward_n) < 0.f;

			if (imBehindTarget)
			{
				if (targetBehindMe) //  <t ------- m>
				{
					arrangement = TargetDirection::OPPOSING;
				}
				else				// <t ------- <m
				{
					arrangement = TargetDirection::INFRONT;
				}
			}
			else //target is facing me
			{
				if (targetBehindMe) // t> ------ m>
				{
					arrangement = TargetDirection::BEHIND;
				}
				else				// t> ------- <m
				{
					arrangement = TargetDirection::FACING;
				}
			}
			return arrangement;
		}

		SA::BehaviorTree::TargetDirection getArrangementWithGenericTarget(Ship& myShip, TargetType& myTarget)
		{
			using namespace glm;

			TargetDirection arrangement = TargetDirection::FAILURE;
			vec3 mForward_n = vec3(myShip.getForwardDir());
			vec3 tarPos = myTarget.getWorldPosition();
			vec3 myPos = myShip.getWorldPosition();

			//no need to normalize as we only care about sign of dot products
			vec3 toMe = myPos - tarPos;
			vec3 toTarget = -(toMe);

			bool targetBehindMe = glm::dot(toTarget, mForward_n) < 0.f;
			if (targetBehindMe) // t ------ m>
			{
				arrangement = TargetDirection::BEHIND;
			}
			else				// t ------- <m
			{
				arrangement = TargetDirection::FACING;
			}
			return arrangement;
		}

		void FireLaserAbilityData::randomizeControllingParameters(RNG& rng)
		{
			burstShotsNum = rng.getInt<uint16_t>(MIN_BURST_SHOT_RANDOMIZATION(), MAX_BURST_SHOT_RANDOMIZATION());
			burstTimeoutDuration = rng.getFloat<float>(MIN_BURST_TIMEOUT_RANDOMIZATION(), MAX_BURST_TIMEOUT_RANDOMIZATION());
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// firing ability since this is now shared
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void FireLaserAbilityData::tick(
			float dt_sec, 
			Ship& myShip, 
			TargetType& myTarget, 
			TargetDirection arrangement,
			float accumulatedTime_sec, 
			RNG& rng)
		{
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Handle firing projectiles
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			static const uint8_t fireArrangement = (uint8_t(TargetDirection::FACING) | uint8_t(TargetDirection::INFRONT));
			if (uint8_t(arrangement) & fireArrangement)
			{
				float fireCooldown = state == FireState::BURST ? MIN_TIME_BETWEEN_BURST_SHOTS() : MIN_TIME_BETWEEN_NORMAL_SHOTS();
				bool bCooldownUp = accumulatedTime_sec - lastFire_TimeStamp > fireCooldown;
				if (bCooldownUp)
				{
					myShip.fireProjectileAtShip(myTarget);
					lastFire_TimeStamp = accumulatedTime_sec;
					currentShotInBurst += state == FireState::BURST ? 1 : 0;
				}
			}

			//post firing state update - update regardless if firing so se can switch between burst/normal fire states
			if (state == FireState::BURST)
			{
				if (currentShotInBurst >= burstShotsNum)
				{
					currentShotInBurst = 0;
					lastBurst_TimeStamp = accumulatedTime_sec;
					state = FireState::NORMAL;
					randomizeControllingParameters(rng);
				}
			}
			else //normal fire state
			{
				if (accumulatedTime_sec - lastBurst_TimeStamp > burstTimeoutDuration)
				{
					state = FireState::BURST;
				}
			}
		}


		const SA::BehaviorTree::DogfightNodeTickData::TargetStats& DogfightNodeTickData::targetStats()
		{
			if (!_targetStats.has_value())
			{
				_targetStats = std::make_optional<TargetStats>();
				_targetStats->toTarget_v = myTarget.getWorldPosition() - myShip.getWorldPosition();
				_targetStats->toTargetDistance = glm::length(_targetStats->toTarget_v);
				_targetStats->toTarget_n = _targetStats->toTarget_v / _targetStats->toTargetDistance;
			}

			return *_targetStats;
		}



		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task_AttackObject
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		void Task_AttackObjective::notifyTreeEstablished()
		{
			rng = GameBase::get().getRNGSystem().getTimeInfluencedRNG();

			//Memory& memory = getMemory();
			//memory.getReplacedDelegate(target_key).addWeakObj(sp_this(), &Task_AttackObject::handleTargetReplaced);
		}

		void Task_AttackObjective::beginTask()
		{
			Task_TickingTaskBase::beginTask();

			accumulatedTime_sec = 0.f;

			state = State::SET_UP_RUN;

			//accumulatedTime_sec = 0.f;
			fireData = FireLaserAbilityData();

			using namespace BehaviorTree;
			Memory& memory = getMemory();
			{
				ScopedUpdateNotifier<ShipAIBrain> brain_writable;
				ScopedUpdateNotifier<TargetType> target_writable;
				if (
					memory.getWriteValueAs(brain_key, brain_writable) &&
					memory.getWriteValueAs(target_key, target_writable)
					)
				{
					ShipAIBrain& brain = brain_writable.get();
					TargetType& targetBase = target_writable.get();

					wp<GameEntity> weakTarget = targetBase.requestReference();
					if (!weakTarget.expired())
					{
						myTarget_Cache = std::dynamic_pointer_cast<ShipPlacementEntity>(weakTarget.lock());
						assert(myTarget_Cache);
						//refreshCachedTargetState();
					}

					if (Ship* myShipRaw = brain.getControlledTarget())
					{
						myShip_Cache = myShipRaw->requestTypedReference_Nonsafe<Ship>();
						assert(myShip_Cache);
					}
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get required memory from keys.");
				}
			}
		}

		bool Task_AttackObjective::tick(float dt_sec)
		{
			using namespace glm;

			accumulatedTime_sec += dt_sec;
			if (myShip_Cache && myTarget_Cache)
			{
				//keep in mind that target reference can be invalidated if target is somehow destroyed within this process. v2 of engine will not allow this to be possible
				TargetType& myTarget = *myTarget_Cache;
				Ship& myShip = *myShip_Cache;

				vec3 targPos = myTarget_Cache->getWorldPosition();
				vec3 myPos = myShip.getWorldPosition();

				if (myTarget.isPendingDestroy())
				{
					getMemory().replaceValue(target_key, sp<TargetType>(nullptr)); //this will invalidate target references
					//myTarget_Cache = nullptr; //doing this will prevent us from seeing debug lines and knowing if we're still looking at ship in this state
					return true; 
				}
				else
				{
					if (state == State::SET_UP_RUN)
					{
						//should only stay in the state for a single frame, so not too worried about dynamic cast.
						if (ShipPlacementEntity* targetObjective = dynamic_cast<ShipPlacementEntity*>(&myTarget))
						{
							targetData.parentLocation_p = glm::vec3(targetObjective->getParentXform() * glm::vec4(0,0,0,1.f));
							targetData.parentToObjective_v = targPos - targetData.parentLocation_p;
							vec3 bombRunOffset_n = normalize(targetData.parentToObjective_v);
							targetData.bombRunStartOffset_v = bombRunOffset_n * c.attackFromDistance;

							//adjust bomb run offset based on ship velocity; so there is some variability for hard-to-reach corners
							vec3 shipVel_n = myShip.getVelocityDir();
							vec3 rotAxis_n = glm::cross(shipVel_n, bombRunOffset_n);
							float cosTheta = glm::dot(shipVel_n, bombRunOffset_n);
							cosTheta = cosTheta < -1.f ? -cosTheta : cosTheta; //adhoc reflect projection into positive [-1,1] range, positive gives us small angle from out direction
							bombRunOffset_n = normalize(bombRunOffset_n * glm::angleAxis(glm::acos(cosTheta), rotAxis_n));
							if (!Utils::anyValueNAN(bombRunOffset_n))
							{
								targetData.bombRunStartOffset_v = bombRunOffset_n * c.attackFromDistance; //only update if we didn't hit Nan
							}
						}
						else { STOP_DEBUGGER_HERE(); } //objective that isn't a ShipPlacementEntity? what should we do about setting up an attack-run?
						state = State::MOVE_TO_SET_UP;
					}
					else if (state == State::MOVE_TO_SET_UP)
					{
						setupData.position = targPos + targetData.bombRunStartOffset_v;
						setupData.toSetup = setupData.position - myPos;

						if (bool bReadyForBombRun = glm::length2(setupData.toSetup) < Utils::square(c.acceptiableAttackRadius))
						{
							state = State::DIVE_BOMB;
							timestamp_timoutDiveBomb = accumulatedTime_sec + c.diveBombTimeoutSec;
						}

						//move regardless if we change state, in next state we will move towards placement
						const Ship::MoveTowardsPointArgs& args{setupData.position, dt_sec};
						myShip_Cache->moveTowardsPoint(args);
					}
					else if (state == State::DIVE_BOMB)
					{
						bool bTooClose = glm::distance2(targPos, myPos) < Utils::square(c.disengageObjectiveDistance);
						bool bTimedOut = accumulatedTime_sec > timestamp_timoutDiveBomb;
						if (bTooClose || bTimedOut)
						{
							state = State::SET_UP_RUN;
						}

						const Ship::MoveTowardsPointArgs& args{ targPos, dt_sec };
						myShip_Cache->moveTowardsPoint(args);
					}

					//always attempt to fire, regardless if we're setting up to do an attack run
					TargetDirection arrangement = getArrangementWithGenericTarget(myShip, myTarget);
					fireData.tick(dt_sec, myShip, myTarget, arrangement, accumulatedTime_sec, *rng);
				}

				//render debug even if we're aborting so that we can easily spot which state the ship is in.
				if constexpr (ENABLE_DEBUG_LINES)
				{
					DebugRenderSystem& db = GameBase::get().getDebugRenderSystem();
					db.renderLine(targPos, myPos, glm::vec3(1.f, 0, 0));

					db.renderLine(targPos, setupData.position, glm::vec3(0.f, 1.f, 0));

					db.renderLine(myPos, setupData.position, glm::vec3(0.f, 0.f, 1.f));

					mat4 acceptableBombRunAreaXform = glm::translate(mat4(1.f), setupData.position);
					acceptableBombRunAreaXform = glm::scale(acceptableBombRunAreaXform, vec3(c.acceptiableAttackRadius));
					db.renderSphere(acceptableBombRunAreaXform, glm::vec3(0.f, 0.5f, 0));

					mat4 bombDisengageXform = glm::translate(mat4(1.f), targPos);
					bombDisengageXform = glm::scale(bombDisengageXform, vec3(c.disengageObjectiveDistance));
					db.renderSphere(bombDisengageXform, glm::vec3(0.f, 0.f, 0.5f));
				}
			}
			return true;
		}
		SA::BehaviorTree::Task_AttackObjective::Constants Task_AttackObjective::c = Task_AttackObjective::Constants{};

	}



}

