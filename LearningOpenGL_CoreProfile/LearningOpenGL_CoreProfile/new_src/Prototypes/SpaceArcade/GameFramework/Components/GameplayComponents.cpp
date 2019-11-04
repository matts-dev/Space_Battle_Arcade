#include "GameplayComponents.h"

namespace SA
{
	void TeamComponent::setTeam(size_t inTeamIdx)
	{
		// anyone can change the team; this may be a problem later.
		// making this public easier behavior (mind control etc), but is an encapsulation problem.
		// perhaps there should be a "lock" where setting team can be locked/unlocked and that is controlled by owner.

		size_t oldTeamIdx = teamIdx;
		teamIdx = inTeamIdx;
		onTeamChanged.broadcast(oldTeamIdx, teamIdx);
	}

}

