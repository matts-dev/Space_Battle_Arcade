#include "SABehaviorTreeTest.h"
#include "../SAAIBrainBase.h"
#include <vector>
#include <iostream>
#include "../SALog.h"
#include <stdio.h>
#include "../../Tools/DataStructures/MultiDelegate.h"
#include "../SAGameBase.h"
#include "../SALevelSystem.h"
#include "../SATimeManagementSystem.h"
#include "../SALevel.h"
#include <utility>

namespace SA
{
	/////////////////////////////////////////////////////////////////////////////////////
	// task that must executes; behavior is such that it will return false to parent
	/////////////////////////////////////////////////////////////////////////////////////
	class task_must_fail : public BehaviorTree::Task
	{
	public:
		task_must_fail(TestTreeStructureResults& testResultLocation)
			: BehaviorTree::Task("task_must_fail"),
			testLocation(testResultLocation){}

		virtual void beginTask() override
		{
			evaluationResult = false;
			testLocation.requiredCompletedTasks.insert(sp_this());
		}

		TestTreeStructureResults& testLocation;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Task that when included in tree, the unit test will fail if this is never hit 
	// The behavior is such that it succeeds for testing purposes (ie returns true to parent)
	/////////////////////////////////////////////////////////////////////////////////////
	class task_must_succeed : public BehaviorTree::Task
	{
	public:
		task_must_succeed(TestTreeStructureResults& testResultLocation) 
			: BehaviorTree::Task("task_must_succeed"), 
			testLocation(testResultLocation) {}

		virtual void beginTask() override
		{
			evaluationResult = true;
			testLocation.requiredCompletedTasks.insert(sp_this());
		}

		TestTreeStructureResults& testLocation;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Task that should never be hit; unit tests should place these in places where tree
	// structure in combination with task types should never allow this to be executed.
	// If this is executed the unit test will fail.
	/////////////////////////////////////////////////////////////////////////////////////
	class task_never_execute : public BehaviorTree::Task
	{
	public:
		task_never_execute(TestTreeStructureResults& testResultLocation)
			: BehaviorTree::Task("task_never_execute"),
			testLocation(testResultLocation) {}

		virtual void beginTask() override
		{
			evaluationResult = true; //result doesn't really matter
			testLocation.forbiddenTasks.insert(sp_this());
		}
		TestTreeStructureResults& testLocation;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Task that will fail on first execute, but pass on remaining executes.
	/////////////////////////////////////////////////////////////////////////////////////
	class task_failFirst_succeedRemaining : public BehaviorTree::Task
	{
	public:
		task_failFirst_succeedRemaining(TestTreeStructureResults& testResultLocation)
			: BehaviorTree::Task("task_failFirst_succeedRemaining"),
			testLocation(testResultLocation)
		{
		}

		virtual void beginTask() override
		{
			evaluationResult = bHasFailedFirst; //this will only return false for first execution
			bHasFailedFirst = true;
			testLocation.requiredCompletedTasks.insert(sp_this());
		}

		bool bHasFailedFirst = false;
		TestTreeStructureResults& testLocation;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Task that completes over time
	/////////////////////////////////////////////////////////////////////////////////////
	class task_deferred : public BehaviorTree::Task
	{
	public:
		task_deferred(std::string name, TestTreeStructureResults& testResultLocation, bool inShouldSucceed, float inDeferTime)
			: BehaviorTree::Task(name),
			shouldSucceed(inShouldSucceed),
			deferredTime(inDeferTime),
			testLocation(testResultLocation)
		{
		}

		virtual void beginTask() override
		{
			const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel();
			if (currentLevel)
			{
				timerDelegate = new_sp<MultiDelegate<>>();
				timerDelegate->addWeakObj(sp_this(), &task_deferred::completeTask);

				const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager();
				worldTimeManager->createTimer(timerDelegate, deferredTime);
			}
			else
			{
				log("BehaviorTreeTest", LogLevel::LOG_ERROR, "Failed to get current level; cannot test with deferred task");
				testLocation.forbiddenTasks.insert(sp_this());
				completeTask();
				evaluationResult = !shouldSucceed; //what ever this should do, do the opposite to be loud about this kind of failure
			}
		}

		void completeTask()
		{
			evaluationResult = shouldSucceed;
			testLocation.requiredCompletedTasks.insert(sp_this());
		}

		float deferredTime = 2.0f;
		bool shouldSucceed = true;
		sp<SA::MultiDelegate<>> timerDelegate = nullptr;
		TestTreeStructureResults& testLocation;
	};

	class task_deferred_succeed : public task_deferred
	{
	public:
		task_deferred_succeed(float inDeferTime, TestTreeStructureResults& testResultLocation)
			: task_deferred("task_deferred_succeed", testResultLocation, true, inDeferTime)
		{
		}
	};

	class task_deferred_fail : public task_deferred
	{
	public:
		task_deferred_fail(float inDeferTime, TestTreeStructureResults& testResultLocation)
			: task_deferred("task_deferred_fail", testResultLocation, false, inDeferTime)
		{
		}
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Task memory updaters
	/////////////////////////////////////////////////////////////////////////////////////
	class task_memory_updater_int : public BehaviorTree::Task
	{
	public:
		task_memory_updater_int(const std::string& key, bool bShouldCreateKey, TestTreeStructureResults& testResultLocation)
			: BehaviorTree::Task("self_updater_int"),
			varKey(key),
			bShouldCreateKey(bShouldCreateKey),
			testLocation(testResultLocation)
		{}

	private:
		virtual void notifyTreeEstablished() override
		{
			if (bShouldCreateKey)
			{
				BehaviorTree::Memory& mem = getMemory();
				if (!mem.hasValue(varKey))
				{
					mem.replaceValue(varKey, new_sp<BehaviorTree::PrimitiveWrapper<int>>(0));
				}
			}
		}

	public:
		virtual void beginTask() override
		{
			BehaviorTree::Memory& mem = getMemory();

			BehaviorTree::ScopedUpdateNotifier<int> writeValue;
			if(mem.getWriteValueAs<int>(varKey, writeValue))
			{
				int& toUpdate = writeValue.get();

				toUpdate++;
				evaluationResult = true;
				if (toUpdate == 3)
				{
					testLocation.requiredCompletedTasks.insert(sp_this());
				}
				else
				{
					testLocation.requiredCompletedTasks.erase(sp_this());
				}
			}
		}

	private:
		std::string varKey;
		bool bShouldCreateKey = true;
		TestTreeStructureResults& testLocation;
	};

	////////////////////////////////////////////////////////
	// small struct that has an int within it to update.
	////////////////////////////////////////////////////////
	struct MemoryUpdateTester : public GameEntity
	{
		MemoryUpdateTester(int value) : myValue(value) { }
		int myValue = 0;
	};

	class task_memory_updater_ge : public BehaviorTree::Task
	{
	public:
		task_memory_updater_ge(const std::string& key, bool bShouldCreateKey, TestTreeStructureResults& testResultLocation)
			: BehaviorTree::Task("self_updater_game_entity"),
			varKey(key),
			bShouldCreateKey(bShouldCreateKey),
			testLocation(testResultLocation)
		{
		}

	private:
		virtual void notifyTreeEstablished() override
		{
			BehaviorTree::Memory& mem = getMemory();
			if (bShouldCreateKey)
			{
				if (!mem.hasValue(varKey))
				{
					mem.replaceValue(varKey, new_sp<MemoryUpdateTester>(0));
				}
			}
		}

	public:
		virtual void beginTask() override
		{
			BehaviorTree::Memory& mem = getMemory();

			BehaviorTree::ScopedUpdateNotifier<MemoryUpdateTester> writeValue;
			if (mem.getWriteValueAs<MemoryUpdateTester>(varKey, writeValue))
			{
				MemoryUpdateTester& toUpdate = writeValue.get();
				toUpdate.myValue++;
				evaluationResult = true;

				if (toUpdate.myValue == 3)
				{
					testLocation.requiredCompletedTasks.insert(sp_this());
				}
				else
				{
					testLocation.requiredCompletedTasks.erase(sp_this());
				}
			}
		}

	private:
		std::string varKey;
		bool bShouldCreateKey = true;
		TestTreeStructureResults& testLocation;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Service that increments an int every half second
	/////////////////////////////////////////////////////////////////////////////////////
	class Service_int_incrementer : public BehaviorTree::Service
	{
	public:
		Service_int_incrementer(const std::string& name,
			const std::string& intKeyName, float updateInterval, 
			const sp<NodeBase>& child)
			: BehaviorTree::Service(name, updateInterval, true, child),
			updateInterval(updateInterval),
			intKeyName(intKeyName)
		{}
	private:
		virtual void startService() override {};
		virtual void stopService() override {};
		virtual void serviceTick() override
		{ 
			BehaviorTree::Memory& mem = getMemory();

			BehaviorTree::ScopedUpdateNotifier<int> writeValue;
			if (mem.getWriteValueAs<int>(intKeyName, writeValue))
			{
				writeValue.get()++;
			}
		}
	private:
		float updateInterval = 0.48f;
		std::string intKeyName = "myInt";
	};

	class task_int_service_waiter : public BehaviorTree::Task
	{
	public:
		task_int_service_waiter(const std::string& name, const std::string valKey, TestTreeStructureResults& testResultLocation)
			: BehaviorTree::Task(name), valKey(valKey), testResultLocation(testResultLocation)
		{ }

		virtual void beginTask() override
		{
			const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel();
			if (currentLevel)
			{
				const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager();
				timerDelegate = new_sp<MultiDelegate<>>();
				timerDelegate->addWeakObj(sp_this(), &task_int_service_waiter::deferredTick);
				worldTimeManager->createTimer(timerDelegate, 0.1f, true);
			}
		}

		void deferredTick()
		{
			if (!evaluationResult.has_value())
			{
				BehaviorTree::Memory& mem = getMemory();
				if (const int* valueCheck = mem.getReadValueAs<int>(valKey))
				{
					if (*valueCheck >= 3)
					{
						testResultLocation.requiredCompletedTasks.insert(sp_this());
						evaluationResult = true;
						timerDelegate->removeAll(sp_this());
					}
				}
			}
		}

		//#todo depending on how aborts get implemented, this may need to be an abort method; probably better so super doesn't have to be called.
		virtual void resetNode() override
		{
			const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel();
			if (currentLevel)
			{
				const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager();
				worldTimeManager->removeTimer(timerDelegate);
			}

			BehaviorTree::Task::resetNode();
		}

	private:
		std::string valKey;
		TestTreeStructureResults& testResultLocation;
		sp<MultiDelegate<>> timerDelegate;
	};


	/////////////////////////////////////////////////////////////////////////////////////
	// service immediate tick testing
	/////////////////////////////////////////////////////////////////////////////////////
	class Service_int_setter : public BehaviorTree::Service
	{
	public:
		Service_int_setter(const std::string& name,
			const std::string& intKeyName, float updateInterval, int valueToSet,
			const sp<NodeBase>& child)
			: BehaviorTree::Service(name, updateInterval, true, child),
			updateInterval(updateInterval),
			intKeyName(intKeyName),
			valueToSet(valueToSet)
		{
		}
	private:
		virtual void startService() override {};
		virtual void stopService() override {};
		virtual void serviceTick() override
		{
			BehaviorTree::Memory& mem = getMemory();
			BehaviorTree::ScopedUpdateNotifier<int> writeValue;
			if (mem.getWriteValueAs<int>(intKeyName, writeValue))
			{
				writeValue.get() = valueToSet;
			}
		}
	private:
		float updateInterval = 0.48f;
		std::string intKeyName = "myInt";
		int valueToSet;
	};
	class task_value_checker : public BehaviorTree::Task
	{
	public:
		task_value_checker(const std::string& name, const std::string valKey, int goalValue, TestTreeStructureResults& testResultLocation)
			: BehaviorTree::Task(name), valKey(valKey), goalValue(goalValue), testResultLocation(testResultLocation)
		{
		}

		virtual void beginTask() override
		{
			BehaviorTree::Memory& memory = getMemory();
			if (const int* readValue = memory.getReadValueAs<int>(valKey))
			{
				if (*readValue == goalValue)
				{
					evaluationResult = true;
					testResultLocation.requiredCompletedTasks.insert(sp_this());
					return;
				}
			}
			evaluationResult = false;
		}

	private:
		std::string valKey;
		int goalValue = -1;
		TestTreeStructureResults& testResultLocation;
		sp<MultiDelegate<>> timerDelegate;
	};

	////////////////////////////////////////////////////////
	// Testing decorator base functionality
	////////////////////////////////////////////////////////
	class Decorator_BoolReader : public BehaviorTree::Decorator
	{
	public:
		Decorator_BoolReader(const std::string& name, const std::string boolKeyValue, const sp<NodeBase>& child) :
			BehaviorTree::Decorator(name, child), boolKeyValue(boolKeyValue)
		{}

		virtual void startBranchConditionCheck()
		{
			BehaviorTree::Memory& memory = getMemory();

			const bool* memoryValue = memory.getReadValueAs<bool>(boolKeyValue);
			assert(memoryValue);
			if (memoryValue)
			{
				conditionalResult = *memoryValue;
			}
			else
			{
				conditionalResult = false;
			}
		}

	private:
		std::string boolKeyValue;
	};


	class Decorator_Deferred_BoolReader : public BehaviorTree::Decorator
	{
	public:
		Decorator_Deferred_BoolReader(const std::string& name, const std::string boolKeyValue, const sp<NodeBase>& child) :
			BehaviorTree::Decorator(name, child), boolKeyValue(boolKeyValue)
		{
		}

		virtual void startBranchConditionCheck()
		{
			static LevelSystem& lvlSystem = GameBase::get().getLevelSystem();
			const sp<LevelBase>& currentLevel = lvlSystem.getCurrentLevel();
			if (currentLevel)
			{
				const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager();
				sp<MultiDelegate<>> timerDelegate = new_sp<MultiDelegate<>>();
				timerDelegate->addWeakObj(sp_this(), &Decorator_Deferred_BoolReader::handleDeferredCheckOver);
				worldTimeManager->createTimer(timerDelegate, 0.25f);
			}
			else
			{
				log("BehaviorTreeTest", LogLevel::LOG_ERROR, "No current level to test deferred decorator");
				conditionalResult = false;
			}
		}

		void handleDeferredCheckOver()
		{
			BehaviorTree::Memory& memory = getMemory();
			const bool* memoryValue = memory.getReadValueAs<bool>(boolKeyValue);
			assert(memoryValue);
			if (memoryValue)
			{
				conditionalResult = *memoryValue;
			}
			else
			{
				conditionalResult = false;
			}
		}

	private:
		std::string boolKeyValue;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// testing behavior tree memory operations
	/////////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////
		// task that modifies memory
		////////////////////////////////////////////////////////
		class task_modify_memory : public BehaviorTree::Task
		{
		public:
			task_modify_memory(const std::string& valueKey, TestTreeStructureResults& testResultLocation)
				: BehaviorTree::Task("task_modify_memory"),
				testLocation(testResultLocation), valueKey(valueKey)
			{
			}

			virtual void beginTask() override
			{
				BehaviorTree::Memory& memory = getMemory();

				BehaviorTree::ScopedUpdateNotifier<MemoryUpdateTester> write_obj;
				if (memory.getWriteValueAs<MemoryUpdateTester>(valueKey, write_obj))
				{
					write_obj.get().myValue = 500;
					testLocation.requiredCompletedTasks.insert(sp_this());
					evaluationResult = true;
				}
				else
				{
					//can't find key value with expected type, the test isn't designed for this scenario to happen.
					testLocation.forbiddenTasks.insert(sp_this()); //insert into forbidden tasks to make this kind of error as loud as possible.
					evaluationResult = false;
				}
			}

		private:
			TestTreeStructureResults& testLocation;
			std::string valueKey;
		};

		////////////////////////////////////////////////////////
		// task that replaces memory
		////////////////////////////////////////////////////////
		class task_replace_memory : public BehaviorTree::Task
		{
		public:
			task_replace_memory(const std::string& valueKey, TestTreeStructureResults& testResultLocation)
				: BehaviorTree::Task("task_replace_memory"),
				testLocation(testResultLocation), valueKey(valueKey)
			{
			}

			virtual void beginTask() override
			{
				BehaviorTree::Memory& memory = getMemory();

				sp<MemoryUpdateTester> newObj = new_sp<MemoryUpdateTester>(0);
				memory.replaceValue(valueKey, newObj);

				if (memory.hasValue(valueKey))
				{
					testLocation.requiredCompletedTasks.insert(sp_this());
					evaluationResult = true;
				}
				else
				{
					testLocation.forbiddenTasks.insert(sp_this()); //insert into forbidden tasks to make this kind of error as loud as possible.
					evaluationResult = false;
				}
			}

		private:
			TestTreeStructureResults& testLocation;
			std::string valueKey;
		};

		////////////////////////////////////////////////////////
		// decorator that gets notified on modification
		////////////////////////////////////////////////////////
		class Decorator_OnModifyPass : public BehaviorTree::Decorator
		{
		public:
			Decorator_OnModifyPass(const std::string& name, const std::string keyValue, TestTreeStructureResults& testResultLocation, const sp<NodeBase>& child) :
				BehaviorTree::Decorator(name, child), keyValue(keyValue), testLocation(testResultLocation)
			{
			}

			virtual void startBranchConditionCheck()
			{
				conditionalResult = true;
			}

		private:
			virtual void notifyTreeEstablished() override
			{
				//perhaps this should be a built-in feature of the decorators and you just specify what keys you want to listen to.
				getMemory().getModifiedDelegate(keyValue).addWeakObj(sp_this(), &Decorator_OnModifyPass::handleValueModified);
			}

			void handleValueModified(const std::string& key, const GameEntity* value)
			{
				if (key == keyValue)
				{
					//!!!user cannot modify values in these delegates as it cause infinite recursion!!!
					//modifying values will cause modified delegate to broadcast
					//BehaviorTree::ScopedUpdateNotifier<MemoryUpdateTester> write_obj;
					//if (getMemory().getWriteValueAs(keyValue, write_obj))
					//{
						//GameEntity* writableVersion = &write_obj.get();
						//if (writableVersion == value)
						//{
							//testLocation.requiredCompletedTasks.insert(sp_this());
							//return;
						//}
					//}
					testLocation.requiredCompletedTasks.insert(sp_this());
				}
				else
				{
					testLocation.forbiddenTasks.insert(sp_this());
				}
			}
		private:
			std::string keyValue;
			TestTreeStructureResults& testLocation;

		};

		////////////////////////////////////////////////////////
		// decorator that gets notified on replace
		////////////////////////////////////////////////////////
		class Decorator_OnReplacePass : public BehaviorTree::Decorator
		{
		public:
			Decorator_OnReplacePass(const std::string& name, const std::string keyValue, TestTreeStructureResults& testResultLocation, const sp<NodeBase>& child) :
				BehaviorTree::Decorator(name, child), keyValue(keyValue), testLocation(testResultLocation)
			{
			}

			virtual void startBranchConditionCheck()
			{
				conditionalResult = true;
			}

		private:
			virtual void notifyTreeEstablished() override
			{
				//perhaps this should be a built-in feature of the decorators and you just specify what keys you want to listen to.
				getMemory().getReplacedDelegate(keyValue).addWeakObj(sp_this(), &Decorator_OnReplacePass::handleValueReplaced);
				cachedStartValue = getMemory().getReadValueAs<GameEntity>(keyValue);
			}

			void handleValueReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue)
			{
				if (key == keyValue)
				{
					//make sure the replaced value is a different instance and it was the instance detected at start.
					if (oldValue == cachedStartValue && newValue != cachedStartValue)
					{
						//make sure the user can get a writable value if they desire
						BehaviorTree::ScopedUpdateNotifier<MemoryUpdateTester> write_obj;
						if (getMemory().getWriteValueAs(keyValue, write_obj))
						{
							GameEntity* writableVersion = &write_obj.get();
							if(writableVersion == newValue)
							{
								testLocation.requiredCompletedTasks.insert(sp_this());
								return;
							}
						}
					}
				}
				testLocation.forbiddenTasks.insert(sp_this());
			}
		private:
			std::string keyValue;
			const GameEntity* cachedStartValue = nullptr;
			TestTreeStructureResults& testLocation;
		};

		////////////////////////////////////////////////////////
		// Task that removes memory
		////////////////////////////////////////////////////////
		class task_remove_memory : public BehaviorTree::Task
		{
		public:
			task_remove_memory(const std::string& valueKey, bool bKeepDelegateListeners, TestTreeStructureResults& testResultLocation)
				: BehaviorTree::Task("task_remove_memory "),
				testLocation(testResultLocation), valueKey(valueKey), bKeepDelegateListeners(bKeepDelegateListeners)
			{
			}

			virtual void beginTask() override
			{
				BehaviorTree::Memory& memory = getMemory();
				memory.getReplacedDelegate(valueKey).addWeakObj(sp_this(), &task_remove_memory::handleValueReplaced);

				bHadMemoryAtleastOnce |= memory.hasValue(valueKey);

				if (bKeepDelegateListeners)
				{
					memory.removeValue(valueKey);

					//stop listening to changes
					memory.getReplacedDelegate(valueKey).removeWeak(sp_this(), &task_remove_memory::handleValueReplaced);
				}
				else
				{
					memory.eraseEntry(valueKey);
					//no need to erase subscription because this should clear out the delegate
				}

				evaluationResult = true;
			}
		private:
			void handleValueReplaced(const std::string& key, const GameEntity* oldValue, const GameEntity* newValue)
			{
				if (bHadMemoryAtleastOnce)
				{
					testLocation.requiredCompletedTasks.insert(sp_this());
				}
			}

		private:
			TestTreeStructureResults& testLocation;
			std::string valueKey;
			bool bKeepDelegateListeners;
			bool bHadMemoryAtleastOnce = false;
		};

		class task_verify_removed : public BehaviorTree::Task
		{
		public:
			task_verify_removed(const std::string& valueKey, TestTreeStructureResults& testResultLocation)
				: BehaviorTree::Task("task_verify_remove_memory"),
				testLocation(testResultLocation), valueKey(valueKey)
			{
			}

			virtual void beginTask() override
			{
				BehaviorTree::Memory& memory = getMemory();
				const GameEntity* ptr = memory.getReadValueAs<GameEntity>(valueKey);

				if (ptr == nullptr)
				{
					testLocation.requiredCompletedTasks.insert(sp_this());
				}
				evaluationResult = true;
			}
		private:
			TestTreeStructureResults& testLocation;
			std::string valueKey;
		};


		class task_make_key : public BehaviorTree::Task
		{
		public:
			task_make_key(const std::string& valueKey, TestTreeStructureResults& testResultLocation)
				: BehaviorTree::Task("task_make_key"),
				testLocation(testResultLocation), valueKey(valueKey)
			{
			}

			virtual void beginTask() override
			{
				BehaviorTree::Memory& memory = getMemory();
				//normally you do not want to be doing this at runtime I would argue, perhaps setting
				//a value, but not creating it. That data should probably be coming from somewhere externally.
				memory.replaceValue(valueKey, new_sp<MemoryUpdateTester>(0));
				testLocation.requiredCompletedTasks.insert(sp_this());
				evaluationResult = true;
			}
		private:
			TestTreeStructureResults& testLocation;
			std::string valueKey;
		};
	/////////////////////////////////////////////////////////////////////////////////////
	// Testing aborts
	/////////////////////////////////////////////////////////////////////////////////////
	class task_must_clear_target : public BehaviorTree::Task
	{
	public:
		task_must_clear_target(const std::string& valueKey, TestTreeStructureResults& testResultLocation)
			: BehaviorTree::Task("task_must_clear_target"),
			testLocation(testResultLocation), valueKey(valueKey)
		{
		}

		virtual void beginTask() override
		{
			BehaviorTree::Memory& memory = getMemory();

			BehaviorTree::ScopedUpdateNotifier<bool> write_hasTarget;
			if (memory.getWriteValueAs<bool>(valueKey, write_hasTarget))
			{
				write_hasTarget.get() = false;
				testLocation.requiredCompletedTasks.insert(sp_this());
				evaluationResult = true;
			}
			else
			{
				//can't find key value with expected type, the test isn't designed for this scenario to happen.
				testLocation.forbiddenTasks.insert(sp_this()); //insert into forbidden tasks to make this kind of error as loud as possible.
				evaluationResult = false;
			}
		}

		TestTreeStructureResults& testLocation;
		std::string valueKey;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Behavior Tree Test
	/////////////////////////////////////////////////////////////////////////////////////


	void BehaviorTreeTest::beginTest()
	{
		log("BehaviorTreeTests", LogLevel::LOG, "Beginning Behavior Tree Tests");
		log("BehaviorTreeTests", LogLevel::LOG, "DEPENDENCY: Timer tests must pass.");

		bStarted = true;
		using namespace BehaviorTree;

		sp<MultiDelegate<>> timeOverHandle = new_sp<MultiDelegate<>>();
		timeOverHandle->addWeakObj(sp_this(), &BehaviorTreeTest::handleTestTimeOver);
		GameBase::get().getSystemTimeManager().createTimer(timeOverHandle, 7.f);

		/////////////////////////////////////////////////////////////////////////////////////
		//Tree below:
		//	-tests basic selector behavior
		//this test should complete immediately since there are no deferred tasks.
		/////////////////////////////////////////////////////////////////////////////////////
		selectorTestTree_ffsn =
			new_sp<Tree>("tree-root",
				new_sp<Selector>("TestSelector", std::vector<sp<NodeBase>>{
					new_sp<task_must_fail>(testResults.at("selectorTestTree_ffsn")),
					new_sp<task_must_fail>(testResults.at("selectorTestTree_ffsn")),
					new_sp<task_must_succeed>(testResults.at("selectorTestTree_ffsn")),
					new_sp<task_never_execute>(testResults.at("selectorTestTree_ffsn"))
				})
			);
		selectorTestTree_ffsn->start();
		selectorTestTree_ffsn->tick(0.1f);
		selectorTestTree_ffsn->stop();
		testResults.at("selectorTestTree_ffsn").bComplete = true;

		/////////////////////////////////////////////////////////////////////////////////////
		//Tree below:
		//	-Tests that if all children fail, failure propagates from selector to selector's parent.
		//	-Tests that nesting of selectors work
		/////////////////////////////////////////////////////////////////////////////////////
		selectorTestTree_fff_sn =
			new_sp<Tree>("tree-root",
				new_sp<Selector>("TopSelector", std::vector<sp<NodeBase>>{
					new_sp<Selector>("ChildSelector_HighPriority", std::vector<sp<NodeBase>>{
						new_sp<task_must_fail>(testResults.at("selectorTestTree_fff_sn")),
						new_sp<task_must_fail>(testResults.at("selectorTestTree_fff_sn")),
						new_sp<task_must_fail>(testResults.at("selectorTestTree_fff_sn"))
					}),
					new_sp<Selector>("ChildSelector_LowPriority", std::vector<sp<NodeBase>>{
						new_sp<task_must_succeed>(testResults.at("selectorTestTree_fff_sn")),
						new_sp<task_never_execute>(testResults.at("selectorTestTree_fff_sn"))
					})
				})
			);
		selectorTestTree_fff_sn->start();
		selectorTestTree_fff_sn->tick(0.1f);
		selectorTestTree_fff_sn->stop();
		testResults.at("selectorTestTree_fff_sn").bComplete = true;

		/////////////////////////////////////////////////////////////////////////////////////
		// Test Sequence Failures
		//	-tests that sequence node continues execution until fail
		//	-tests that failing sequence node returns false to node parent
		//	-tests that sequence node doesn't continue executing children once failure happens
		// Dependencies: selector behavior must work
		/////////////////////////////////////////////////////////////////////////////////////
		sequenceTestFailureTree_sssfn_s = 
			new_sp<Tree>("tree-root",
				new_sp<Selector>("TopSelector", std::vector<sp<NodeBase>>{
					new_sp<Sequence>("Sequence", std::vector<sp<NodeBase>>{
						new_sp<task_must_succeed>(testResults.at("sequenceTestFailureTree_sssfn_s")),
						new_sp<task_must_succeed>(testResults.at("sequenceTestFailureTree_sssfn_s")),
						new_sp<task_must_succeed>(testResults.at("sequenceTestFailureTree_sssfn_s")),
						new_sp<task_must_fail>(testResults.at("sequenceTestFailureTree_sssfn_s")),
						new_sp<task_never_execute>(testResults.at("sequenceTestFailureTree_sssfn_s"))
					}),
					new_sp<task_must_succeed>(testResults.at("sequenceTestFailureTree_sssfn_s"))	//this will be hit because sequence fill fail.
				})
			);
		sequenceTestFailureTree_sssfn_s->start();
		sequenceTestFailureTree_sssfn_s->tick(0.1f);
		sequenceTestFailureTree_sssfn_s->stop();
		testResults.at("sequenceTestFailureTree_sssfn_s").bComplete = true;

		/////////////////////////////////////////////////////////////////////////////////////
		// Test sequence successes
		//		-tests that all nodes in sequence are executed if each pass
		//		-tests that the result of the sequence node is correctly propagated to the parent.
		// Dependencies: selector tests pass
		/////////////////////////////////////////////////////////////////////////////////////
		sequenceTestSuccessTree_ssss_n = 
			new_sp<Tree>("tree-root",
				new_sp<Selector>("TopSelector", std::vector<sp<NodeBase>>{
					new_sp<Sequence>("Sequence", std::vector<sp<NodeBase>>{
						new_sp<task_must_succeed>(testResults.at("sequenceTestSuccessTree_ssss_n")),
						new_sp<task_must_succeed>(testResults.at("sequenceTestSuccessTree_ssss_n")),
						new_sp<task_must_succeed>(testResults.at("sequenceTestSuccessTree_ssss_n")),
						new_sp<task_must_succeed>(testResults.at("sequenceTestSuccessTree_ssss_n")),
					}),
					new_sp<task_never_execute>(testResults.at("sequenceTestSuccessTree_ssss_n")) //this should never execute because the sequence will pass for all children
				})
			);
		sequenceTestSuccessTree_ssss_n->start();
		sequenceTestSuccessTree_ssss_n->tick(0.1f);
		sequenceTestSuccessTree_ssss_n->stop();
		testResults.at("sequenceTestSuccessTree_ssss_n").bComplete = true;

		/////////////////////////////////////////////////////////////////////////////////////
		// Test that tree state gets cleared
		//		-tests that previous tree state from previous runs gets correctly cleared.
		//	Dependencies: Sequence tests must pass
		/////////////////////////////////////////////////////////////////////////////////////
		treeTest_stateClearSelectorState1 = 
			new_sp<Tree>("tree-root",
				new_sp<Sequence>("Sequence", std::vector<sp<NodeBase>>{
					new_sp<Selector>("Selector", std::vector<sp<NodeBase>>{
						new_sp<task_must_fail>(testResults.at("treeTest_stateClearSelectorState1")),
						new_sp<task_must_fail>(testResults.at("treeTest_stateClearSelectorState1")),
						new_sp<task_failFirst_succeedRemaining>(testResults.at("treeTest_stateClearSelectorState1")), //second run this will pass, letting the sequence hit its final node
					}),
					new_sp<task_must_succeed>(testResults.at("treeTest_stateClearSelectorState1")), //first tree tick this will not be hit since the selector will fail
				})
			);
		treeTest_stateClearSelectorState1->start();
		treeTest_stateClearSelectorState1->tick(0.1f); //task failFirst will fail, preventing last nodes in sequence from running
		treeTest_stateClearSelectorState1->tick(0.1f); //task failFirst will now succeed and the final node in sequence can be hit. This implicitly means selector's clear state is being cleared as it failed on the first test
		treeTest_stateClearSelectorState1->stop();
		testResults.at("treeTest_stateClearSelectorState1").bComplete = true;

		/////////////////////////////////////////////////////////////////////////////////////
		// Tests that the sequence node has its results cleared after first run
		//		-tests that a sequence node works correctly the second time it is hit
		// Dependencies: Sequence and Selector tests pass
		/////////////////////////////////////////////////////////////////////////////////////
		treeTest_stateClearSequence =
			new_sp<Tree>("tree-root",
				new_sp<Sequence>("TopSequence", std::vector<sp<NodeBase>>{
					new_sp<Sequence>("Sequence", std::vector<sp<NodeBase>>{
						new_sp<task_must_succeed>(testResults.at("treeTest_stateClearSequence")),
						new_sp<task_must_succeed>(testResults.at("treeTest_stateClearSequence")),
						new_sp<task_failFirst_succeedRemaining>(testResults.at("treeTest_stateClearSequence")), //second run this will pass, letting the sequence hit its final node
					}),
					new_sp<task_must_succeed>(testResults.at("treeTest_stateClearSequence")), //first tree tick this will not be hit since the selector will fail
				})
			);
		treeTest_stateClearSequence->start();
		treeTest_stateClearSequence->tick(0.1f); //task failFirst will fail, preventing last nodes in sequence from running
		treeTest_stateClearSequence->tick(0.1f); //task failFirst will now succeed and the final node in sequence can be hit. This implicitly means selector's clear state is being cleared as it failed on the first test
		treeTest_stateClearSequence->stop();
		testResults.at("treeTest_stateClearSequence").bComplete = true;

		/////////////////////////////////////////////////////////////////////////////////////
		// Test having updating memory
		//		-test having tree initialize some memory
		//		-tests creating tasks that will update keys
		//		-demos syntax of creating tasks that accept parameters
		//		-tests tasks reading memory they did not create
		//		-tests tree creating memory values
		//		-tests node notify of tree being established
		/////////////////////////////////////////////////////////////////////////////////////

		memoryTaskTest =
			new_sp<Tree>("tree-root",
				new_sp<Sequence>("Sequence", std::vector<sp<NodeBase>>{
					new_sp<task_memory_updater_int>("myGeneratedInt", true, testResults.at("memoryTaskTest")),
					new_sp<task_memory_updater_int>("treeMemoryInt", false, testResults.at("memoryTaskTest")),
					new_sp<task_memory_updater_ge>("myGeneratedGE", true, testResults.at("memoryTaskTest")),
					new_sp<task_memory_updater_ge>("treeMemoryGE", false, testResults.at("memoryTaskTest")),
				}),
				std::vector<std::pair<std::string, sp<GameEntity>>>{
					//std::make_pair<std::string, sp<BehaviorTree::PrimitiveWrapper<int>>>("treeMemoryInt", new_sp<BehaviorTree::PrimitiveWrapper<int>>(0)), //you can use this syntax, but below is easier remember and use
					{"treeMemoryInt", new_sp<BehaviorTree::PrimitiveWrapper<int>>(0)},
					{"treeMemoryGE", new_sp<MemoryUpdateTester>(0)}
				}
			);
		memoryTaskTest->start();
		memoryTaskTest->tick(0.1f); //nodes requires 3 ticks to pass
		memoryTaskTest->tick(0.1f);
		memoryTaskTest->tick(0.1f);
		memoryTaskTest->stop();
		testResults.at("memoryTaskTest").bComplete = true;


		/////////////////////////////////////////////////////////////////////////////////////
		// Test simple deferred tasks in a selector 
		//		-tests that deferred tasks in selector work correctly.
		/////////////////////////////////////////////////////////////////////////////////////
		treeTest_deferredSelection_ffsn = 
			new_sp<Tree>("tree-root-def-sel",
				new_sp<Selector>("TestSelector", std::vector<sp<NodeBase>>{
					new_sp<task_deferred_fail>(1.0f, testResults.at("treeTest_deferredSelection_ffsn")),
					new_sp<task_deferred_fail>(1.0f, testResults.at("treeTest_deferredSelection_ffsn")),
					new_sp<task_deferred_succeed>(1.0f, testResults.at("treeTest_deferredSelection_ffsn")),
					new_sp<task_never_execute>(testResults.at("treeTest_deferredSelection_ffsn"))
				})
			);
		treeTest_deferredSelection_ffsn->start();
		treesToTick.insert(treeTest_deferredSelection_ffsn);


		/////////////////////////////////////////////////////////////////////////////////////
		// Test simple deferred tasks in sequence
		//		-tests deferred tasks within a sequence operator correctly
		//		-tests return values of sequence are correctly used by selector node
		/////////////////////////////////////////////////////////////////////////////////////
		treeTest_deferredSequence_sss_n =
			new_sp<Tree>("tree-root-def-seq",
				new_sp<Selector>("Selector", std::vector<sp<NodeBase>>{
					new_sp<Sequence>("Sequence", std::vector<sp<NodeBase>>{
						new_sp<task_deferred_succeed>(1.0f, testResults.at("treeTest_deferredSequence_sss_n")),
						new_sp<task_deferred_succeed>(1.0f, testResults.at("treeTest_deferredSequence_sss_n")),
						new_sp<task_deferred_succeed>(1.0f, testResults.at("treeTest_deferredSequence_sss_n")),
					}),
					new_sp<task_never_execute>(testResults.at("treeTest_deferredSequence_sss_n"))
				})
			);
		treeTest_deferredSequence_sss_n->start();
		treesToTick.insert(treeTest_deferredSequence_sss_n);

		/////////////////////////////////////////////////////////////////////////////////////
		// Testing basic service updates
		/////////////////////////////////////////////////////////////////////////////////////
		serviceTest_basicFeatures =
			new_sp<Tree>("tree-root-service-basic-test",
				new_sp<Service_int_incrementer>("Service", "myInt", 0.25f,
					new_sp<task_int_service_waiter>("task_serviceWaiter", "myInt", testResults.at("serviceTest_basicFeatures"))
				),
				BehaviorTree::MemoryInitializer{
					{ "myInt", new_sp<BehaviorTree::PrimitiveWrapper<int>>( 0 )}
				}
			);
		serviceTest_basicFeatures->start();
		treesToTick.insert(serviceTest_basicFeatures);

		/////////////////////////////////////////////////////////////////////////////////////
		// Service execute immediately vs doesn't execute immediately feature
		//		-tests that a service will immediately execute if configured to do so
		// Dependencies
		//		-memory tests pass
		/////////////////////////////////////////////////////////////////////////////////////
		serviceTest_immediateExecute =
			new_sp<Tree>("tree-root-service-immediateExecute",
				new_sp<Service_int_setter>("Service", "goalInt", 10.0f, 5,
					new_sp<task_value_checker>("task_value_checker", "goalInt", 5, testResults.at("serviceTest_immediateExecute"))
					),
				BehaviorTree::MemoryInitializer{
					{ "goalInt", new_sp<BehaviorTree::PrimitiveWrapper<int>>(1)}
				}
		);
		serviceTest_immediateExecute->start();
		serviceTest_immediateExecute->tick(1.0f);
		serviceTest_immediateExecute->stop();
		testResults.at("serviceTest_immediateExecute").bComplete = true;

		/////////////////////////////////////////////////////////////////////////////////////
		// Decorator immediate branch condition checking
		//		-tests that failed decorates do not execute child
		//		-tests that passing decorators do execute child
		//		-tests that decorators correctly pass child node results to parent
		/////////////////////////////////////////////////////////////////////////////////////
		decoratorTest_basicFunctionality =
			new_sp<Tree>("root-decorator-conditionals-test",
				new_sp<Selector>("selector", std::vector<sp<NodeBase>>{
					new_sp<Decorator_BoolReader>("false bool decorator", "falseBool",
						new_sp<task_never_execute>(testResults.at("decoratorTest_basicFunctionality"))
					),
					new_sp<Decorator_BoolReader>("true bool decorator", "trueBool",
						new_sp<task_must_succeed>(testResults.at("decoratorTest_basicFunctionality")) 
					),
					new_sp<task_never_execute>(testResults.at("decoratorTest_basicFunctionality"))	//makes sure that the decorators results correctly propagates to parent
				}),
				BehaviorTree::MemoryInitializer{
					{ "trueBool", new_sp<BehaviorTree::PrimitiveWrapper<bool>>(true)},
					{ "falseBool", new_sp<BehaviorTree::PrimitiveWrapper<bool>>(false) }
				}
		);
		decoratorTest_basicFunctionality->start();
		decoratorTest_basicFunctionality->tick(0.1f);
		decoratorTest_basicFunctionality->stop();
		testResults.at("decoratorTest_basicFunctionality").bComplete = true;

		/////////////////////////////////////////////////////////////////////////////////////
		// Deferred decorator test with basic functionality
		//		-tests that decorators can have deferred evaluation of condition
		/////////////////////////////////////////////////////////////////////////////////////
		decoratorTest_deferred_basicFunctionality =
			new_sp<Tree>("root-decorator-deferred-conditionals-test",
				new_sp<Selector>("selector", std::vector<sp<NodeBase>>{
					new_sp<Decorator_Deferred_BoolReader>("false bool decorator", "falseBool",
						new_sp<task_never_execute>(testResults.at("decoratorTest_deferred_basicFunctionality"))
					),
					new_sp<Decorator_Deferred_BoolReader>("true bool decorator", "trueBool",
						new_sp<task_must_succeed>(testResults.at("decoratorTest_deferred_basicFunctionality")) 
					),
					new_sp<task_never_execute>(testResults.at("decoratorTest_deferred_basicFunctionality"))	//makes sure that the decorators results correctly propagates to parent
				}),
				BehaviorTree::MemoryInitializer{
					{ "trueBool", new_sp<BehaviorTree::PrimitiveWrapper<bool>>(true)},
					{ "falseBool", new_sp<BehaviorTree::PrimitiveWrapper<bool>>(false) }
				}
		);
		decoratorTest_deferred_basicFunctionality->start();
		treesToTick.insert(decoratorTest_deferred_basicFunctionality);

		////////////////////////////////////////////////////////
		// Memory operations test
		//		-memory reads
		//		-memory writes
		//		-memory replaced delegate
		//		-memory updated delegate
		//		-has value checks
		//		-remove value and subscribers
		//		-remove value leave subscribers
		//		todo?-subscribing to memory entry that doesn't yet exist?
		//
		////////////////////////////////////////////////////////
		memoryOperationsTestTree =
			new_sp<Tree>("memory_op_test_tree",
				new_sp<Sequence>("Sequence", MakeChildren{
					new_sp<Decorator_OnModifyPass>("decorator_modify", "modifyValueKey", testResults.at("memoryOperationsTestTree"), 
						new_sp<task_modify_memory>("modifyValueKey", testResults.at("memoryOperationsTestTree"))
					),
					new_sp<Decorator_OnReplacePass>("decorator_replace", "replaceValueKey", testResults.at("memoryOperationsTestTree"),
						new_sp<task_replace_memory>("replaceValueKey", testResults.at("memoryOperationsTestTree"))
					),
					new_sp<Decorator_OnReplacePass>("decorator_replace_rt", "futureAdd", testResults.at("memoryOperationsTestTree"),
						new_sp<Decorator_OnModifyPass>("decorator_modify_rt", "futureAdd", testResults.at("memoryOperationsTestTree"),
							new_sp<task_make_key>("futureAdd", testResults.at("memoryOperationsTestTree"))
						)
					),
					new_sp<task_remove_memory>("Remove_1", true, testResults.at("memoryOperationsTestTree")),
					new_sp<task_verify_removed>("Remove_1", testResults.at("memoryOperationsTestTree")),
					new_sp<task_remove_memory>("Remove_2", false, testResults.at("memoryOperationsTestTree")),
					new_sp<task_verify_removed>("Remove_2", testResults.at("memoryOperationsTestTree")),
				}),
				MemoryInitializer{
					{"modifyValueKey", new_sp<MemoryUpdateTester>(0)},
					{"replaceValueKey", new_sp<MemoryUpdateTester>(0)},
					{ "Remove_1", new_sp<PrimitiveWrapper<int>>(0) },
					{ "Remove_2", new_sp<MemoryUpdateTester>(0) }
					//note: future add will be added at runtime by a task, its decorators should respond to its add.
				}
			);
		memoryOperationsTestTree->start();
		memoryOperationsTestTree->tick(0.1f);
		memoryOperationsTestTree->stop();
		testResults.at("memoryOperationsTestTree").bComplete = true;

		/////////////////////////////////////////////////////////////////////////////////////
		// Aborting deferred behavior test; basic funcitonality
		/////////////////////////////////////////////////////////////////////////////////////
		//abortTest_basicFunctionality =
		//	new_sp<Tree>("abort-basic-test-root",
		//		new_sp<Decorator_BoolReader>("has_target_decorator", "bHasTargetKey",	//TODO this needs updating to use a decorator that aborts
		//			new_sp<task_must_clear_target>("bHasTargetKey", testResults.at(""))
		//		)
		//		new_sp<
		//	);

	}

	void BehaviorTreeTest::tick()
	{
		if(bStarted)
		{
			if (!bComplete)
			{
				//this tick is based off of system time; we want game time so get the delta sec for current level.
				const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel();
				if (currentLevel)
				{
					float deltaSec = currentLevel->getWorldTimeManager()->getDeltaTimeSecs();
					for (const sp<BehaviorTree::Tree>& tree : treesToTick)
					{
						tree->tick(deltaSec);
					}
				}

				bool bAllComplete = true;
				bAllPasing = true;
				for (auto kvPair : testResults)
				{
					bAllComplete &= kvPair.second.bComplete;
					bAllPasing &= kvPair.second.passed();
				}

				if (bAllComplete)
				{
					char msg[2048];
					constexpr size_t msgSize = sizeof(msg) / sizeof(msg[0]);

					for (auto kvPair : testResults)
					{
						std::string result = kvPair.second.passed() ? "PASSED" : "\tFAILED";
						snprintf(msg, msgSize, "\t %s : %s", result.c_str(), kvPair.first.c_str());
						log("BehaviorTreeTests", LogLevel::LOG, msg);
					}
					snprintf(msg, msgSize, "%s : Ending Behavior Tree Tests", bAllPasing ? "PASSED" : "FAILED");
					log("BehaviorTreeTests", LogLevel::LOG, msg);
				}
				bComplete = bAllComplete;
			}
		}
	}

	void BehaviorTreeTest::handleTestTimeOver()
	{
		char msg[2048];
		constexpr size_t msgSize = sizeof(msg) / sizeof(msg[0]);
		snprintf(msg, msgSize, "Allowed test completion time over");
		log("BehaviorTreeTests", LogLevel::LOG, msg);

		for (const sp<BehaviorTree::Tree>& tree : treesToTick)
		{
			tree->stop();
		}

		//flag all tests as being complete; some tests rely on this to signal completion in successful scenarios
		for (auto& mapIter : testResults)
		{
			mapIter.second.bComplete = true;
		}
	}

}
