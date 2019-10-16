#include "SAAIBrainBase.h"
#include "SALog.h"
#include <assert.h>
#include "..\Tools\DataStructures\MultiDelegate.h"
#include "SALevelSystem.h"
#include "SAGameBase.h"
#include "SALevel.h"

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

