#pragma once
#include "../../GameFramework/SABehaviorTree.h"
#include "../../GameFramework/SATimeManagementSystem.h"
#include "../SAShip.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../Tools/DataStructures/AdvancedPtrs.h"



namespace SA
{
	class RNG;
	class ShipAIBrain;

	enum class MentalState_Fighter : uint32_t
	{
		EVADE, 
		FIGHT,
		WANDER,
	};

	namespace BehaviorTree
	{
		constexpr bool ENABLE_DEBUG_LINES = true;
		using TargetType = WorldEntity;

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

		/////////////////////////////////////////////////////////////////////////////////////
		// Task that automatically registers ticker and calls a tick function
		//
		//	#concern api: child tasks can be misused and return false in tick, which will auto-unregister this.
		/////////////////////////////////////////////////////////////////////////////////////
		class Task_TickingTaskBase : public Task, public ITickable
		{
		public:
			Task_TickingTaskBase(const std::string& name) : Task(name)
			{}

		protected:
			virtual void handleNodeAborted() override {}
			virtual void beginTask() override;
			virtual void taskCleanup() override;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Task follow target.
		//		Indefinitely means this take will not complete unless aborted.
		/////////////////////////////////////////////////////////////////////////////////////
		class Task_Ship_FollowTarget_Indefinitely : public Task_TickingTaskBase
		{
			using TargetType = WorldEntity;
		public:
			Task_Ship_FollowTarget_Indefinitely(
				const std::string& nodeName,
				const std::string& shipBrain_MemoryKey,
				const std::string& target_MemoryKey,
				const std::string& activeAttackers_MemoryKey
			) : Task_TickingTaskBase(nodeName),
				shipBrain_MemoryKey(shipBrain_MemoryKey),
				target_MemoryKey(target_MemoryKey),
				activeAttackers_MemoryKey(activeAttackers_MemoryKey)
			{}
		protected:
			virtual void beginTask() override;
			virtual void taskCleanup() override;
			virtual bool tick(float dt_sec) override;

		private:
			void handleTargetDestroyed(const sp<GameEntity>& target);

		private:
			sp<Ship> myShip = sp<Ship>(nullptr);
			sp<TargetType> myTarget = sp<TargetType>(nullptr); //#TOOD #releasing_ptr 
			float prefDistTarget2 = 100.0f;

		private: //node properties
			const std::string shipBrain_MemoryKey;
			const std::string target_MemoryKey;
			const std::string activeAttackers_MemoryKey;
			float preferredDistanceToTarget = 30.0f;
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Service target finder
		/////////////////////////////////////////////////////////////////////////////////////
		class Service_TargetFinder : public Service
		{
			enum class SearchMethod
			{
				COMMANDER_ASSIGNED,
				NEARBY_HASH_CELLS,
				LINEAR_SEARCH,
			};

		private:
			static const bool DEBUG_TARGET_FINDER = true;
		public:
			Service_TargetFinder(const std::string& name, float tickSecs, bool bLoop, 
				const std::string& brainKey, const std::string& targetKey, const sp<NodeBase>& child)
				: Service(name, tickSecs, bLoop, child),
				brainKey(brainKey), targetKey(targetKey)
			{ }

		protected:
			virtual void serviceTick() override;
			virtual void startService() override;
			virtual void stopService() override;
			virtual void handleNodeAborted() override {}

			void handleTargetModified(const std::string& key, const GameEntity* value);
			void handleTargetDestroyed(const sp<GameEntity>& entity);
			void resetSearchData();
			void tickFindNewTarget_slow();

			void setTarget(const sp<WorldEntity>& target);

		private: //search data
			size_t cachedTeamIdx;
			float cachedPrefDist2;

		private:
			std::string brainKey;
			std::string targetKey;
			const ShipAIBrain* owningBrain;
			sp<WorldEntity> currentTarget;
			float preferredTargetMaxDistance = 50.f;
			SearchMethod currentSearchMethod;

		private: //helper data for navigating 3d world to find target over successful ticks
		};

		////////////////////////////////////////////////////////
		// Data types for tracking current attackers.
		////////////////////////////////////////////////////////
		struct CurrentAttackerDatum
		{
			CurrentAttackerDatum(const sp<TargetType>& inAttacker) : attacker(inAttacker) {}
			fwp<TargetType> attacker;
		};
		using CurrentAttackers = std::map<WorldEntity*, CurrentAttackerDatum>;

		/////////////////////////////////////////////////////////////////////////////////////
		// Service that fires projectiles when targets align with crosshairs
		/////////////////////////////////////////////////////////////////////////////////////
		class Service_OpportunisiticShots : public Service
		{
		public:
			Service_OpportunisiticShots(const std::string& name, float tickSecs, bool bLoop,
				const std::string& brainKey, const std::string& targetKey, const std::string& secondaryTargetsKey, const sp<NodeBase>& child)
				: Service(name, tickSecs, bLoop, child),
				brainKey(brainKey), targetKey(targetKey), secondaryTargetsKey(secondaryTargetsKey)
			{
			}

		protected:
			virtual void startService() override;
			virtual void stopService() override;
			virtual void serviceTick() override;
			virtual void handleNodeAborted() override {}


			virtual void postConstruct() override;

		private:
			void handleTargetModified(const std::string& key, const GameEntity* value);
			void handleSecondaryTargetsReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue);

			bool canShoot() const;
			bool tryShootAtTarget(const TargetType& target, const Ship* myShip, const glm::vec3& myPos, const glm::vec3& myForward_n);

		private:
			sp<RNG> rng = nullptr;
			float lastShotTimestamp = 0.f;

		private: //node properties
			std::string brainKey;
			std::string targetKey;
			std::string secondaryTargetsKey;
			float fireRadius_cosTheta = glm::cos(10 * glm::pi<float>()/180);
			float shootRandomOffsetStrength = 1.0f;
			float shootCooldown = 1.1f;

		private: //node cached values
			const ShipAIBrain* owningBrain;
			sp<const PrimitiveWrapper<std::vector<sp<TargetType>>>> secondaryTargets;
			sp<const TargetType> primaryTarget = nullptr; //#todo #releasing_ptr
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Decorator that responds to events to change the top-level state of the tree
		//	-primarily an event handler that always returns true so child trees can execute.
		//	-monitors memory to see if state changes should be applied.
		/////////////////////////////////////////////////////////////////////////////////////
		class Decorator_FighterStateSetter : public BehaviorTree::Decorator
		{
		public:
			Decorator_FighterStateSetter(
					const std::string& name, 
					const std::string& stateKey, 
					const std::string& targetKey,
					const sp<NodeBase>& child) 
				: BehaviorTree::Decorator(name, child),
				stateKey(stateKey), targetKey(targetKey)
			{
			}

		private:
			virtual void startBranchConditionCheck()
			{
				//this decorator is primarily an event handler
				conditionalResult = true;
			}
			virtual void notifyTreeEstablished() override;

			virtual void handleNodeAborted() override {}

			void handleTargetModified(const std::string& key, const GameEntity* value);
			void reevaluateState();
			void writeState(MentalState_Fighter newState, Memory& memory);

		private:
			const std::string stateKey;
			const std::string targetKey;
		};


		/////////////////////////////////////////////////////////////////////////////////////
		// Task to get target location and write it to memory
		/////////////////////////////////////////////////////////////////////////////////////
		class Task_Ship_GetEntityLocation : public Task
		{
			//typedef is more flexible because the type of target may change from being a world entity in future
			using EntityType = WorldEntity;

		public:
			Task_Ship_GetEntityLocation(
					const std::string& name,
					const std::string& entityKey,
					const std::string& writeLocKey)
				: Task(name),
				entityKey(entityKey),
				writeLocKey(writeLocKey)
			{}
		public:
			virtual void beginTask() override;
		protected:
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};
		private:
			const std::string entityKey;
			const std::string writeLocKey;
		};
	}
}