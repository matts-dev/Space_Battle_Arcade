#pragma once

#include "../SAAutomatedTestSystem.h"

#define VERBOSE_TIMER_TEST_LOGGING 0

namespace SA
{
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
	};
}