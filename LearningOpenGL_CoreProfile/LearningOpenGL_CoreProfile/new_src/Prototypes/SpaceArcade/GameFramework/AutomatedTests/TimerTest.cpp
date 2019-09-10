#include "TimerTest.h"
#include "../SAGameBase.h"
#include "../SATimeManagementSystem.h"
#include "../SALevel.h"
#include "../SALog.h"
#include "../SALevelSystem.h"



namespace SA
{

	////////////////////////////////////////////////////////
	// Timer tests
	////////////////////////////////////////////////////////

	void TimerTest::tick()
	{
		static GameBase& game = GameBase::get();

		if (bStarted)
		{
			//basic remove test
			if (!bRemoveTestTickComplete)
			{
				bRemoveTestTickComplete = true;

				if (const sp<LevelBase>& currentLevel = game.getLevelSystem().getCurrentLevel())
				{
					const sp<TimeManager>& worldTM = currentLevel->getWorldTimeManager();
					if (worldTM->hasTimerForDelegate(removeTestDelegate))
					{
						//since this is done in a tick, it is NOT testing removal during a timer broadcast
						worldTM->removeTimer(removeTestDelegate);
					}
					else
					{
						log("AutomatedTest_TimerTest", LogLevel::LOG_ERROR, "timer not present for remove test");
					}
				}
			}
		}
	}

	void TimerTest::postConstruct()
	{
	}

	void TimerTest::beginTest()
	{
		bStarted = true;
		log("TimerTest", LogLevel::LOG, "Beginning Timer Tests");

		//timer test must wait for an active level to start
		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
		{
			startTest(*currentLevel);
		}
		else
		{
			log("TimerTest", LogLevel::LOG, "Waiting on level to be generated.");
			levelSystem.onPostLevelChange.addWeakObj(sp_this(), &TimerTest::handlePostLevelChange);
		}
	}

	void TimerTest::startTest(LevelBase& level)
	{

		//only run tests for a single level run
		GameBase::get().getLevelSystem().onPostLevelChange.removeWeak(sp_this(), &TimerTest::handlePostLevelChange);

		TimeManager& systemTM = GameBase::get().getSystemTimeManager();
		const sp<TimeManager>& worldTM = level.getWorldTimeManager();

		////////////////////////////////////////////////////////
		// Set up a time limit for all tests to complete
		////////////////////////////////////////////////////////
		totalTestTimeHandle = new_sp<MultiDelegate<>>();
		totalTestTimeHandle->addWeakObj(sp_this(), &TimerTest::handleTestTimeOver);
		systemTM.createTimer(totalTestTimeHandle, 5.f, false);


		////////////////////////////////////////////////////////
		// Tests
		////////////////////////////////////////////////////////

		//	add timer, callback fires
		float basicTestTime = 1.f;
		sp<MultiDelegate<>> basicCallbackTestDelegate = new_sp<MultiDelegate<>>();
		basicCallbackTestDelegate->addWeakObj(sp_this(), &TimerTest::handleBasicTimerTest);
		worldTM->createTimer(basicCallbackTestDelegate, basicTestTime, false);


		//  add timer, remove timer, callback never fires
		removeTestDelegate = new_sp<MultiDelegate<>>();
		removeTestDelegate->addWeakObj(sp_this(), &TimerTest::handleBasicRemoveTestFailure);
		worldTM->createTimer(removeTestDelegate, 1.1f, false);

		//  add looping timer, timer loops 3 times on same delegate
		loopingTimerTestDelegate = new_sp<MultiDelegate<>>();
		loopingTimerTestDelegate->addWeakObj(sp_this(), &TimerTest::handleLoopTimerTest);
		worldTM->createTimer(loopingTimerTestDelegate, 0.5f, true);
		 
		//  try to add duplicate delegate timer, should fail
		sp<MultiDelegate<>> duplicateTestDelegate = new_sp<MultiDelegate<>>();
		duplicateTestDelegate->addWeakObj(sp_this(), &TimerTest::handleDuplicateTimerTest);
		worldTM->createTimer(duplicateTestDelegate, 1.29f, false);
		ETimerOperationResult secondAddResult = worldTM->createTimer(duplicateTestDelegate, 1.29f, false);
		if (secondAddResult == ETimerOperationResult::SUCCESS)
		{
			bDuplicateTimerTestPassed = false;
		}
		
		//  add timer from callback of timer, make sure works (cannot be the same delegate though)
		//this will piggy back on the "handleBasicTimer" test, that callback will add the new delegate below
		addTimerFromTimerDelegate = new_sp<MultiDelegate<>>();
		addTimerFromTimerDelegate->addWeakObj(sp_this(), &TimerTest::handleTimerFromTimer);


		//  remove timer from callback of timer, make sure it work
		removeTimerFromTimerDelegate = new_sp<MultiDelegate<>>();
		removeTimerFromTimerDelegate->addWeakObj(sp_this(), &TimerTest::handleRemoveTimerDuringTimerBroadcastTest);
		worldTM->createTimer(removeTimerFromTimerDelegate, 3.f, false);


		//  test that contains timer works
		bContainsTimerPassed = worldTM->hasTimerForDelegate(basicCallbackTestDelegate);

		//  test that non-looping timers are getting cleared, this can be done by re-adding the original delegate (not during callback though)
		timersClearTestDelegate = new_sp<MultiDelegate<>>();
		timersClearTestDelegate->addWeakObj(sp_this(), &TimerTest::handleTimerClearedTest);
		worldTM->createTimer(timersClearTestDelegate, 0.25F, false); //NOTE: time must be shorter than delegate that will re-add it

		// test delay
		sp<MultiDelegate<>> delayTimerDelegate = new_sp<MultiDelegate<>>();
		delayTimerDelegate->addWeakObj(sp_this(), &TimerTest::handleDelayedTimer);
		worldTM->createTimer(delayTimerDelegate, basicTestTime - 0.2f, false, 0.5f); //delay half a sec, this should mean it fires after basicTestTime

	}

	void TimerTest::handleBasicTimerTest()
	{
#if VERBOSE_TIMER_TEST_LOGGING
		log("TimerTest", LogLevel::LOG, "\t\t" __FUNCTION__);
#endif // VERBOSE_TIMER_TEST_LOGGING
		bBasicTimerTestPass = true;

		//some tests require creating/removing timers from another timer; below are such tests
		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			const sp<TimeManager>& worldTM = currentLevel->getWorldTimeManager();
#if VERBOSE_TIMER_TEST_LOGGING
			log("TimerTest", LogLevel::LOG_WARNING, "\t\t started 2 second timer NOW -- not accounting for time dilation");
#endif // VERBOSE_TIMER_TEST_LOGGING
			worldTM->createTimer(addTimerFromTimerDelegate, 2.0f, false);

			worldTM->removeTimer(removeTimerFromTimerDelegate);

			worldTM->createTimer(timersClearTestDelegate, 0.25F, false);
		}
	}

	void TimerTest::handleBasicRemoveTestFailure()
	{
#if VERBOSE_TIMER_TEST_LOGGING
		log("TimerTest", LogLevel::LOG, "\t\t" __FUNCTION__);
#endif // VERBOSE_TIMER_TEST_LOGGING
		//this timer should be removed before the callback is hit!
		bRemoveTestPass = false;
	}

	void TimerTest::handleLoopTimerTest()
	{
#if VERBOSE_TIMER_TEST_LOGGING
		log("TimerTest", LogLevel::LOG, "\t\t" __FUNCTION__);
#endif // VERBOSE_TIMER_TEST_LOGGING

		loopCount++;
		if (loopCount == 3)
		{
			bLoopTestPassed = true;
			
			if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
			{
				const sp<TimeManager>& worldTM = currentLevel->getWorldTimeManager();
				if (worldTM->hasTimerForDelegate(loopingTimerTestDelegate))
				{
					worldTM->removeTimer(loopingTimerTestDelegate);
				}
				else
				{
#if VERBOSE_TIMER_TEST_LOGGING
					log("TimerTest", LogLevel::LOG_WARNING, "\t\t did not find timer for loop delegate");
#endif // VERBOSE_TIMER_TEST_LOGGING

				}
			}
		}

		if (loopCount > 3)
		{
			//removal didn't work!
			log("TimerTest", LogLevel::LOG_WARNING, "\t\tFailed to remove loop delegate internal to callback function");
			bLoopTestPassed = false;
		}

	}

	void TimerTest::handleDuplicateTimerTest()
	{
		//really doesn't need to do anything, we just need to make sure we can't add the same delegate twice
	}

	void TimerTest::handleTimerFromTimer()
	{
#if VERBOSE_TIMER_TEST_LOGGING
		log("TimerTest", LogLevel::LOG, "\t\t" __FUNCTION__);
		log("TimerTest", LogLevel::LOG, "\t\t ended 2 second timer over -- not accounting for time dilation");
#endif // VERBOSE_TIMER_TEST_LOGGING
		bTimerFromTimerPassed = true;
	}

	void TimerTest::handleRemoveTimerDuringTimerBroadcastTest()
	{
		//this didn't get removed, fail the test
#if VERBOSE_TIMER_TEST_LOGGING
		log("TimerTest", LogLevel::LOG, "\t\t" __FUNCTION__);
#endif // VERBOSE_TIMER_TEST_LOGGING
		bRemoveTimerFromTimerPassed = false;
	}

	void TimerTest::handleTimerClearedTest()
	{
#if VERBOSE_TIMER_TEST_LOGGING
		log("TimerTest", LogLevel::LOG, "\t\t" __FUNCTION__);
#endif // VERBOSE_TIMER_TEST_LOGGING
		++timerClearedIncrement;

		//this delegate will be re-added from another delegate, so the timerClearedIncrement should reach 2
		bNonLoopTimersClearedPassed = timerClearedIncrement == 2;
	}

	void TimerTest::handleDelayedTimer()
	{
		//if the basic time test passed, then the delay worked because this timer has a quicker callback than the basic test
		//but is delayed so that it will end up firing after the first basic test. So if basic test has already passed, then
		// this test was successfully delayed.
		bDelayedTimerPassed = bBasicTimerTestPass;
	}

	void TimerTest::handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel)
	{
		startTest(*newCurrentLevel);
	}

	void TimerTest::handleTestTimeOver()
	{
		char msg[2048];
		constexpr size_t msgSize = sizeof(msg) / sizeof(msg[0]);

		bAllPasing = true;

		bAllPasing &= bBasicTimerTestPass;
			snprintf(msg, msgSize , "\t %s : Basic Timer Test", bBasicTimerTestPass ? "PASSED" : "FAILED");
			log("TimerTest", LogLevel::LOG, msg);

		bAllPasing &= bRemoveTestPass;
			snprintf(msg, msgSize , "\t %s : Basic Remove Test", bRemoveTestPass ? "PASSED" : "FAILED");
			log("TimerTest", LogLevel::LOG, msg);

		bAllPasing &= bLoopTestPassed;
			snprintf(msg, msgSize , "\t %s : Looping Timer Test", bLoopTestPassed ? "PASSED" : "FAILED");
			log("TimerTest", LogLevel::LOG, msg);

		bAllPasing &= bDuplicateTimerTestPassed;
			snprintf(msg, msgSize , "\t %s : Prevent Add Duplicate Timer Test", bDuplicateTimerTestPassed ? "PASSED" : "FAILED");
			log("TimerTest", LogLevel::LOG, msg);

		bAllPasing &= bTimerFromTimerPassed;
			snprintf(msg, msgSize , "\t %s : Add non-same timer from timer callback test", bTimerFromTimerPassed ? "PASSED" : "FAILED");
			log("TimerTest", LogLevel::LOG, msg);

		//remove timer from timer
		bAllPasing &= bRemoveTimerFromTimerPassed;
			snprintf(msg, msgSize , "\t %s : Remove non-same timer from timer callback test", bRemoveTimerFromTimerPassed ? "PASSED" : "FAILED");
			log("TimerTest", LogLevel::LOG, msg);

		//timer clear test
		bAllPasing &= bNonLoopTimersClearedPassed;
			snprintf(msg, msgSize , "\t %s : Non-loop timers cleared test", bNonLoopTimersClearedPassed ? "PASSED" : "FAILED");
			log("TimerTest", LogLevel::LOG, msg);

		//delay timer test
		bAllPasing &= bDelayedTimerPassed;
			snprintf(msg, msgSize, "\t %s :Delayed Timer Test", bDelayedTimerPassed ? "PASSED" : "FAILED");
			log("TimerTest", LogLevel::LOG, msg);

		

		snprintf(msg, msgSize , "%s : Ending Timer Tests", bAllPasing ? "PASSED" : "FAILED");
		log("TimerTest", LogLevel::LOG, msg);

		bComplete = true; 
	}

}
