#pragma once
#include "GameFramework/TimeManagement/TickGroupManager.h"
namespace SA
{
	struct SATickGroups : public TickGroups
	{
		static const SATickGroups& get();

		//calls parent ctor by default, add cust tickgroups here. These automatically register if this is made within the GameBase virtual
		TickGroupDefinition DemoGroup = TickGroupDefinition("DemoCustomGroup", 10.0f);
	};

}