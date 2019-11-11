#include "GameplayComponents.h"

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

}

