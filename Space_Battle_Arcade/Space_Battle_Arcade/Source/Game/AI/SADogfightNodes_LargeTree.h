#pragma once
#include <string>
#include <vector>
#include "Tools/DataStructures/AdvancedPtrs.h"
#include "GameFramework/SABehaviorTree.h"
#include "GameFramework/SAWorldEntity.h"
#include "SAShipBehaviorTreeNodes.h"

namespace SA
{
	class Ship;
	class ShipAIBrain;

	namespace BehaviorTree
	{
		class BehaviorTreeBrain;

		enum class TargetIs : uint8_t
		{
			BEHIND,
			INFRONT,
			FACING,
			OPPOSING
		};
		using Phases = std::vector<TargetIs>;

		/////////////////////////////////////////////////////////////////////////////////////
		// Data for making requests on driving ship
		/////////////////////////////////////////////////////////////////////////////////////
		struct ShipInputRequest
		{
			std::optional<float> speedAdjustment;
			std::optional<float> rollOverride_rad;
			std::optional<bool> bUseAvailableBoost;
			std::optional<glm::vec3> targetPoint;
			bool bDisableRoll = false;
			glm::vec3 debugColor { 0.5f, 0.5f, 0.5f };
		};

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ComboStepBase  - The base class for any custom combo step
		//
		//	Combosteps encapsulate behavior for a certain phase/orientation of combat.
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class ComboStepBase_VerboseTree : public GameEntity
		{
		public:
			ComboStepBase_VerboseTree(std::vector<TargetIs>&& targetPhases) 
				: targetPhases(std::move(targetPhases)) {}
			//void init();
			bool isInPhase(TargetIs arrangement) const;
			void updateTimeAlive(float dt_sec, TargetIs arrangement);
			virtual bool inGracePeriod() const { return basePOD.timeInWrongPhase < wrongPhaseTimeout && !basePOD.bForceStepEnd;  }
			virtual void doAction(float dt_sec, TargetIs arrangement, const fwp<Ship>& myTarget, const fwp<Ship>& myShip, const fwp<ShipInputRequest>& inputRequestLoc){}
		protected:
			//virtual void onInit() {}
		protected:
			std::vector<TargetIs> targetPhases;
			float wrongPhaseTimeout = 0.1f;
			struct BasePOD
			{
				float totalActiveTime = 0.f;
				float timeInWrongPhase = 0.f;
				bool bForceStepEnd = false;
			} basePOD;
		};

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// list of combo steps to go through
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class ComboList
		{
		public:
			const sp<ComboStepBase_VerboseTree>& currentStep() { return activeIdx < steps.size() ? steps[activeIdx] : nullopt; }
			const sp<ComboStepBase_VerboseTree>& nextStep() { return activeIdx + 1 < steps.size() ? steps[activeIdx + 1] : nullopt; }
			void advanceStep();
			void clear();
			void addStep(const sp<ComboStepBase_VerboseTree>& nextStep);
		private:
			static sp<ComboStepBase_VerboseTree> nullopt;
			std::vector<sp<ComboStepBase_VerboseTree>> steps;
			size_t activeIdx = 0;
		};



		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// TargetDependentTask
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class DogfightTaskBase : public Task
		{
		public:
			DogfightTaskBase(const std::string& nodeName,
				const std::string& targetArrangement_Key,
				const std::string& brain_Key,
				const std::string& target_Key
			) : Task(nodeName),
				targetArrangement_Key(targetArrangement_Key),
				brain_Key(brain_Key),
				target_Key(target_Key)
			{}
		protected:
			virtual void notifyTreeEstablished() override;
			void updateTargetPointer();
			void updateMyShipPtr();
		private:
			void handleTargetReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue);
		protected: //node properties
			const std::string targetArrangement_Key;
			const std::string brain_Key;
			const std::string target_Key;
		protected: //cached values
			fwp<TargetIs> targetArrangment;
			fwp<ShipAIBrain> myBrain;
			fwp<Ship> myShip;
			fwp<Ship> myTarget;//#refactor move forward_dir code into a gameplay component so we don't have to dynamic cast this.
		};

		////////////////////////////////////////////////////////
		// Task_ComboProcessorBase
		////////////////////////////////////////////////////////
		class Task_ComboProcessor : public DogfightTaskBase
		{
		public:
			Task_ComboProcessor(const std::string& nodeName,
				const std::string& comboListKey,
				const std::string& inputRequestKey,
				TargetIs arrangement,
				const std::string& targetArrangement_Key,
				const std::string& brain_Key,
				const std::string& target_Key
			) : DogfightTaskBase(nodeName, targetArrangement_Key, brain_Key, target_Key),
				arrangement(arrangement),
				comboListKey(comboListKey),
				inputRequestKey(inputRequestKey)
			{
			}
		protected:
			virtual void notifyTreeEstablished() override;
			virtual void beginTask() override;
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};
		protected:
		private:
			fwp<ComboList> comboList;
			fwp<ShipInputRequest> inputRequestLoc;
			TargetIs arrangement;
		private: //node properties
			const std::string comboListKey;
			const std::string inputRequestKey;
		};

		////////////////////////////////////////////////////////
		// Task_StartAvoidanceCombo
		////////////////////////////////////////////////////////
		class Task_StartDefenseCombo : public Task
		{
		public:
			Task_StartDefenseCombo(const std::string& nodeName,
				const std::string& comboList_Key
			): Task(nodeName),
				comboList_Key(comboList_Key)
			{	}
		protected:
			virtual void beginTask() override;
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};

			virtual void postConstruct() override;

		private:
			const std::string comboList_Key;
		private:
			fwp<ComboList> comboList;
			sp<RNG> rng;
		};

		////////////////////////////////////////////////////////
		// DefaultShipDrive_DF
		////////////////////////////////////////////////////////
		class Task_DefaultDogfightInput : public DogfightTaskBase
		{
		public:
			Task_DefaultDogfightInput(const std::string& nodeName,
				const std::string& inputRequest_key,
				const std::string& targetArrangement_Key,
				const std::string& brain_Key,
				const std::string& target_Key
			): DogfightTaskBase(nodeName, targetArrangement_Key, brain_Key, target_Key),
				inputRequest_key(inputRequest_key)
			{	}
		protected:
			virtual void notifyTreeEstablished() override;
			virtual void beginTask() override;
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};
		private: //node properties 
			const std::string inputRequest_key;
		private:
			fwp<ShipInputRequest> inputRequest;
		};


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Decorator_TargetIs
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Decorator_TargetIs : public BehaviorTree::Decorator
		{
		public:
			Decorator_TargetIs(const std::string& nodeName,
				const std::string& memoryKey,
				const TargetIs& value,
				const sp<NodeBase>& child) :
				BehaviorTree::Decorator(nodeName, child), 
				memoryKey(memoryKey),
				value(value)
			{}
		protected:
			virtual void notifyTreeEstablished() override
			{
				Memory& memory = getMemory();
				const sp<TargetIs>* memoryValue = memory.getReadValueAs<sp<TargetIs>>(memoryKey);
				assert(memoryValue);
				cachedValue = (memoryValue)? *memoryValue : nullptr; 
			}
			virtual void startBranchConditionCheck()
			{
				//Memory& memory = getMemory();
				//const sp<TargetIs>* memoryValue = memory.getReadValueAs<sp<TargetIs>>(memoryKey);
				//conditionalResult = memoryValue && *memoryValue && (**memoryValue == value);
				conditionalResult = cachedValue && *cachedValue == value;
			}
		private:
			virtual void handleNodeAborted() override {}
		private: //node properties
			const std::string memoryKey;
			const TargetIs value;
		private:
			sp<TargetIs> cachedValue = nullptr;
		};


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task_CalculateTargetArrangment
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Task_CalculateTargetArrangment : public DogfightTaskBase
		{
		public:
			Task_CalculateTargetArrangment(const std::string& nodeName,
				const std::string& targetArrangement_Key,
				const std::string& brain_Key,
				const std::string& target_Key
			): DogfightTaskBase(nodeName, 
				targetArrangement_Key,
				brain_Key, 
				target_Key)
			{}
		protected:
			virtual void beginTask() override;
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};
		};


		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Task_DriveShip_DogFight
		//		Reads memory to determine what it should perform considering conditions.
		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		class Task_ApplyInputRequest : public DogfightTaskBase
		{
		public:
			Task_ApplyInputRequest(const std::string& nodeName,
				const std::string& targetArrangement_Key,
				const std::string& brain_Key,
				const std::string& target_Key,
				const std::string& inputRequests_key
			): DogfightTaskBase(nodeName,
					targetArrangement_Key,
					brain_Key,
					target_Key
				),
				inputRequests_key(inputRequests_key)
			{}

		protected:
			virtual void beginTask() override;
			virtual void handleNodeAborted() override {}
			virtual void taskCleanup() override {};

			virtual void notifyTreeEstablished() override;

		private:
			const std::string inputRequests_key;
		private:
			fwp<ShipInputRequest> inputRequest;
		};

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Combo building blocks
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////
		// sharp turn
		////////////////////////////////////////////////////////
		class SharpTurnStep_VerboseTree : public ComboStepBase_VerboseTree
		{
		public:
			SharpTurnStep_VerboseTree(std::vector<TargetIs>&& phases) 
				: ComboStepBase_VerboseTree(std::move(phases))
			{}
			//virtual void onInit() {}
			virtual void doAction(float dt_sec, TargetIs arrangement, const fwp<Ship>& myTarget, const fwp<Ship>& myShip, const fwp<ShipInputRequest>& inputRequestLoc);
		};

		////////////////////////////////////////////////////////
		// slow when facing
		////////////////////////////////////////////////////////
		class SlowWhenFacingStep_VerboseTree : public ComboStepBase_VerboseTree
		{
		public:
			SlowWhenFacingStep_VerboseTree(std::vector<TargetIs>&& phases)
				: ComboStepBase_VerboseTree(std::move(phases))
			{}
			//virtual void onInit() {}
			virtual void doAction(float dt_sec, TargetIs arrangement, const fwp<Ship>& myTarget, const fwp<Ship>& myShip, const fwp<ShipInputRequest>& inputRequestLoc);

		private:
			float slowdownDist2 = 20 * 20;
		};

		////////////////////////////////////////////////////////
		// complete step
		////////////////////////////////////////////////////////
		class CompleteStep_VerboseTree : public ComboStepBase_VerboseTree
		{
		public:
			CompleteStep_VerboseTree(std::vector<TargetIs>&& phases)
				: ComboStepBase_VerboseTree(std::move(phases))
			{
			}
			//virtual void onInit() {}
			virtual void doAction(float dt_sec, TargetIs arrangement, const fwp<Ship>& myTarget, const fwp<Ship>& myShip, const fwp<ShipInputRequest>& inputRequestLocLoc) {};

		};

		////////////////////////////////////////////////////////
		// follow and attack
		////////////////////////////////////////////////////////
		class FollowAndAttackStep_VerboseTree : public ComboStepBase_VerboseTree
		{
		public:
			FollowAndAttackStep_VerboseTree(std::vector<TargetIs>&& phases)
				: ComboStepBase_VerboseTree(std::move(phases))
			{
			}
			//virtual void onInit() {}
			virtual void doAction(float dt_sec, TargetIs arrangement, const fwp<Ship>& myTarget, const fwp<Ship>& myShip, const fwp<ShipInputRequest>& inputRequestLoc) {};
		};

	}
}