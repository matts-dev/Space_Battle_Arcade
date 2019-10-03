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
//#include "../../GameFramework/SAGameBase.h"
//#include "../../GameFramework/SALevelSystem.h"

namespace SA
{
	namespace BehaviorTree
	{
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
				vec3 forwardDir_n = glm::normalize(vec3(myShip->getForwardDir()));
				vec3 targetDir_n = glm::normalize(targetDir);
				bool bPointingTowardsMoveLoc = glm::dot(forwardDir_n, targetDir_n) >= 0.99f;

				if (!bPointingTowardsMoveLoc)
				{
					vec3 rotationAxis_n = glm::cross(forwardDir_n, targetDir_n);

					float angleBetween_rad = Utils::getRadianAngleBetween(forwardDir_n, targetDir_n);
					float maxTurn_Rad = myShip->getMaxTurnAngle_PerSec() * dt_sec;
					float possibleRotThisTick = glm::clamp(maxTurn_Rad / angleBetween_rad, 0.f, 1.f);

					quat completedRot = Utils::getRotationBetween(forwardDir_n, targetDir_n) * xform.rotQuat;
					quat newRot = glm::slerp(xform.rotQuat, completedRot, possibleRotThisTick); 

					//roll ship -- we want the ship's right (or left) vector to match this rotation axis to simulate it pivoting while turning
					vec3 rightVec_n = newRot * vec3(1, 0, 0);
					bool bRollMatchesTurnAxis = glm::dot(rightVec_n, rotationAxis_n) >= 0.99f;

					vec3 newForwardVec_n = newRot * vec3(myShip->localForwardDir_n());
					if (!bRollMatchesTurnAxis)
					{
						float rollAngle_rad = Utils::getRadianAngleBetween(rightVec_n, rotationAxis_n);
						float rollThisTick = glm::clamp(maxTurn_Rad / rollAngle_rad, 0.f, 1.f);
						glm::quat roll = glm::angleAxis(rollThisTick, newForwardVec_n);
						newRot = roll * newRot;
					}

					Transform newXform = xform;
					newXform.rotQuat = newRot;
					myShip->setTransform(newXform);

					myShip->setVelocity(newForwardVec_n * myShip->getMaxSpeed());
				}
				else
				{
					myShip->setVelocity(forwardDir_n * myShip->getMaxSpeed());
				}
			}

			if constexpr (ENABLE_DEBUG_LINES)
			{
				static DebugRenderSystem& debug = GameBase::get().getDebugRenderSystem();
				debug.renderLine(vec4(xform.position, 1), vec4(moveLoc, 1), vec4(1, 0, 0, 1));
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
	}
}

