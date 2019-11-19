#pragma once
#include "../../GameFramework/SAAIBrainBase.h"
#include "../../GameFramework/SATimeManagementSystem.h"

namespace SA
{
	class Ship;
	class LevelBase;
	class TimeManager;
	template<typename... Args>
	class MultiDelegate;

	namespace BehaviorTree
	{
		class Tree;
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Base ship brain;
	//		-controls a ship object
	//		- keeps track of current level.
	/////////////////////////////////////////////////////////////////////////////////////
	class ShipAIBrain : public BehaviorTreeBrain
	{
	public:
		ShipAIBrain(const sp<Ship>& controlledShip);

		virtual bool onAwaken() override;
		virtual void onSleep() override;

	public:
		Ship* getControlledTarget();
		const Ship* getControlledTarget() const;
		wp<Ship> getWeakControlledTarget() { return controlledTarget; }
		
	protected:
		//#TODO use a softPtr rather than weak pointer, so that not "locking" every tick. but those don't exist yet
		wp<Ship> controlledTarget;
		wp<LevelBase> wpLevel;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Brain that continuously firing and flying in direction; use for perf testing
	/////////////////////////////////////////////////////////////////////////////////////
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
	protected:
		virtual void postConstruct() override;

	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Brain that just flies in a single direction, used for testing.
	/////////////////////////////////////////////////////////////////////////////////////
	class FlyInDirectionBrain : public ShipAIBrain
	{
	public:
		FlyInDirectionBrain(const sp<Ship>& controlledShip) : ShipAIBrain(controlledShip) {}
		virtual bool onAwaken() override;
		void postConstruct();
	private:
		float speed = 1.f;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// A behavior tree for a docile ship that randomly wanders about its local vicinity
	/////////////////////////////////////////////////////////////////////////////////////
	class WanderBrain : public ShipAIBrain
	{
	public:
		WanderBrain(const sp<Ship>& controlledShip) : ShipAIBrain(controlledShip) {}
	protected:
		virtual void postConstruct() override;
	};

	////////////////////////////////////////////////////////
	// The primary behavior tree for a fighter ship.
	////////////////////////////////////////////////////////
	class EvadeTestBrain : public ShipAIBrain
	{
	public:
		EvadeTestBrain(const sp<Ship>& controlledShip) : ShipAIBrain(controlledShip) {}

	protected:
		virtual void postConstruct() override;
	};

	////////////////////////////////////////////////////////
	// The primary behavior tree for a fighter ship.
	////////////////////////////////////////////////////////
	class FighterBrain : public ShipAIBrain
	{
	public:
		FighterBrain(const sp<Ship>& controlledShip) : ShipAIBrain(controlledShip) {}

	protected:
		virtual void postConstruct() override;
	};

}