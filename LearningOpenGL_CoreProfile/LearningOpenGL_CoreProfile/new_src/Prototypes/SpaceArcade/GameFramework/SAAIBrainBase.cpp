#include "SAAIBrainBase.h"

namespace SA
{
	bool AIBrain::awaken()
	{
		bActive = onAwaken();
		return bActive;
	}

	void AIBrain::sleep()
	{
		onSleep();
		bActive = false;
	}

}

