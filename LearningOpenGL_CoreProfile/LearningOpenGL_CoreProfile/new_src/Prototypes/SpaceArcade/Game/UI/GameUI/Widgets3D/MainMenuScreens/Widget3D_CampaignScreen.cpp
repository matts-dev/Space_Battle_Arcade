
#include "Widget3D_CampaignScreen.h"
#include "../../../../GameSystems/SAModSystem.h"
#include "../../../../SpaceArcade.h"
#include "../../../../AssetConfigs/CampaignConfig.h"

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
		Parent::onActivationChanged(bActive);
		if (bActive)
		{
			//activated
			getCampaign();

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

	void Widget3D_CampaignScreen::getCampaign()
	{
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			activeCampaign = activeMod->getCampaign(activeCampaignIdx);
			assert(activeCampaign); //only should be null if we're passing an invalid index. Currently there is only 1 but in future may support multiple

			//DEBUG -- force a save to generate a template
			//activeCampaign->requestSave();
		}
	}

}

