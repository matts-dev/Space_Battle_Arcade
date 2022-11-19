#pragma once

#include <vector>

#include "GameFramework/SASystemBase.h"
#include "Tools/DataStructures/MultiDelegate.h"

#define SA_ENABLE_AUTOMATED_TESTS 0

namespace SA
{
	class LevelBase;
	class LiveTest;

	class AutomatedTestSystem : public SystemBase
	{
	private:
		virtual void tick(float deltaSec) override;
		virtual void initSystem() override;
		virtual void shutdown() override;

	private:
		void handleGameloopBeginning();
		void handlePreLevelChange(const sp<LevelBase>& /*currentLevel*/, const sp<LevelBase>& /*newLevel*/);
	private:
		std::vector<sp<LiveTest>> liveTests;
		bool bLiveTestingRunning = false;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Base classes for live tests (ie tests that require some duration of ticking)
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class LiveTest : public GameEntity
	{
	public:
		virtual bool isComplete() { return bComplete;}
		virtual bool passedTest() { return bAllPasing;}
		virtual bool hasStarted() { return bStarted; }
		virtual void tick() = 0;
		virtual void beginTest() = 0;

	protected:
		bool bStarted = false;
		bool bComplete = false;
		bool bAllPasing = false;
	};



}
