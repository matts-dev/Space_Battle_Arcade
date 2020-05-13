#pragma once
#include "Widget3D_ActivatableBase.h"

namespace SA
{
	class Widget3D_LaserButton;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The in game (not editor) main menu screen. 
	//	  This is the entry point of the game.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_GameMainMenuScreen : public Widget3D_ActivatableBase
	{
	protected:
		virtual void postConstruct() override;
		virtual void onActivationChanged(bool bActive) override;
	private:
		virtual void renderGameUI(GameUIRenderData& ui_rd) override;
	private:
		sp<Widget3D_LaserButton> campaignButton = nullptr;
	};

}
