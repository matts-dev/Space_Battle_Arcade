#include <GLFW/glfw3.h>
#include "SATimeManagementSystem.h"
#include "SALog.h"
#include "../Tools/DataStructures/IterableHashSet.h"
#include "../Tools/DataStructures/ObjectPools.h"
#include <algorithm>

namespace
{
	SA::SP_SimpleObjectPool<SA::Timer> timerPool;

}

namespace SA
{

	void Timer::reset()
	{
		durationSecs = 0.f;
		currentTime = 0.f;
		bLoop = false;
		userCallback = nullptr;
	}

	void Timer::set(const sp<MultiDelegate<>>& inCallbackDelegate, float inDurationSecs, bool inbLoop, float delaySecs)
	{
		reset();
		userCallback = inCallbackDelegate;
		durationSecs = inDurationSecs;
		bLoop = inbLoop;

		//set the timer into negative region for the delay
		currentTime = -delaySecs; 
	}

	bool Timer::update(float dt_dilatedSecs)
	{
		if (!userCallback) { return true; }
		if (userCallback->numBound() == 0) { return true; }

		currentTime += dt_dilatedSecs;

		//if timer should have ticked twice in time-frame, then tick it twice.
		//the alternative may cause unexpected behavior for user
		while (currentTime > durationSecs)
		{
			userCallback->broadcast();
			currentTime -= durationSecs;

			if (!bLoop)
			{
				return true;
			}
		}

		return false;
	}

	void TimeManager::update(PrivateKey key, TimeSystem& timeSystem)
	{
		////////////////////////////////////////////////////////
		// updating state
		////////////////////////////////////////////////////////
		framesToStep = framesToStep > 0 ? framesToStep - 1 : framesToStep;
		if (framesToStep_nextFrame > 0)
		{
			framesToStep = framesToStep_nextFrame;
			framesToStep_nextFrame = 0;
		}

		//prevents time dilation from happening mid frame
		timeDilationFactor = DilationFactor_nextFrame;

		dt_undilatedSecs = timeSystem.getDeltaTimeSecs();
		dt_dilatedSecs = dt_undilatedSecs * timeDilationFactor;

		////////////////////////////////////////////////////////
		//tick timers
		////////////////////////////////////////////////////////
		{
			bTickingTimers = true;
			for (const sp<Timer>& timer : timers)
			{
				if (timer->update(dt_dilatedSecs))
				{
					timersToRemoveWhenTickingOver.emplace_back(timer);
				}
			}
			bTickingTimers = false;
		}

		////////////////////////////////////////////////////////
		//clear stale timers before adding deferred timers
		////////////////////////////////////////////////////////
		if (timersToRemoveWhenTickingOver.size() > removeTimerReservationSpace)
		{ 
			removeTimerReservationSpace = std::clamp(removeTimerReservationSpace, 0U, 10000U);
			timersToRemoveWhenTickingOver.reserve(removeTimerReservationSpace);
		}
		for (const sp<Timer> timer : timersToRemoveWhenTickingOver)
		{
			//ticking cannot be happening since we're below it -- this should remove the timer (and avoid code duplication)
			if (removeTimer(timer->getUserCallback()) != ETimerOperationResult::SUCCESS)
			{
				//log failure when there exists a more performant logging system
				log("TimeManagementSystem", LogLevel::LOG_ERROR, "Failed to remove timer; this should not happen.");
			}
		}
		timersToRemoveWhenTickingOver.clear();

		////////////////////////////////////////////////////////
		//add deferred timers
		////////////////////////////////////////////////////////
		for (const sp<Timer>& timer : deferredTimersToAdd)
		{
			if (MultiDelegate<>* delegateHandle = timer->getUserCallback().get())
			{
				timers.insert(timer);
				delegateToTimerMap.insert({ delegateHandle , timer });
			}
			else
			{
				//#TODO log when logging has been refactored and isn't so expensive; actually this really should be an assertion failure
			}
		}
		deferredTimersToAdd.clear();
		deferredTimerDelegatesToAdd.clear();
	}

	TimeManager::TimeManager()
	{
		timersToRemoveWhenTickingOver.reserve(removeTimerReservationSpace);
	}

	bool TimeManager::hasTimerForDelegate(const sp<MultiDelegate<>>& boundDelegate)
	{
		if (boundDelegate)
		{
			return delegateToTimerMap.find(boundDelegate.get()) != delegateToTimerMap.end();
		}
		return false;
	}

	SA::ETimerOperationResult TimeManager::createTimer(const sp<MultiDelegate<>>& callbackDelegate, float durationSec, bool bLoop /* = false*/, float delaySecs /*= 0.f*/)
	{
		if (durationSec < 0) //setting duration equal to 0 is like a "next tick" timer
		{
			return ETimerOperationResult::FAILURE_NEGATIVE_DURATION;
		}

		if (bTickingTimers)
		{
			/*! timers can be deferred until timer ticking is over because adding during ticking will cause premature addition to the timer's clock */
			if (delegateToTimerMap.find(callbackDelegate.get()) != delegateToTimerMap.end())
			{
				//technically this timer could be pending remove after its tick, but if user wants that then they should be looping
				return ETimerOperationResult::DEFER_FAILURE_TIMER_FOR_DELEGATE_EXISTS;
			}
			if (deferredTimerDelegatesToAdd.find(callbackDelegate.get()) != deferredTimerDelegatesToAdd.end())
			{
				return ETimerOperationResult::DEFER_FAILURE_DELEGATE_ALREDY_PENDING_ADD;
			}

			sp<SA::Timer> timerInstance = timerPool.getInstance();
			timerInstance->set(callbackDelegate, durationSec, bLoop, delaySecs);

			deferredTimerDelegatesToAdd.insert({ callbackDelegate.get(), timerInstance });
			deferredTimersToAdd.insert(timerInstance);
		}
		else
		{
			if (delegateToTimerMap.find(callbackDelegate.get()) != delegateToTimerMap.end())
			{
				return ETimerOperationResult::FAILURE_TIMER_FOR_DELEGATE_EXISTS;
			}

			sp<SA::Timer> timerInstance = timerPool.getInstance();
			timerInstance->set(callbackDelegate, durationSec, bLoop, delaySecs);

			delegateToTimerMap.insert({ callbackDelegate.get(), timerInstance });
			timers.insert(timerInstance);
		}

		return ETimerOperationResult::SUCCESS;
	}

	ETimerOperationResult TimeManager::removeTimer(const sp<MultiDelegate<>>& callbackDelegate)
	{
		auto findResult = delegateToTimerMap.find(callbackDelegate.get());
		if (findResult != delegateToTimerMap.end())
		{
			if (!bTickingTimers)
			{
				findResult->second->reset();
				timerPool.releaseInstance(findResult->second);

				timers.remove(findResult->second);
				findResult->second->reset();
				delegateToTimerMap.erase(findResult);
				return ETimerOperationResult::SUCCESS;
			}
			else
			{
				timersToRemoveWhenTickingOver.emplace_back(findResult->second);
				return ETimerOperationResult::DEFERRED;
			}
		}
		return ETimerOperationResult::FAILURE_TIMER_NOT_FOUND;

	}

	void TimeSystem::updateTime(PrivateKey key)
	{
		float currentTime = static_cast<float>(glfwGetTime());
		rawDeltaTimeSecs = currentTime - lastFrameTime;
		rawDeltaTimeSecs = rawDeltaTimeSecs > MAX_DELTA_TIME_SECS ? MAX_DELTA_TIME_SECS : rawDeltaTimeSecs;
		deltaTimeSecs = rawDeltaTimeSecs;
		lastFrameTime = currentTime;

		for (const sp<TimeManager>& manager : managers)
		{
			manager->update(TimeManager::PrivateKey{}, *this);
		}
	}

	void TimeSystem::markManagerCritical(PrivateKey, sp<TimeManager>& manager)
	{
		criticalManagers.insert(manager);
	}

	sp<TimeManager> TimeSystem::createManager()
	{
		sp<TimeManager> newManager = new_sp<TimeManager>();

		managers.insert(newManager);

		return newManager;
	}

	void TimeSystem::destroyManager(sp<TimeManager>& manager)
	{
		if (criticalManagers.find(manager) == criticalManagers.end())
		{
			managers.erase(manager);
			manager = nullptr;
		}
		else
		{
			log("TimeSystem", LogLevel::LOG_WARNING, "Attempting to destroy a system critical time manager");
		}
	}


}

