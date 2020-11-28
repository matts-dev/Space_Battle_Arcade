#include "Widget3D_ModsScreen.h"
#include "../../../../SpaceArcade.h"

namespace SA
{


	void Widget3D_ModsScreen::tick(float dt_sec)
	{
		Parent::tick(dt_sec);
	}

	void Widget3D_ModsScreen::postConstruct()
	{
		Widget3D_MenuScreenBase::postConstruct();
		backButton = new_sp<Widget3D_LaserButton>("Back");
		enabledButtons.push_back(backButton.get());
	}

	void Widget3D_ModsScreen::onActivationChanged(bool bActive)
	{
		Parent::onActivationChanged(bActive);

		SpaceArcade& game = SpaceArcade::get();

		if (bActive)
		{
			//activated
			configureButtonToDefaultBackPosition(*backButton);
			for (Widget3D_LaserButton* button : enabledButtons) { button->activate(bActive); } //for stagged anim, stagger in tick.

			if (game.isEditorMainmenuFeatureEnabled() && !game.isEditorMainMenuOnScreen())
			{
				game.toggleEditorUIMainMenuVisible();
			}
		}
		else
		{
			//deactivated
			for (Widget3D_LaserButton* button : enabledButtons) { button->activate(bActive); }

			if (game.isEditorMainmenuFeatureEnabled() && game.isEditorMainMenuOnScreen())
			{
				game.toggleEditorUIMainMenuVisible();
			}
		}
	}

	void Widget3D_ModsScreen::renderGameUI(GameUIRenderData& ui_rd)
	{
		//the buttons automatically hook up to be rendered a part of their post construction.
	}

}

