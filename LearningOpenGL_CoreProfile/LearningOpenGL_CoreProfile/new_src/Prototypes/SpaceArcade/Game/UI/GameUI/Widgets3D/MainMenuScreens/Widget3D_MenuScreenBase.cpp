#include "Widget3D_MenuScreenBase.h"

namespace SA
{

	void Widget3D_MenuScreenBase::tick(float dt_sec)
	{
		//Parent::tick(dt_sec);

		for (Widget3D_LaserButton* button : enabledButtons)
		{
			if (button)
			{
				button->tick(dt_sec);
			}
		}
	}

	void Widget3D_MenuScreenBase::postConstruct()
	{
		bRegisterForGameUIRender = true; //must be called before post construct

		Parent::postConstruct();
	}


	void Widget3D_MenuScreenBase::configureButtonToDefaultBackPosition(Widget3D_LaserButton& button) const
	{
		using namespace glm;

		GameUIRenderData rd_ui = {};
		const HUDData3D& hd = rd_ui.getHUDData3D();

		vec3 centerPnt = hd.camPos + hd.camFront * hd.frontOffsetDist;
		vec3 position = centerPnt + hd.camUp*(-hd.savezoneMax_y*0.75f) + hd.camRight*(hd.savezoneMax_x*0.75f);

		Transform buttonXform = {};
		buttonXform.position = position;
		buttonXform.scale = vec3(0.25f);
		buttonXform.rotQuat = rd_ui.camQuat();
		button.setXform(buttonXform);
		//not activating button as screen may want to activate button as part of an animation.
	}

}
