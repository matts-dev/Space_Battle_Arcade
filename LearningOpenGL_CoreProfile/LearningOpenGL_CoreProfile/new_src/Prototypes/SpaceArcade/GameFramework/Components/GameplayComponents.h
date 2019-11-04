#pragma once
#include "..\..\Tools\DataStructures\MultiDelegate.h"
#include "SAComponentEntity.h"

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// NOTE: none of these components contribute to the memory profile unless the current game instantiates one
	//		
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class TeamComponent : public GameComponentBase
	{
	public:
		MultiDelegate<size_t /*old_team*/, size_t/*new_team*/> onTeamChanged;

		size_t getTeam() const { return teamIdx; }
		void setTeam(size_t teamIdx);
	private:
		size_t teamIdx = 0;
	};

}