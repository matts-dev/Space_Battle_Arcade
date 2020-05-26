
#include "Widget3D_CampaignScreen.h"

namespace SA
{
	void Widget3D_CampaignScreen::tick(float dt_sec)
	{
		Parent::tick(dt_sec);
	}

	void Widget3D_CampaignScreen::postConstruct()
	{
		Widget3D_MenuScreenBase::postConstruct();
		backButton = new_sp<Widget3D_LaserButton>("Back");
		enabledButtons.push_back(backButton.get());
	}

	void Widget3D_CampaignScreen::onActivationChanged(bool bActive)
	{
		if (bActive)
		{
			//activated
			configureButtonToDefaultBackPosition(*backButton);
			for (Widget3D_LaserButton* button : enabledButtons) { button->activate(bActive); } //for stagged anim, stagger in tick.
		}
		else
		{
			//deactivated
			for (Widget3D_LaserButton* button : enabledButtons) { button->activate(bActive); }
		}
	}

	void Widget3D_CampaignScreen::renderGameUI(GameUIRenderData& ui_rd)
	{
		//the buttons automatically hook up to be rendered a part of their post construction.
	}

}

