#include "SATickGroups.h"
#include "Game/SpaceArcade.h"

namespace SA
{
	const SATickGroups& SATickGroups::get()
	{
		return SpaceArcade::get().saTickGroups();
	}

}


