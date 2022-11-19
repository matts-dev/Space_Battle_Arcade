#include "GameFramework/SAAutomatedTestSystem.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SALevelSystem.h"
#include "GameFramework/SALog.h"
#include "GameFramework/SALevel.h"
#include <stdio.h>
#include "AutomatedTests/TimerTest.h"
#include "AutomatedTests/SABehaviorTreeTest.h"
#include "0.TestsFiles/CompilationTests/LifetimePointerSyntaxTest.h"




namespace SA
{
	void AutomatedTestSystem::tick(float deltaSec)
	{
#if SA_ENABLE_AUTOMATED_TESTS
		if (bLiveTestingRunning)
		{
			bool bAllTestsComplete = true;
			bool bAllTestsPassing = true;
			for (const sp<LiveTest>& test : liveTests)
			{
				if (!test->hasStarted())
				{
					test->beginTest();
				}
				if (!test->isComplete())
				{
					test->tick();

					//let this test complete before processing next test
					//#future probably should be event based than polling in a tick
					return;
				}

				bAllTestsComplete &= test->isComplete();
				bAllTestsPassing &= test->passedTest();
			}

			if (bAllTestsComplete)
			{
				if (!bAllTestsPassing)
				{
					log("AutomatedTestSystem", LogLevel::LOG_WARNING, "Live Test Failures Detected.");
				}
				else
				{
					log("AutomatedTestSystem", LogLevel::LOG, "Live Tests Passed.");
				}
				log("AutomatedTestSystem", LogLevel::LOG, "Live Testing Over.");
				liveTests.clear();

				bLiveTestingRunning = false;
			}
		}

#endif // SA_ENABLE_AUTOMATED_TESTS
	}

	void AutomatedTestSystem::initSystem()
	{
#if SA_ENABLE_AUTOMATED_TESTS
		log("AutomatedTestSystem", LogLevel::LOG, "Automated Testing Turned On; WARNING: strange behavior will occur while testing is active");
		GameBase::get().onGameloopBeginning.addWeakObj(sp_this(), &AutomatedTestSystem::handleGameloopBeginning);
		GameBase::get().getLevelSystem().onPreLevelChange.addWeakObj(sp_this(), &AutomatedTestSystem::handlePreLevelChange);
#else
		log("AutomatedTestSystem", LogLevel::LOG, "Automated Testing Turned Off; no automated testing will be done");
#endif // SA_ENABLE_AUTOMATED_TESTS

	}

	void AutomatedTestSystem::shutdown()
	{
#if SA_ENABLE_AUTOMATED_TESTS

#endif // SA_ENABLE_AUTOMATED_TESTS
	}

	void AutomatedTestSystem::handleGameloopBeginning()
	{
		log("AutomatedTestSystem", LogLevel::LOG, "Live Testing Started");

		liveTests.push_back(new_sp<LifetimePtrTest>());
		liveTests.push_back(new_sp<BehaviorTreeTest>());
		liveTests.push_back(new_sp<TimerTest>());

		bLiveTestingRunning = true;
	}


	void AutomatedTestSystem::handlePreLevelChange(const sp<LevelBase>& /*currentLevel*/, const sp<LevelBase>& /*newLevel*/)
	{
		if (bLiveTestingRunning)
		{
			log("AutomatedTestSystem", LogLevel::LOG_ERROR, "!!! - Unexpected level change while testing --- this will break any timer based tests - !!!");
		}
	}

	void LiveTest::beginTest()
	{
		bStarted = true;
	}

}