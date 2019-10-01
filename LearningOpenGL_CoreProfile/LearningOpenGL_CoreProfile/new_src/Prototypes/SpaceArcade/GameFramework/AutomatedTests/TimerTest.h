#pragma once

#include "../SAAutomatedTestSystem.h"

#define VERBOSE_TIMER_TEST_LOGGING 0

namespace SA
{
	struct TickerTestResults
	{
		const uint32_t STOP_TICKING_AFTER_X_TICKS = 20;

		bool bDeferredAdd = false;
		bool bAttemptedDeferredRemove = false;
		uint32_t startTickerTicks = 0;
		uint32_t childTickerTicks = 0;
		uint32_t removedTickerTicks = 0;
		bool bEndOfTestChildTickerPresent = false;
		bool bEndOfTestChildTickerRemoved = false;

		bool hasPassed()
		{
			return bDeferredAdd
				&& bAttemptedDeferredRemove
				&& startTickerTicks == STOP_TICKING_AFTER_X_TICKS
				&& childTickerTicks >= (STOP_TICKING_AFTER_X_TICKS / 2) //just needs to be some number less than the base ticker
				&& removedTickerTicks == 1
				&& bEndOfTestChildTickerPresent && bEndOfTestChildTickerRemoved;

		}
	};

	class TimerTest : public LiveTest
	{
	public:
		virtual void tick() override;
		virtual bool hasStarted() { return bStarted; }

	protected:
		virtual void postConstruct() override;

	private:
		virtual void beginTest() override;
		void startTest(LevelBase& level);

	private: //test data
		bool bBasicTimerTestPass = false;
		void handleBasicTimerTest();

		bool bRemoveTestTickComplete = false;
		bool bRemoveTestPass = true;
		sp<MultiDelegate<>> removeTestDelegate;
		void handleBasicRemoveTestFailure();

		int loopCount = 0;
		bool bLoopTestPassed = false;
		sp<MultiDelegate<>> loopingTimerTestDelegate;
		void handleLoopTimerTest();

		bool bDuplicateTimerTestPassed = true;
		void handleDuplicateTimerTest();

		bool bTimerFromTimerPassed = false;
		sp<MultiDelegate<>> addTimerFromTimerDelegate;
		void handleTimerFromTimer();

		bool bRemoveTimerFromTimerPassed = true;
		sp<MultiDelegate<>> removeTimerFromTimerDelegate;
		void handleRemoveTimerDuringTimerBroadcastTest();

		int timerClearedIncrement = 0;
		bool bNonLoopTimersClearedPassed = false;
		sp<MultiDelegate<>> timersClearTestDelegate;
		void handleTimerClearedTest();

		bool bDelayedTimerPassed = false;
		void handleDelayedTimer();

		bool bContainsTimerPassed = false;

	private:
		void handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel);

		sp<MultiDelegate<>> totalTestTimeHandle;
		void handleTestTimeOver();

	private: //state
		bool bStarted = false;

		TickerTestResults tickerResults;
	};
}