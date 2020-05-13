#include "Widget3D_Base.h"
#include "../../../SpaceArcade.h"
#include "../../../GameSystems/SAUISystem_Game.h"


namespace SA
{

	void Widget3D_Base::setRenderWithGameUIDispatch(bool bRender)
	{
		const sp<UISystem_Game>& gameUISystem = SpaceArcade::get().getGameUISystem();

		if (bRender)
		{
			if (!gameUISystem->onUIGameRender.hasBoundWeak(*this))
			{
				gameUISystem->onUIGameRender.addWeakObj(sp_this(), &Widget3D_Base::renderGameUI); //if made weak, be sure to change strong check to weak check (above)
			}
		}
		else
		{
			gameUISystem->onUIGameRender.removeWeak(sp_this(), &Widget3D_Base::renderGameUI);
		}
	}

	void Widget3D_Base::onDestroyed()
	{
		//remove strong delegate if this object is flagged as destroyed
		setRenderWithGameUIDispatch(false);
	}

}

