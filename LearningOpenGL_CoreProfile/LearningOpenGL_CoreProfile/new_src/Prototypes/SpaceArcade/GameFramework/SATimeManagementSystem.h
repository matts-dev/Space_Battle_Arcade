#pragma once

#include "SAGameEntity.h"
#include <set>
#include "../Tools/RemoveSpecialMemberFunctionUtils.h"
#include "../Tools/DataStructures/MultiDelegate.h"
#include "../Tools/DataStructures/IterableHashSet.h"
#include <unordered_map>
#include "Interfaces/SATickable.h"

namespace SA
{
	//forward declarations
	template<typename T>
	class IterableHashSet;

	struct TickGroupDefinition;

	//#consider this may be better suited as bit-vector for masking operations (eg SUCCESS = DEFERRED | REMOVED | ADDED) 
	enum class ETimerOperationResult : char
	{
		SUCCESS = 0,
		FAILURE_TIMER_FOR_DELEGATE_EXISTS,
		FAILURE_TIMER_NOT_FOUND,
		FAILURE_NEGATIVE_DURATION,
		DEFER_FAILURE_TIMER_FOR_DELEGATE_EXISTS,
		DEFER_FAILURE_DELEGATE_ALREADY_PENDING_ADD,
		DEFERRED
	};

	struct Timer
	{
	public:
		/* @returns true when timer is complete and should be removed */
		bool update(float dt_sec_dilated);
		void reset();
		void set(const sp<MultiDelegate<>>& callbackDelegate, float duration, bool bLoop, float delaySecs);
		const sp<MultiDelegate<>>& getUserCallback() { return userCallback; }

	private:
		float durationSecs = 0.f;
		float currentTime = 0.f;
		bool bLoop = false;
		sp<MultiDelegate<>> userCallback;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//An object that manipulates time; this allows creating time systems based on the true time, but with effects like 
	//time dilation and time stepping and setting timers influenced on those effects
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class TimeManager final : public RemoveCopies, public RemoveMoves
	{
	public:
		TimeManager();
		virtual ~TimeManager() = default;
		 
	public:
		/* Private key only allows friends to call ctor */
		struct PrivateKey { private: friend class TimeSystem; PrivateKey() {}; };
		void update(PrivateKey key, class TimeSystem& timeSystem);

	public:
		inline void setTimeFreeze(bool bInFreezeTime) { bFreezeTime = bInFreezeTime; }
		inline void setFramesToStep(unsigned int frames) { newFramesToStep = frames; }
		/*Changing time dilation mid-frame is not supported as it would cause havoc and setting order-depend behavior*/
		inline void setTimeDilationFactor_OnNextFrame(float inTimeDilationFactor) { DilationFactor_nextFrame = inTimeDilationFactor; }

		inline float getDeltaTimeSecs() const {return dt_dilatedSecs; }
		inline float getUndilatedTimeSecs() const { return dt_undilatedSecs; }
		inline float getTimeDilationFactor() const { return timeDilationFactor; }
		inline float getTimestampSecs() const { return timeSinceStartDilatedSecs; }
		inline int getRemaningFramesToStep() const { return framesToStep; }
		inline bool isTimeFrozen() const { return bFreezeTime && framesToStep == 0; }
		inline bool isFrameStepping() const { return bFreezeTime && framesToStep > 0; }
		MultiDelegate<float /*dt_sec*/>& getEvent(const TickGroupDefinition& tickGroupDeclaration);

	public: //timers
		/** timer functions returning bool indicate success/failure */
		ETimerOperationResult createTimer(const sp<MultiDelegate<>>& callbackDelegate, float durationSec, bool bLoop = false, float delaySecs = 0.f);

		/* notes: will if timer is going to tick this frame, the timer will tick regardless of if timer is removed */
		ETimerOperationResult removeTimer(const sp<MultiDelegate<>>& callbackDelegate);
		bool hasTimerForDelegate(const sp<MultiDelegate<>>& timerBoundDelegate);

	public: //tickers
		//#optimize ticking using virtual dispatch is probably an unneccsary perf hit. 
		void registerTicker(const sp<ITickable>& tickable);
		void removeTicker(const sp<ITickable>& tickable);
		bool hasRegisteredTicker(const sp<ITickable>& tickable);

	private:
		//next frame pattern prevents affects from happening mid-frame
		float dt_undilatedSecs = 0.f;
		float dt_dilatedSecs = 0.f;
		float timeSinceStartDilatedSecs = 0.f;

		int framesToStep = 0;
		int newFramesToStep = 0;
		bool bFreezeTime = false;

		float timeDilationFactor = 1.f;
		float DilationFactor_nextFrame = 1.f;

		bool bTickingTimers = false;

		IterableHashSet<sp<Timer>> timers;
		std::unordered_map<MultiDelegate<>*, sp<Timer>> delegateToTimerMap;

		////////////////////////////////////////////////////////
		//Timer helper data structures; 
		////////////////////////////////////////////////////////
		//helpers to add timers if user attempts to set timer while the timers are ticking
		IterableHashSet<sp<Timer>> deferredTimersToAdd;
		std::unordered_map<MultiDelegate<>*, sp<Timer>> deferredTimerDelegatesToAdd;
		//deferred removes
		std::vector<sp<Timer>> timersToRemoveWhenTickingOver;
		uint32_t removeTimerReservationSpace = 20;
		uint32_t tickerDeferredRegistrationMinBufferSize = 100;

		////////////////////////////////////////////////////////
		// Ticker helper data structures
		////////////////////////////////////////////////////////
		bool bIsTickingTickables = false;
		IterableHashSet<sp<ITickable>> tickables;
		IterableHashSet<sp<ITickable>> pendingRemovalTickables;
		IterableHashSet<sp<ITickable>> pendingAddTickables;

		//#todo perhaps replace ITickable and only have tick groups. I think ITickable currently has better performance.
		struct TickGroupEntry 
		{
			std::string name;
			float priority = 0.f;
			size_t sortIdx = 0;
			sp<MultiDelegate<float /*dt_sec*/>> onTick = nullptr;
		};
		std::vector<TickGroupEntry> tickGroups;
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Time system is a special system, above all systems and is strongly coupled with game base 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class TimeSystem
	{
	public:
		inline float getCurrentTime() const { return currentTime; };
		inline float getLastFrameTime() const { return lastFrameTime; };
		inline float getRawDeltaTimeSecs() const { return rawDeltaTimeSecs; };
		inline float getDeltaTimeSecs() const { return deltaTimeSecs; };
		inline float getMAX_DELTA_TIME_SECS() const { return MAX_DELTA_TIME_SECS; };

		/* Private key only allows friends to call ctor*/
		struct PrivateKey { private: friend class GameBase; PrivateKey() {}; };
		void updateTime(PrivateKey);
		void markManagerCritical(PrivateKey, sp<TimeManager>& manager);

	public:
		sp<TimeManager> createManager();
		void destroyManager(sp<TimeManager>& worldTimeManager);

	private:
		float currentTime = 0;
		float lastFrameTime = 0;
		float rawDeltaTimeSecs = 0;
		float deltaTimeSecs = 0.f;
		float MAX_DELTA_TIME_SECS = 0.5f;

		std::set<sp<TimeManager>> managers;
		std::set<sp<TimeManager>> criticalManagers;
	};

}