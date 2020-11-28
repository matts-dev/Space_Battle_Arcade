#pragma once
#include "SAGameEntity.h"
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <assert.h>
#include "../Tools/DataStructures/MultiDelegate.h"
#include "Interfaces/SATickable.h"

namespace SA
{
	//forward declarations
	namespace BehaviorTree
	{
		class Tree;
	}
	class LevelBase;

	//#TODO this probably needs to exist separately in another header, so  player can do pattern of having protected member return a key
	/** Special key to allows brains (ai/player) to access methods
		Only friend classes can construct these, giving the friend
		classes unique access to methods requiring an instance as
		a parameter.*/
	class BrainKey
	{
		friend class PlayerBase;
		friend class AIBrain;
	};

	class AIBrain : public GameEntity
	{
	public:
		bool awaken();
		void sleep();
		bool isActive() { return bActive; }

	protected:
		/** return true if successful */
		virtual bool onAwaken() = 0;
		virtual void onSleep() = 0;

	protected:
		/** Give subclasses easy access to a BrainKey; copy elision should occur*/
		inline BrainKey getBrainKey() { return BrainKey{}; }

	private:
		bool bActive = false;
	};


	/////////////////////////////////////////////////////////////////////////////////////
	// 	A brain that uses a behavior tree to make decisions
	//		-expects behavior tree to be set in the child postConstruct method
	/////////////////////////////////////////////////////////////////////////////////////
	class BehaviorTreeBrain : public AIBrain, public ITickable
	{
	public:
		BehaviorTreeBrain(){}

	public:
		virtual bool onAwaken() override;
		virtual void onSleep() override;

		BehaviorTree::Tree& getBehaviorTree() { return *behaviorTree.get(); }
		const BehaviorTree::Tree& getBehaviorTree() const { return *behaviorTree.get(); }

	protected:
		virtual bool tick(float dt_sec) override;

	protected:
		sp<BehaviorTree::Tree> behaviorTree;
		wp<LevelBase> tickingOnLevel;
	};

}