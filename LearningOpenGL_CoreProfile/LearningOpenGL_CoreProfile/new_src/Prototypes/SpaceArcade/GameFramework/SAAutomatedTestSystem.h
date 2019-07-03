#pragma once

#include <vector>

#include "SASystemBase.h"
#include "../Tools/DataStructures/MultiDelegate.h"

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
	private:
		std::vector<sp<LiveTest>> liveTests;
		bool bLiveTestingStarted = false;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Base classes for live tests (ie tests that require some duration of ticking)
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class LiveTest : public GameEntity
	{
	public:
		virtual bool isComplete() = 0;
		virtual bool passedTest() = 0;
		virtual void tick() = 0;
	};



}
