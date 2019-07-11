#pragma once
#include "SAAIBrainBase.h"

namespace SA
{
	class Ship;
	class LevelBase;
	class TimeManager;
	template<typename... Args>
	class MultiDelegate;


	class ShipAIBrain : public AIBrain
	{
	public:
		ShipAIBrain(const sp<Ship>& controlledShip);

		virtual bool onAwaken() override;
		virtual void onSleep() override;

		
	protected:
		//#TODO use a softPtr rather than weakpointer, so that not "locking" every tick. but those don't exist yet
		wp<Ship> controlledTarget;
		wp<LevelBase> wpLevel;
	};

	class ContinuousFireBrain : public ShipAIBrain
	{
	public:
		ContinuousFireBrain(const sp<Ship>& controlledShip) : ShipAIBrain(controlledShip){}
		virtual bool onAwaken() override;
		virtual void onSleep() override;
		void setFireRateSecs(float fireRateSecs);

		void setDelayStartFire(float delayFirstFireSecs);
	private:
		void createTimerForFire();
		void removeFireTimer(TimeManager& worldTM);
		void handleTimeToFire();
	private:
		sp<MultiDelegate<>> fireDelegate;
		float fireRateSec = 1.0f;
		float timerDelay = 0.f;
	};

	class FlyInDirectionBrain : public ShipAIBrain
	{
	public:
		FlyInDirectionBrain(const sp<Ship>& controlledShip) : ShipAIBrain(controlledShip) {}
		virtual bool onAwaken() override;
	private:
		float speed = 1.f;
	};

}