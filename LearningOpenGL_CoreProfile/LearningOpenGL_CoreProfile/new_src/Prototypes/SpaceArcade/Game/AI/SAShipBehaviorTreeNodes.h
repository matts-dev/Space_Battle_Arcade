#pragma once
#include "../../GameFramework/SABehaviorTree.h"
#include "../../GameFramework/SATimeManagementSystem.h"
#include "../SAShip.h"

namespace SA
{
	class RNG;

	namespace BehaviorTree
	{
		constexpr bool ENABLE_DEBUG_LINES = true;


		/////////////////////////////////////////////////////////////////////////////////////
		// task find random location within radius
		/////////////////////////////////////////////////////////////////////////////////////
		class Task_FindRandomLocationNearby : public Task
		{
		public:
			Task_FindRandomLocationNearby(
				const std::string& outputLocation_MemoryKey,
				const std::string& inputLocation_MemoryKey,
				const float radius
			) : Task("task_random_location_nearby"),
				outputLocation_MemoryKey(outputLocation_MemoryKey),
				inputLocation_MemoryKey(inputLocation_MemoryKey),
				radius(radius)
			{}
		protected:
			virtual void postConstruct() override;
		public:
			virtual void beginTask() override;

		protected:
			/* Node returns immediately, so no need to cancel any timers or anything. */
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};

		private:
			const std::string outputLocation_MemoryKey;
			const std::string inputLocation_MemoryKey;
			const float radius;
			sp<RNG> rng;
		};


		/////////////////////////////////////////////////////////////////////////////////////
		// task save current location
		/////////////////////////////////////////////////////////////////////////////////////
		class Task_Ship_SaveShipLocation : public Task
		{
		public:
			Task_Ship_SaveShipLocation(
				const std::string& outputLocation_MemoryKey,
				const std::string& shipBrain_MemoryKey
			) : Task("task_find_random_location_nearby"),
				outputLocation_MemoryKey(outputLocation_MemoryKey),
				shipBrain_MemoryKey(shipBrain_MemoryKey)
			{}
		public:
			virtual void beginTask() override;
		protected:
			virtual void handleNodeAborted() override {}	//immediate return; no need to cancel timers
			virtual void taskCleanup() override {};
		private:
			const std::string outputLocation_MemoryKey;
			const std::string shipBrain_MemoryKey;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// task move to location
		/////////////////////////////////////////////////////////////////////////////////////
		class Task_Ship_MoveToLocation : public Task, public ITickable
		{
		public:
			Task_Ship_MoveToLocation(
				const std::string& shipBrain_MemoryKey,
				const std::string& targetLoc_MemoryKey,
				const float timeoutSecs
			) : Task("task_ship_move_to_location"),
				shipBrain_MemoryKey(shipBrain_MemoryKey),
				targetLoc_MemoryKey(targetLoc_MemoryKey),
				timeoutSecs(timeoutSecs)
			{
			}
		public:
			virtual void beginTask() override;
			virtual void taskCleanup() override;

		protected:
			virtual void handleNodeAborted() override;
			virtual bool tick(float dt_sec) override;

		private:
			//memory keys
			const std::string shipBrain_MemoryKey;
			const std::string targetLoc_MemoryKey;
			const float timeoutSecs;

			//cached state
			sp<Ship> myShip = sp<Ship>(nullptr);
			glm::vec3 moveLoc;

			//thresholds
			const float atLocThresholdLength2 = 0.1f;
			float accumulatedTime = 0;
		};

	}
}