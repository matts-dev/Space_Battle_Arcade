#pragma once
#include <stack>
#include "../../SAAutomatedTestSystem.h"
#include "../../../Tools/DataStructures/LifetimePointer.h"

namespace SA
{
	class LifetimePtrTest : public LiveTest
	{
	public:
		virtual void beginTest() override;
		virtual void tick() override;
		virtual bool passedTest() override;

	public:
		void completeTest(bool bPassed, const std::string& subtestName, const std::string& failReason = "");
		bool hasFinishedFirstTick() { return tickNum > 0; }
		int tickNum = 0;

		void markDeferredDeleteComplete() { bDeferredDeleteComplete = true; }
		bool bDeferredDeleteComplete = false;
	private:
		sp<GameEntity> destroy_sp;
		wp<GameEntity> destroy_wp;
		lp<GameEntity> destroy_lp;
		bool bIsNonNullAtStart_destroyTest = false;

		std::stack<lp<GameEntity>> lifetimePointerStack;

		wp<GameEntity> cleanup_wp;
	private:
		std::set<std::string> passingTests;
		std::map<std::string, std::string> failingTests;
	};
}

