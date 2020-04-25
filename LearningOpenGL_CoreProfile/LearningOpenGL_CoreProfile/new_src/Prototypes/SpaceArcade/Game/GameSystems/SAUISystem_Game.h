#pragma once
#include "../../GameFramework/SASystemBase.h"
#include "../../Tools/DataStructures/MultiDelegate.h"

struct GLFWwindow;

namespace SA
{
	class UISystem_Game : public SystemBase
	{
		friend class SpaceArcade;
		void startGameUIPass() const;
	public:
		mutable MultiDelegate<> onUIGameRender;
	};
}