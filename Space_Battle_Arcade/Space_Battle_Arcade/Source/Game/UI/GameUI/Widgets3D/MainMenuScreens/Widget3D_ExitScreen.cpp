#include "Widget3D_ExitScreen.h"
#include "GameFramework/SAGameBase.h"

namespace SA
{
	void Widget3D_ExitScreen::tick(float dt_sec)
	{
		Parent::tick(dt_sec);
	}

	void Widget3D_ExitScreen::postConstruct()
	{
		Widget3D_MenuScreenBase::postConstruct();
		backButton = new_sp<Widget3D_LaserButton>("Cancel");
		enabledButtons.push_back(backButton.get());

		quitButton = new_sp<Widget3D_LaserButton>("Quit");
		quitButton->onClickedDelegate.addWeakObj(sp_this(), &Widget3D_ExitScreen::handleQuitClicked);
		enabledButtons.push_back(quitButton.get());
	}

	void Widget3D_ExitScreen::onActivationChanged(bool bActive)
	{
		using namespace glm;
		if (bActive)
		{
			//activated
			GameUIRenderData rd_ui = {};
			const HUDData3D& hd = rd_ui.getHUDData3D();

			vec3 centerPnt = hd.camPos + hd.camFront * hd.frontOffsetDist;
			float sideOffsetSafezonePerc = 0.5f;

			float buttonScale = 0.25f;
			Transform buttonXform = {};
			buttonXform.position = centerPnt + hd.camRight*(hd.savezoneMax_x*sideOffsetSafezonePerc);
			buttonXform.scale = vec3(buttonScale);
			buttonXform.rotQuat = rd_ui.camQuat();
			backButton->setXform(buttonXform);

			buttonXform.position = centerPnt + -hd.camRight*(hd.savezoneMax_x*sideOffsetSafezonePerc);
			quitButton->setXform(buttonXform);

			for (Widget3D_LaserButton* button : enabledButtons) { button->activate(bActive); } //for stagged anim, stagger in tick.
		}
		else
		{
			//deactivated
			for (Widget3D_LaserButton* button : enabledButtons) { button->activate(bActive); }
		}
	}

	void Widget3D_ExitScreen::renderGameUI(GameUIRenderData& ui_rd)
	{
		//the buttons automatically hook up to be rendered a part of their post construction.
	}

	void Widget3D_ExitScreen::handleQuitClicked()
	{
		GameBase::get().startShutdown();
	}

}

