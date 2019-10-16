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
	class ShipAIBrain : public AIBrain
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
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// Brain that just flies in a single direction, used for testing.
	/////////////////////////////////////////////////////////////////////////////////////
	class FlyInDirectionBrain : public ShipAIBrain
	{
	public:
		FlyInDirectionBrain(const sp<Ship>& controlledShip) : ShipAIBrain(controlledShip) {}
		virtual bool onAwaken() override;
	private:
		float speed = 1.f;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// 	A brain that uses a behavior tree to make decisions
	//		-expects behavior tree to be set in the postConstruct method
	/////////////////////////////////////////////////////////////////////////////////////
	class BehaviorTreeBrain : public ShipAIBrain, public ITickable
	{
	public:
		BehaviorTreeBrain(const sp<Ship>& controlledShip) : ShipAIBrain(controlledShip) {}

	public:
		virtual bool onAwaken() override;
		virtual void onSleep() override;

		BehaviorTree::Tree& getBehaviorTree() { return *behaviorTree.get(); }
		const BehaviorTree::Tree& getBehaviorTree() const { return *behaviorTree.get(); }

	protected:
		virtual void postConstruct() override;

		virtual bool tick(float dt_sec) override;

	protected:
		sp<BehaviorTree::Tree> behaviorTree;
		wp<LevelBase> tickingOnLevel;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	// A behavior tree for a docile ship that randomly wanders about its local vicinity
	/////////////////////////////////////////////////////////////////////////////////////
	class WanderBrain : public BehaviorTreeBrain
	{
	public:
		WanderBrain(const sp<Ship>& controlledShip) : BehaviorTreeBrain(controlledShip) {}
	protected:
		virtual void postConstruct() override;
	};


	////////////////////////////////////////////////////////
	// The primary behavior tree for a fighter ship.
	////////////////////////////////////////////////////////
	class FighterBrain : public BehaviorTreeBrain
	{
	public:
		FighterBrain(const sp<Ship>& controlledShip) : BehaviorTreeBrain(controlledShip) {}

	protected:
		virtual void postConstruct() override;
	};

}