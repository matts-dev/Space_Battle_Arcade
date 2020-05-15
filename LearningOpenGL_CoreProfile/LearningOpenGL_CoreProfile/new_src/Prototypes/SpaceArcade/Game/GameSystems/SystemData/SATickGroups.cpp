#include "SATickGroups.h"
#include "../../SpaceArcade.h"

namespace SA
{
	const SATickGroups& SATickGroups::get()
	{
		return SpaceArcade::get().saTickGroups();
	}

}


