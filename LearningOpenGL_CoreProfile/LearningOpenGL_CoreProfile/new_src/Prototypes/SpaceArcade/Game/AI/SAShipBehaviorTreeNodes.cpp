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

			evaluationResult = false;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		// task move to location
		/////////////////////////////////////////////////////////////////////////////////////

		void Task_Ship_MoveToLocation::beginTask()
		{
			static LevelSystem& levelSystem = GameBase::get().getLevelSystem();
			if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
			{
				const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager();
				// create ticker
				//

			}
			else
			{
				log("Task_Ship_MoveToLocation", LogLevel::LOG_ERROR, "could not get current level");
				evaluationResult = false;
			}
		}

		void Task_Ship_MoveToLocation::handleTick(float dt_sec)
		{
			//check if at location (or give up after certain time?)
		}

		void Task_Ship_MoveToLocation::handleNodeAborted()
		{
			//cancel ticker
		}

	}
}

