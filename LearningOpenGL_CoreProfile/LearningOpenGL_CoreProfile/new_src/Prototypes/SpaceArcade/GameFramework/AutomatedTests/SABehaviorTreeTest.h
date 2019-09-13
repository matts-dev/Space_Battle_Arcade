#pragma once
#include "../SAAutomatedTestSystem.h"

namespace SA
{
	//forward declarations
	namespace BehaviorTree
	{
		class Tree;
		class Task;
		class NodeBase;
	}

	//struct that keeps track of the tasks that should and shouldn't be hit. Designing a tree around these can test many feature
	//see the tasks in the cpp: task_must_fail, task_must_succeed, task_never_execute
	struct TestTreeStructureResults
	{
		TestTreeStructureResults(size_t numRequiredTasks) : numRequiredCompletedTasks(numRequiredTasks) {}
		size_t numRequiredCompletedTasks = 0;	//the expected number of tasks to update
		bool bComplete = false;
		std::set<sp<BehaviorTree::NodeBase>> requiredCompletedTasks;	//location for tasks that are required to pass
		std::set<sp<BehaviorTree::NodeBase>> forbiddenTasks;			//location for tasks that should never be hit (or have failed)

		bool passed() 
		{ 
			return requiredCompletedTasks.size() == numRequiredCompletedTasks 
				&& forbiddenTasks.size() == 0;
		}
	};

	class BehaviorTreeTest : public LiveTest
	{
	public:
		virtual void beginTest() override;
		virtual bool hasStarted() { return bStarted; }
		virtual void tick() override;
		void handleTestTimeOver();
	private:
		bool bStarted = false;

		//see definition of trees for notes on what they are testing
		sp<BehaviorTree::Tree> selectorTestTree_ffsn;	//fail,fail,succeed,neverhit
		sp<BehaviorTree::Tree> selectorTestTree_fff_sn;	
		sp<BehaviorTree::Tree> sequenceTestFailureTree_sssfn_s;	//test failures in sequence
		sp<BehaviorTree::Tree> sequenceTestSuccessTree_ssss_n;	//test success in sequence
		sp<BehaviorTree::Tree> treeTest_stateClearSelectorState1; 
		sp<BehaviorTree::Tree> treeTest_stateClearSequence;
		sp<BehaviorTree::Tree> memoryTaskTest;
		sp<BehaviorTree::Tree> decoratorTest_basicFunctionality;
		sp<BehaviorTree::Tree> decoratorTest_deferred_basicFunctionality;
		sp<BehaviorTree::Tree> memoryOperationsTestTree;
		//sp<BehaviorTree::Tree> abortTest_basicFunctionality;

		sp<BehaviorTree::Tree> treeTest_deferredSelection_ffsn;
		sp<BehaviorTree::Tree> treeTest_deferredSequence_sss_n;
		sp<BehaviorTree::Tree> serviceTest_basicFeatures;
		sp<BehaviorTree::Tree> serviceTest_immediateExecute;
		std::set<sp<BehaviorTree::Tree>> treesToTick;

		//****when accessing us "at" method instead of operator [] to get exceptions when typos are used****
		//the map cannot be const because its values are mutable.
		std::map<std::string, TestTreeStructureResults> testResults =
		{
			{"selectorTestTree_ffsn", 3},
			{"selectorTestTree_fff_sn", 4},
			{"sequenceTestFailureTree_sssfn_s", 5},
			{"sequenceTestSuccessTree_ssss_n", 4},
			{"treeTest_stateClearSelectorState1", 4 },
			{"treeTest_stateClearSequence", 4},
			{"memoryTaskTest", 4},
			{"treeTest_deferredSelection_ffsn", 3},
			{"treeTest_deferredSequence_sss_n", 3 },
			{"serviceTest_basicFeatures", 1 },
			{"serviceTest_immediateExecute", 1},
			{"decoratorTest_basicFunctionality", 1 },
			{"decoratorTest_deferred_basicFunctionality", 1},
			{"memoryOperationsTestTree", 11}
		};

	};
}