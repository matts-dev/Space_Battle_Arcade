#include "GameplayComponents.h"
#include "../../Tools/DataStructures/SATransform.h"

namespace SA
{

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// team component
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void TeamComponent::setTeam(size_t inTeamIdx)
	{
		// anyone can change the team; this may be a problem later.
		// making this public easier behavior (mind control etc), but is an encapsulation problem.
		// perhaps there should be a "lock" where setting team can be locked/unlocked and that is controlled by owner.

		size_t oldTeamIdx = teamIdx;
		teamIdx = inTeamIdx;
		onTeamChanged.broadcast(oldTeamIdx, teamIdx);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// brain component
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void BrainComponent::setNewBrain(const sp<AIBrain>& newBrain, bool bStartNow /*= true*/)
	{
		if (brain)
		{
			brain->sleep();
		}

		//update brain to new brain after stopping previous; should not allow two brains to operate on a single ship 
		brain = newBrain;
		if (newBrain && bStartNow)
		{
			newBrain->awaken();
		}

		cachedBehaviorTree = nullptr;
		if(BehaviorTreeBrain* btBrain = dynamic_cast<BehaviorTreeBrain*>(newBrain.get()))
		{
			cachedBehaviorTree = &btBrain->getBehaviorTree();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// hit point component
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void HitPointComponent::adjust(float delta)
	{
		if (delta < 0)
		{
			delta = damageReductionFactor * delta;
		}

		HitPoints old = hp;
		hp.current += delta;
		hp.current = glm::clamp(hp.current, 0.f, hp.max);

		if(old != hp) 
		{ 
			onHpChangedEvent.broadcast(old, hp);
		}
	}

	void HitPointComponent::overwriteHP(const HitPoints& newHP)
	{
		HitPoints old = hp;
		hp = newHP;

		if (old != hp)
		{
			onHpChangedEvent.broadcast(old, hp);
		}
	}

	void HitPointComponent::setDamageReductionFactor(float reductionFactor)
	{
		if (reductionFactor != 0.f)
		{
			damageReductionFactor = reductionFactor;
		}
	}

	void OwningPlayerComponent::setOwningPlayer(const sp<PlayerBase>& player)
	{
		//#TODO broadcast event if needed
		owningPlayer = player;
	}

}

