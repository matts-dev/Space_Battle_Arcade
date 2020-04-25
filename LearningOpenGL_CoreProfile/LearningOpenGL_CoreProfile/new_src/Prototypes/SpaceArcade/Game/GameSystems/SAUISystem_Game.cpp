#include "SAUISystem_Game.h"

namespace SA
{
	void UISystem_Game::startGameUIPass() const
	{
		onUIGameRender.broadcast();
	}

}

