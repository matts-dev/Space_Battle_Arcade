#pragma once
#include "../../GameFramework/SASystemBase.h"
#include "../../Tools/DataStructures/MultiDelegate.h"

struct GLFWwindow;

namespace SA
{
	class UISystem_Game : public SystemBase
	{
	public:
		MultiDelegate<> onUIGameRender;
	};
}