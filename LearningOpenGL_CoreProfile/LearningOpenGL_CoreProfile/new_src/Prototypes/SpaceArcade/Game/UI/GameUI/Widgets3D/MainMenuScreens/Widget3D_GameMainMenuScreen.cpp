#include "Widget3D_GameMainMenuScreen.h"
#include "../Widget3D_LaserButton.h"
#include "../../../../SpaceArcade.h"

namespace SA
{
	void Widget3D_GameMainMenuScreen::postConstruct()
	{
		campaignButton = new_sp<Widget3D_LaserButton>("Campaign");

		SpaceArcade::get().getGameUISystem()->onUIGameRender.addWeakObj(sp_this(), &Widget3D_GameMainMenuScreen::renderGameUI);
	}

	void Widget3D_GameMainMenuScreen::onActivationChanged(bool bActive)
	{
		using namespace glm;

		GameUIRenderData rd_ui = {};

		if (bActive)
		{
			const HUDData3D& hd = rd_ui.getHUDData3D();

			vec3 centerPnt = hd.camPos + hd.camFront * hd.frontOffsetDist;
			vec3 topPoint = centerPnt + hd.camUp*(hd.savezoneMax_y*0.75f);

			//align buttons vertically, start from top and push down based on width
			Transform buttonXform = {};
			buttonXform.position = topPoint;
			buttonXform.scale = vec3(0.25f);
			buttonXform.rotQuat = rd_ui.camQuat();
			campaignButton->setXform(buttonXform);

			//offset next button based on the button aboves height; include scale!
		}
		else
		{
			//deactivated
		}

		campaignButton->activate(bActive);

	}

	void Widget3D_GameMainMenuScreen::renderGameUI(GameUIRenderData& ui_rd)
	{
		//the buttons automatically hook up to be rendered a part of their post construction.
	}

}

