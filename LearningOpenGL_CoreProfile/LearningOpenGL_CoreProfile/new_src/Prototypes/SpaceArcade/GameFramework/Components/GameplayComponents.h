#pragma once
#include "../../Tools/DataStructures/MultiDelegate.h"
#include "SAComponentEntity.h"
#include "../SAAIBrainBase.h"
#include "../SAGameEntity.h"

namespace SA
{
	namespace BehaviorTree
	{
		class Tree;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// NOTE: none of these components contribute to the memory profile unless the current game instantiates one
	//		
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////////////////
	// team component
	/////////////////////////////////////////////////////////////////////////////////////
	class TeamComponent : public GameComponentBase
	{
	public:
		MultiDelegate<size_t /*old_team*/, size_t/*new_team*/> onTeamChanged;

		size_t getTeam() const { return teamIdx; }
		void setTeam(size_t teamIdx);
	private:
		size_t teamIdx = 0;
	};


	/////////////////////////////////////////////////////////////////////////////////////
	// brain component
	/////////////////////////////////////////////////////////////////////////////////////
	class BrainComponent : public GameComponentBase
	{
	public:
		template <typename BrainType, typename... Args>
		void spawnNewBrain(Args... args)
		{
			static_assert(std::is_base_of<AIBrain, BrainType>::value, "BrainType must be derived from AIBrain");
			setNewBrain(new_sp<BrainType>(std::forward<Args>(args)...));
		}
		void setNewBrain(const sp<AIBrain>& newBrain, bool bStartNow = true);
		AIBrain* getBrain() const { return brain.get(); }
	public:
		const BehaviorTree::Tree* getTree() const
		{ 
			return cachedBehaviorTree; 
		}
	private:

	public:

		sp<AIBrain> brain;
		BehaviorTree::Tree* cachedBehaviorTree = nullptr;
	};


	/////////////////////////////////////////////////////////////////////////////////////
	// Hit point Component
	/////////////////////////////////////////////////////////////////////////////////////
	struct HitPoints
	{
		float current;
		float max;

		friend bool operator==(const HitPoints& a, const HitPoints& b)
		{
			//this exists primarily to determine whether to not to broadcast event; hence float comparisons are acceptable to be exact match
			return a.current == b.current && a.max == b.max;
		}
		friend bool operator!=(const HitPoints& a, const HitPoints& b) { return !(a == b); }
	};

	class HitPointComponent : public GameComponentBase
	{
	private:
		HitPoints hp;
	public:
		void adjust(float delta);
		const HitPoints& getHP() const { return hp; }
		void overwriteHP(const HitPoints& newHP);
		void setDamageReductionFactor(float reductionFactor);
	public:
		MultiDelegate<const HitPoints& /*old*/, const HitPoints& /*current*/> onHpChangedEvent;
	private:
		float damageReductionFactor = 1;
	};


	class OwningPlayerComponent : public GameComponentBase
	{
	public:
		bool hasOwningPlayer() const { return !owningPlayer.expired(); }
		void setOwningPlayer(const sp<PlayerBase>& player);
		const wp<PlayerBase>& getOwningPlayer() const { return owningPlayer; }
	private:
		wp<PlayerBase> owningPlayer;
	};
}