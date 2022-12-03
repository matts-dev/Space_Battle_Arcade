#pragma once
#include <cstdint>

#include "GameFramework/SABehaviorTree.h"
#include "GameFramework/SATimeManagementSystem.h"
#include "Game/SAShip.h"
#include "Tools/DataStructures/SATransform.h"
#include "Tools/DataStructures/AdvancedPtrs.h"
#include "Tools/DataStructures/LifetimePointer.h"



namespace SA
{
	////////////////////////////////////////////////////////
	// Forward Declarations
	////////////////////////////////////////////////////////
	class RNG;
	class ShipAIBrain;
	namespace BehaviorTree
	{
		class Task_DogfightNode;
		struct DogfightNodeTickData;
	}

	enum class MentalState_Fighter : uint32_t
	{
		EVADE, 
		ATTACK_FIGHTER,
		WANDER,
		ATTACK_OBJECTIVE,
	};

	namespace BehaviorTree
	{
		constexpr bool ENABLE_DEBUG_LINES = false;

		using TargetType = WorldEntity;
		using SecondaryTargetContainer = std::vector<lp<TargetType>>;

		////////////////////////////////////////////////////////
		// Data types for tracking current attackers.
		////////////////////////////////////////////////////////
		struct CurrentAttackerDatum
		{
			CurrentAttackerDatum(const sp<TargetType>& inAttacker) : attacker(inAttacker) {}
			fwp<TargetType> attacker;
			//extra meta data here
		};
		using ActiveAttackers = std::map<const TargetType*, CurrentAttackerDatum>;

		//constexpr bool SHIP_TREE_DEBUG_LOG = true;
		void LogShipNodeDebugMessage(const Tree& tree, const NodeBase& node, const std::string& msg);
/** These messages get compiled out and cause no performance lose when not enabled.*/
#define SHIP_TREE_DEBUG_LOG 0
#if SHIP_TREE_DEBUG_LOG
#define DEBUG_SHIP_MSG(message)\
LogShipNodeDebugMessage(this->getTree(), *this, message);
#else
#define DEBUG_SHIP_MSG(message)
#endif

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

			//Call super on the following overrides if overridden
			virtual void beginTask() override;
			virtual void taskCleanup() override;

			//override this to tick 
			//virtual bool tick(float dt_sec) override;
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
				NEARBY_HASH_CELLS, //#TODO remove these if they don't get used
				LINEAR_SEARCH,
			};

		private:
			static const bool DEBUG_TARGET_FINDER = true;
			static const bool TARGET_ATTACKERS_FEATURE_ENABLED = true; //prevents cyclic targeting
		public:
			Service_TargetFinder(const std::string& name, float tickSecs, bool bLoop, 
				const std::string& brainKey, const std::string& targetKey, const std::string& activeAttackersKey, const std::string& stateKey, const sp<NodeBase>& child)
				: Service(name, tickSecs, bLoop, child),
				brainKey(brainKey), targetKey(targetKey), activeAttackersKey(activeAttackersKey), stateKey(stateKey)
			{ }

		protected:
			virtual void serviceTick() override;
			virtual void startService() override;
			virtual void stopService() override;
			virtual void handleNodeAborted() override {}

			void handleTargetModified(const std::string& key, const GameEntity* value);
			void handleActiveAttackersModified(const std::string& key, const GameEntity* value);
			void handleTargetDestroyed(const sp<GameEntity>& entity);
			void handleStateModified(const std::string& key, const GameEntity* value);

			void resetSearchData();
			void tickFindNewTarget_slow();

			void setTarget(const sp<WorldEntity>& target, bool bCommanderAssignment = false);
		private: //utils
			bool XisTargetingY(const sp<TargetType>& x, const lp<const TargetType>& y);
			void clearPlayerSpecialCases();
			void tryApplyPlayerSpecialCases();

		private: //search data
			size_t cachedTeamIdx;
			float cachedPrefDist2;
			lp<Ship> myShip; //this should be easily refactorable to generic type if needed, ship reference can be kept too for ship specific details. changing type to worldentity->ship for avoidance
			
		private:
			const std::string brainKey;
			const std::string targetKey;
			const std::string activeAttackersKey;
			const std::string stateKey;
		private:
			ShipAIBrain* owningBrain = nullptr;
			ActiveAttackers* attackers = nullptr;
			fwp<TargetType> attackerToTarget = nullptr;
			sp<TargetType> currentTarget;
			lp<const PrimitiveWrapper<MentalState_Fighter>> stateRef = nullptr;
			float preferredTargetMaxDistance = 200.f;
			bool bCommanderProvidedTarget = false;
			bool bEvaluateActiveAttackersOnNextTick = false;
			bool bTargetIsPlayer = false;
			SearchMethod currentSearchMethod;
			float timeTicked = 0;

			//check to make sure that are target is targeting us, if not see if there is anyone that is trying to target us
			//float nextAttackerRefreshTimeStamp = 0;
			//float attackerRefreshRateSec = 5.0f;

		private: //helper data for navigating 3d world to find target over successful ticks
		};


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Servvice_AttackerSetter
		//
		// This exists so that no matter what node set or external source sets a target, the attackers data structure will
		// be updated.
		//
		// Perhaps it would be better if these were a decorator. But I would like the setting of attackers to happen 
		// on another frame from when the target is set up to prevent any sort of edge cases where recursive flip flopping 
		// between targets may happen. At time of writing, start/stopping timers for next tick slightly expensive based
		// on profiling. Having a service that ticks at a reasonable rate will smooth out any hitches and give us the benfit
		// of updating on next tick. Unfortunately it is slightly a polling mechanism. 
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Service_AttackerSetter final : public Service
		{
		public:
			Service_AttackerSetter(const std::string& nodeName,
			float tickSecs,	bool bLoop, 
			const std::string& attackersKey,
			const std::string& targetKey,
			const std::string& brainKey,
			const sp<NodeBase>& child)
				: Service(nodeName, tickSecs, bLoop, child),
				attackersKey(attackersKey),
				targetKey(targetKey),
				brainKey(brainKey)
			{ }
		
		protected:
			virtual void startService() override;
			virtual void stopService() override;
			virtual void serviceTick() override;

			void handleTargetReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue);
			
			virtual void handleNodeAborted() override {}
		private: //keys
			const std::string attackersKey;
			const std::string targetKey;
			const std::string brainKey;
		private: //data
			struct Data
			{
				//invariant
				ShipAIBrain* brain = nullptr;
				ActiveAttackers* attackers = nullptr;
				//variable
				lp<TargetType> myShip = nullptr;
				lp<TargetType> lastTarget = nullptr;
				lp<TargetType> currentTarget = nullptr;
				//state
				bool bNeedsRefresh = false;
			} data;
		};

		void cleanActiveAttackers(BehaviorTree::ActiveAttackers& attackers);

		/////////////////////////////////////////////////////////////////////////////////////
		// Service that fires projectiles when targets align with crosshairs
		/////////////////////////////////////////////////////////////////////////////////////
		class Service_OpportunisiticShots : public Service
		{
		public:
			Service_OpportunisiticShots(const std::string& name, float tickSecs, bool bLoop,
				const std::string& brainKey,
				const std::string& targetKey,
				const std::string& secondaryTargetsKey,
				const std::string& stateKey,
				const sp<NodeBase>& child)
				: Service(name, tickSecs, bLoop, child),
				brainKey(brainKey),
				targetKey(targetKey),
				secondaryTargetsKey(secondaryTargetsKey),
				stateKey(stateKey)
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
			void handleStateModified(const std::string& key, const GameEntity* value);
			void handleSecondaryTargetsReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue);

			bool canShoot() const;
			bool tryShootAtTarget(const TargetType& target, Ship* myShip, const glm::vec3& myPos, const glm::vec3& myForward_n);

		private:
			sp<RNG> rng = nullptr;
			float lastShotTimestamp = 0.f;

		private: //node properties
			const std::string brainKey;
			const std::string targetKey;
			const std::string secondaryTargetsKey;
			const std::string stateKey;
			float fireRadius_cosTheta = glm::cos(10 * glm::pi<float>()/180);
			float shootRandomOffsetStrength = 1.0f;
			float shootCooldown = 1.1f;

		private: //node cached values
			ShipAIBrain* owningBrain;
			sp<const PrimitiveWrapper<SecondaryTargetContainer>> secondaryTargets; 
			lp<const TargetType> primaryTarget = nullptr;
			lp<const PrimitiveWrapper<MentalState_Fighter>> stateRef = nullptr;
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
					const std::string& activeAttackersKey,
					const sp<NodeBase>& child) 
				: BehaviorTree::Decorator(name, child),
				stateKey(stateKey), targetKey(targetKey), activeAttackersKey(activeAttackersKey)
			{
			}

		private:
			virtual void startBranchConditionCheck() override
			{
				//this decorator is primarily an event handler
				conditionalResult = true;
			}
			virtual void notifyTreeEstablished() override;
			virtual void handleNodeAborted() override {}

			void handleDeferredStateUpdate();
			void startStateUpdateTimer();
			void stopStateUpdateTimer();

			void handleTargetModified(const std::string& key, const GameEntity* value);
			void handleActiveAttackersModified(const std::string& key, const GameEntity* value);

			void reevaluateState();
			bool targetIsAnObjective(const TargetType& target) const;
			void writeState(MentalState_Fighter currentState, MentalState_Fighter newState, Memory& memory);
		private:
			const float deferrStateTime = 0.25f;
			sp<MultiDelegate<>> deferredStateChangeTimer;
			sp<RNG> rng;
		private:
			const std::string stateKey;
			const std::string targetKey;
			const std::string activeAttackersKey;
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


		////////////////////////////////////////////////////////
		// Task_FindDogfightLocation
		////////////////////////////////////////////////////////
		class Task_FindDogfightLocation : public Task
		{
		public:
			Task_FindDogfightLocation(const std::string& name,
				const std::string& writeLocationKey,
				const std::string& brainKey
			) : Task(name),
				writeLocationKey(writeLocationKey),
				brainKey(brainKey)
			{
			}
		public:
			virtual void beginTask() override;
			
		protected:
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};
			virtual void postConstruct() override;
		private:
			glm::vec3 getRandomPointNear(glm::vec3 point);
		private:
			sp<RNG> rng;
		private:
			const std::string writeLocationKey;
			const std::string brainKey;
			float maxRandomPointRadius = 300.f;
			float minRandomPointRadius = 10.f;
			float maxTravelDistance = 200.f;
			float cacheMaxTravelDistance2 = maxTravelDistance * maxTravelDistance;
		};

		////////////////////////////////////////////////////////
		// Task_EvadePatternBase
		////////////////////////////////////////////////////////
		class Task_EvadePatternBase : public Task_TickingTaskBase
		{
		public:
			Task_EvadePatternBase(const std::string& name,
				const std::string& targetLocKey,
				const std::string& brainKey,
				const float timeoutSecs
			): Task_TickingTaskBase(name),
				targetLocKey(targetLocKey),
				brainKey(brainKey),
				timeoutSecs(timeoutSecs)
			{	}
		protected:
			virtual void handleNodeAborted() override {}

			virtual void childSetup() = 0;
			virtual void beginTask() override;
			virtual void taskCleanup() override;
			virtual bool tick(float dt_sec) override;
			virtual void tickPattern(float dt_sec) = 0;

			virtual void postConstruct() override;

		private: //node state
			float accumulatedTime = 0.f;
		protected: //node state
			fwp<Ship> myShip;
			sp<RNG> rng;
			struct MyPOD
			{
				glm::vec3 myShipStartLoc = glm::vec3(0.f);
				glm::vec3 myTargetLoc = glm::vec3(0.f);
				glm::vec3 startToTarget_n = glm::vec3(0.f);
				float totalTravelDistance = 0.f;
			} base; //data
		private: //node properties
			const std::string targetLocKey;
			const std::string brainKey;
			float timeoutSecs;
		};
		
		/////////////////////////////////////////////////////////////////////////////////////
		// Evade pattern spiral
		/////////////////////////////////////////////////////////////////////////////////////
		class Task_EvadePatternSpiral : public Task_EvadePatternBase
		{
		public:
			Task_EvadePatternSpiral(const std::string& name,
				const std::string& targetLocKey,
				const std::string& brainKey,
				const float timeoutSecs
			) : Task_EvadePatternBase(name,targetLocKey,brainKey,timeoutSecs)
			{
			}
		protected:
			virtual void childSetup() override;
			virtual void tickPattern(float dt_sec) override;
			virtual void taskCleanup() override;
		private:
			// distIncrement, rotationDegrees, and cylinderRadius have relationship that if one is a small value, all should be small value.
			const float pointDistanceIncrement = 0.99f;
			const float cylinderRotationIncrement_rad = 35.0f * (3.1415f/180);
			const float cylinderRadius = 0.99f;
			float cylinderRotDirection = 1.0f;
			struct SpiralData
			{
				glm::vec3 nextPnt = glm::vec3(0.f);
				glm::vec3 right_n = glm::vec3(0.f);
				float cylinderRotation_rad = 0.f;
				float nextPointCalculationDistance = 0.f;
				float speedBoost = 1.f;
				bool bFirstTick = true;
			}spiral;

		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Evade pattern spin
		/////////////////////////////////////////////////////////////////////////////////////
		class Task_EvadePatternSpin : public Task_EvadePatternBase
		{
		public:
			Task_EvadePatternSpin(const std::string& name,
				const std::string& targetLocKey,
				const std::string& brainKey,
				const float timeoutSecs
			) : Task_EvadePatternBase(name, targetLocKey, brainKey, timeoutSecs)
			{
			}
		protected:
			virtual void childSetup() override;
			virtual void tickPattern(float dt_sec) override;
			virtual void taskCleanup() override;
		private:
			const float pointDistanceIncrement = 20;
			float spinDir = 1.0f;
			float cylinderDir = 1.0f;
			const float cylinderRotationIncrement_rad = 105.0f * (3.1415f / 180);
			const float cylinderRadius = 0.49f;
			struct SpinData
			{
				glm::vec3 nextPnt = glm::vec3(0.f);
				glm::vec3 right_n = glm::vec3(0.f);
				float cylinderRotation_rad = 0.f;
				float rollspinIncrement_rad = 200.0f * (3.1415f / 180);
				float nextPointCalculationDistance = 0.f;
				bool bFirstTick = true;
			}spin;

		};

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// DogfightNode
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		void AiVsPlayer_MoveArgAdjustments(BehaviorTree::DogfightNodeTickData& p, Ship::MoveTowardsPointArgs& moveArgs);
		float getAIVsPlayer_Viscosity(float variabilityMultiplier, DogfightNodeTickData& p);
		float getAIVsPlayer_MoveSpeed(float variabilityMultiplier, DogfightNodeTickData& p); 

		enum class TargetDirection : uint8_t { FAILURE=0, INFRONT=1, BEHIND=1<<1, FACING=1<<2, OPPOSING=1<<3};
		enum class DF_ComboStepFlags : uint8_t { NONE=0, FIGHTING_PLAYER=1<<0 };

		struct ComboStepData
		{
			/*TargetDirection*/   uint8_t arrangements_bitvector = 0;
			glm::vec4 optionalFloats = glm::vec4(0.f);
		};

		struct ComboStepBase
		{
		public:
			ComboStepBase(){}

		public:
			void updateTimeAlive(DogfightNodeTickData& tickData);
			bool isInArrangement(TargetDirection arrangement) const;

			void setComboData(const ComboStepData& inComboData);
			virtual bool inGracePeriod() const;
			virtual void tickStep(DogfightNodeTickData& p) {}

		protected:
			ComboStepData data;
			float wrongPhaseTimeout = 0.1f;
			std::optional<float> inPhaseTimeout;
			struct BasePOD
			{
				float totalActiveTime = 0.f;
				float timeInWrongPhase = 0.f;
				bool bForceStepEnd = false;
			} basePOD;
		};

		////////////////////////////////////////////////////////
		// sharp turn
		////////////////////////////////////////////////////////
		class SharpTurnStep : public ComboStepBase
		{
		public:
			SharpTurnStep() : ComboStepBase(){}
			virtual void tickStep(DogfightNodeTickData& p) override;
		};

		////////////////////////////////////////////////////////
		// slow when facing
		////////////////////////////////////////////////////////
		class SlowWhenFacingStep : public ComboStepBase
		{
		public:
			SlowWhenFacingStep() : ComboStepBase() {}
			virtual void tickStep(DogfightNodeTickData& p) override;

		private:
			float slowdownDist2 = 20 * 20;
		};

		////////////////////////////////////////////////////////
		// Faceoff collision avoidance
		////////////////////////////////////////////////////////
		class FaceoffCollisionAvoidanceStep : public ComboStepBase
		{
		public:
			FaceoffCollisionAvoidanceStep() : ComboStepBase() {}
			virtual void tickStep(DogfightNodeTickData& p) override;

		private:
			float startAvoidDist2 = 20 * 20;
		};

		////////////////////////////////////////////////////////
		// Faceoff boost after avoidance
		////////////////////////////////////////////////////////
		class BoostAwayStep : public ComboStepBase
		{
		public:
			BoostAwayStep() : ComboStepBase(){}
			virtual void tickStep(DogfightNodeTickData& p) override;

		private:
			float startAvoidDist2 = 20 * 20;
		};
		
		////////////////////////////////////////////////////////
		// follow and attack
		////////////////////////////////////////////////////////
		class FollowAndAttackStep : public ComboStepBase
		{
		public:
			FollowAndAttackStep() : ComboStepBase() { }
			virtual void tickStep(DogfightNodeTickData& p) override;
		};


		/**
			Processes combo steps. Combo steps can be used to define a series of actions given an arrangment of two fighting ships.

			notes about efficiency:
				Defining this with a normal behavior tree node structure is very slow because of all the tree node changes per tick.
				Thus this is all done internally to the ticking DogfightNode (ie the outer of this class)
				My optimizations make this a little strange to read. please see below.

				For efficiency, these steps are shared per instance (see getSharedStepObj). This avoid new/mallocs. But means that we
				must pass data to the step object when it gets activated. The same step may appear in a combo list; eg [stepA, stepB, stepA].
				passing [StepA, StepA] is an error because we cannot set up the nextStep data and currentStep data because they steps are actually one shared object.
			
				ComboStepData is the data each step has access to. To override values in ComboStepBase, one can populate the optionalFloats vector
				and override the value in its tick function.
		 */
		class DogFightComboProccessor
		{
		public:
			DogFightComboProccessor();

			/////////////////////////////////////////////////////////////////////////////////////
			// Current combo list
			/////////////////////////////////////////////////////////////////////////////////////
		public:
			template<typename Step>
			void addStep(const ComboStepData& data)
			{
				static_assert(std::is_base_of<ComboStepBase, Step>::value);
				activeSteps.push_back(getSharedStepObj<Step>());
				stepData.push_back(data);

				//if a second step is added, we need to set up its data too so we can detect if we should transition.
				if (activeSteps.size() == 1 || activeSteps.size() == 2) { advanceToState(0); }
			}
			void tickStep(DogfightNodeTickData& tickData);
			void resetComboList();
			bool hasActiveCombo() { return activeSteps.size() > 0; }
		private:
			void advanceToState(size_t idx);
		private:
			size_t activeStepIdx = 0;
			std::vector<ComboStepBase*> activeSteps;
			std::vector<ComboStepData> stepData;

			/////////////////////////////////////////////////////////////////////////////////////
			// Shared Step Storage
			/////////////////////////////////////////////////////////////////////////////////////
		private:
			template<typename STEP>
			STEP* getSharedStepObj()
			{
				static size_t typedStepIdx = currentGeneratedStepIdx++;	//a different index for every type requested from this function
				if (typedStorage.size() <= typedStepIdx) { typedStorage.resize(typedStepIdx + 1, nullptr); }
				if (typedStorage[typedStepIdx] == nullptr) { typedStorage[typedStepIdx] = new_sp<STEP>(); }
				return static_cast<STEP*>(typedStorage[typedStepIdx].get());
			}
			static size_t currentGeneratedStepIdx;			
			std::vector<sp<ComboStepBase>> typedStorage;	
		};

		////////////////////////////////////////////////////////
		// Dogfight node
		////////////////////////////////////////////////////////
		struct DogfightNodeTickData
		{
			DogfightNodeTickData(
				float dt_sec
				,Ship& myTarget
				,Ship& myShip
				,TargetDirection arrangement
				,/*DF_ComboStepFlags*/ uint8_t flags
				, BehaviorTree::Task_DogfightNode& owningDFNode
			) :
				dt_sec(dt_sec)
				,myTarget(myTarget)
				,myShip(myShip)
				, arrangement(arrangement)
				,flags(flags) 
				, owningDFNode(owningDFNode)
			{}

			struct TargetStats
			{
				glm::vec3 toTarget_v;
				glm::vec3 toTarget_n;
				float toTargetDistance;
			};
		private:
			std::optional<TargetStats> _targetStats;
		public:
			////////////////////////////////////////////////////////
			// Data
			////////////////////////////////////////////////////////
			float dt_sec;
			Ship& myTarget;
			Ship& myShip;
			TargetDirection arrangement;
			/*DF_ComboStepFlags*/ uint8_t flags;
			class Task_DogfightNode& owningDFNode;
			const TargetStats& targetStats();
		};

		namespace DogFightConstants
		{
			extern float AiVsPlayer_ViscosityRange; //how far away an AI ship needs to be before all viscosity is lost.
			extern float AiVsPlayer_SpeedBoostRange; //how far away an AI ship needs to be before all viscosity is lost.
			extern float AiVsPlayer_MaxSpeedBoost;
		}

		enum class FireState : uint8_t { NORMAL, BURST };
		struct FireLaserAbilityData
		{
			float lastFire_TimeStamp = 0;
			float burstTimeoutDuration = 1.0f;
			float lastBurst_TimeStamp = 0.f;
			uint16_t currentShotInBurst = 0;
			uint16_t burstShotsNum = 3;
			FireState state = FireState::BURST;

			static inline float MIN_BURST_TIMEOUT_RANDOMIZATION() { return 1.f; }
			static inline float MAX_BURST_TIMEOUT_RANDOMIZATION() { return 2.f; }
			static inline uint16_t MIN_BURST_SHOT_RANDOMIZATION() { return 2; }
			static inline uint16_t MAX_BURST_SHOT_RANDOMIZATION() { return 5; }
			static inline float MIN_TIME_BETWEEN_NORMAL_SHOTS() { return 0.50f; }
			static inline float MIN_TIME_BETWEEN_BURST_SHOTS() { return 0.10f; }
			void randomizeControllingParameters(RNG& rng);
			void tick(float dt_sec, Ship& myShip, TargetType& myTarget, TargetDirection arrangment, float accumulatedTime_sec, RNG& rng);
		};

		TargetDirection calculateShipArangement(Ship& myTarget, Ship& myShip);

		class Task_DogfightNode : public Task_TickingTaskBase
		{
		public:
			Task_DogfightNode(const std::string& nodeName,
				const std::string& brain_key,
				const std::string& target_key,
				const std::string& secondaryTarget_key
			): Task_TickingTaskBase(nodeName),
				brain_key(brain_key),
				target_key(target_key),
				secondaryTarget_key(secondaryTarget_key)
			{	}
		public:
			inline float getVariabilityIntervalSec() const { return variabilityIntervalSec;}
			inline float getAccumulatedTime() const { return accumulatedTime_sec; }
		protected:
			virtual void notifyTreeEstablished() override;
			virtual void beginTask() override;
			virtual void taskCleanup() override { Task_TickingTaskBase::taskCleanup(); };
			virtual bool tick(float dt_sec) override;
		private:
			void handleTargetReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue);
			void updateTargetPointer();
			void generateDefensiveCombo();
			void generateFacingCombo();
			void defaultBehavior(DogfightNodeTickData& tick);
			void refreshCachedTargetState();
		private:
		private:
			float accumulatedTime_sec = 0;
			sp<RNG> rng;
		private: 
			FireLaserAbilityData fireData;
		private: //cached values
			fwp<Ship> myTarget_Cache = nullptr;
			fwp<Ship> myShip_Cache = nullptr;
			DogFightComboProccessor comboProcessor;
			bool bTargetIsPlayer = false;
		private: //node properties
			const std::string brain_key;
			const std::string target_key;
			const std::string secondaryTarget_key;
			float variabilityIntervalSec = 3.0f;
		};

		TargetDirection getArrangementWithGenericTarget(Ship& myShip, TargetType& myTarget);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// task for targeting and attacking carrier objectives
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Task_AttackObjective : public Task_TickingTaskBase
		{
			enum class State : uint8_t { MOVE_TO_SET_UP, SET_UP_RUN, DIVE_BOMB };
		public:
			Task_AttackObjective(const std::string& nodeName,
				const std::string& brain_key,
				const std::string& target_key
			) : Task_TickingTaskBase(nodeName),
				brain_key(brain_key),
				target_key(target_key)
			{	}
		public:
		protected:
			virtual void notifyTreeEstablished() override;
			virtual void beginTask() override;
			virtual void taskCleanup() override { Task_TickingTaskBase::taskCleanup(); };
			virtual bool tick(float dt_sec) override;
		private:
			
			//void handleTargetReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue);
		public:
			struct Constants
			{
				float attackFromDistance = 92.f;
				float acceptiableAttackRadius = 25.f;
				float disengageObjectiveDistance = 25.f;
				float diveBombTimeoutSec = 5.f;
			};
			static Constants c; //non-const to allow runtime manipulation through tools
		private: //transient data
			fwp<class ShipPlacementEntity> myTarget_Cache = nullptr;
			fwp<Ship> myShip_Cache = nullptr;
			struct TargetDataCache
			{
				glm::vec3 parentLocation_p;
				glm::vec3 parentToObjective_v;
				glm::vec3 bombRunStartOffset_v;
			} targetData;
			struct SetupData
			{
				glm::vec3 position{ 0.f };
				glm::vec3 toSetup{ 0.f };
			} setupData;
			FireLaserAbilityData fireData;
			float accumulatedTime_sec = 0.f;
			float timestamp_timoutDiveBomb = 0.f;
			State state = State::SET_UP_RUN;
		private: //node properties
			const std::string brain_key;
			const std::string target_key;
			sp<RNG> rng = nullptr;
		};

	}
}