#include "Widget3D_GameMainMenuScreen.h"
#include "../Widget3D_LaserButton.h"
#include "../../../../SpaceArcade.h"
#include "../../../../../Tools/PlatformUtils.h"

namespace SA
{
	void Widget3D_GameMainMenuScreen::tick(float dt_sec)
	{
		if (isActive())
		{
			animInTime += dt_sec;
			tryShowNewButton(dt_sec);
		}
		else
		{
			animInTime = 0.f;
			animInButtonIdx = std::nullopt;
		}

		for (Widget3D_LaserButton* button : enabledButtons)
		{
			if (button) 
			{
				button->tick(dt_sec);
			}
		}
	}

	void Widget3D_GameMainMenuScreen::postConstruct()
	{
		campaignButton = new_sp<Widget3D_LaserButton>("Campaign");
		enabledButtons.push_back(campaignButton.get());

		skirmishButton = new_sp<Widget3D_LaserButton>("Skirmish");
		enabledButtons.push_back(skirmishButton.get());

		modsButton = new_sp<Widget3D_LaserButton>("Mods");
		enabledButtons.push_back(modsButton.get());

		settingsButton = new_sp<Widget3D_LaserButton>("settings");
		enabledButtons.push_back(settingsButton.get());

		exitButton = new_sp<Widget3D_LaserButton>("Exit");
		enabledButtons.push_back(exitButton.get());

		SpaceArcade::get().getGameUISystem()->onUIGameRender.addWeakObj(sp_this(), &Widget3D_GameMainMenuScreen::renderGameUI);

		sigmoid = GameBase::get().getCurveSystem().generateSigmoid_medp(20.0f);
	}

	void Widget3D_GameMainMenuScreen::onActivationChanged(bool bActive)
	{
		using namespace glm;

		if (bActive)
		{
			//alignButtonPositionsVertically();			//now done in tick to space out button anim-in
			
		}
		else
		{
			//deactivated
		}

		//for (Widget3D_LaserButton* button : enabledButtons)
		//{
		//	//perhaps this should be staggered? enabling each some time after the first.
		//	button->activate(bActive);
		//}

	}

	void Widget3D_GameMainMenuScreen::renderGameUI(GameUIRenderData& ui_rd)
	{
		//the buttons automatically hook up to be rendered a part of their post construction.
	}

	//void Widget3D_GameMainMenuScreen::alignButtonPositionsVertically()
	//{
	//	using namespace glm;

	//	GameUIRenderData rd_ui = {};
	//	const HUDData3D& hd = rd_ui.getHUDData3D();

	//	//writing this in a way that it will be easy to stagger the indices
	//	for (size_t btnIdx = 0; btnIdx < enabledButtons.size(); ++btnIdx)
	//	{
	//		if (Widget3D_LaserButton* button = enabledButtons[btnIdx])
	//		{
	//			vec3 position = vec3(0.f);
	//			if (btnIdx == 0)
	//			{
	//				vec3 centerPnt = hd.camPos + hd.camFront * hd.frontOffsetDist;
	//				vec3 topPoint = centerPnt + hd.camUp*(hd.savezoneMax_y*0.75f);
	//				position = topPoint;
	//			}
	//			else if (btnIdx -1 < enabledButtons.size())
	//			{
	//				if (Widget3D_LaserButton* previousButton = enabledButtons[btnIdx - 1])
	//				{
	//					position = previousButton->getXform().position;
	//					position += hd.camUp * -1.f * (buttonSpacing + previousButton->getSize().y);

	//				} else { STOP_DEBUGGER_HERE(); } //should always have a previous button
	//			}

	//			//align buttons vertically, start from top and push down based on width
	//			Transform buttonXform = {};
	//			buttonXform.position = position;
	//			buttonXform.scale = vec3(0.25f);
	//			buttonXform.rotQuat = rd_ui.camQuat();
	//			button->setXform(buttonXform);
	//		}
	//		else
	//		{
	//			log(__FUNCTION__, LogLevel::LOG_WARNING, "Empty button in button list? this shouldn't happen!");
	//			STOP_DEBUGGER_HERE();
	//		}
	//	}



	//	//offset next button based on the button aboves height; include scale!
	//}

	void Widget3D_GameMainMenuScreen::tryShowNewButton(float dt_sec)
	{
		using namespace glm;

		if (!animInButtonIdx.has_value() || animInButtonIdx != enabledButtons.size())
		{
			float buttonAnimFrac = animInTime / showAllButtonsAnimDurSec;
			float curvedButtonAnimFrac = buttonAnimFrac;// sigmoid.eval_smooth(buttonAnimFrac);
			size_t newButtonIdx = glm::clamp<size_t>(size_t(enabledButtons.size() * curvedButtonAnimFrac), 0, enabledButtons.size() - 1);

			if (!animInButtonIdx.has_value() || newButtonIdx > *animInButtonIdx) //only do this once we cross into a new button index
			{
				animInButtonIdx = newButtonIdx;

				GameUIRenderData rd_ui = {};
				const HUDData3D& hd = rd_ui.getHUDData3D();

				if (Widget3D_LaserButton* button = enabledButtons[*animInButtonIdx])
				{
					vec3 position = vec3(0.f);
					if (*animInButtonIdx == 0)
					{
						vec3 centerPnt = hd.camPos + hd.camFront * hd.frontOffsetDist;
						vec3 topPoint = centerPnt + hd.camUp*(hd.savezoneMax_y*0.75f);
						position = topPoint;
					}
					else if (*animInButtonIdx - 1 < enabledButtons.size())
					{
						if (Widget3D_LaserButton* previousButton = enabledButtons[*animInButtonIdx - 1])
						{
							position = previousButton->getXform().position;
							position += hd.camUp * -1.f * (buttonSpacing + previousButton->getSize().y);

						}
						else { STOP_DEBUGGER_HERE(); } //should always have a previous button
					}

					//align buttons vertically, start from top and push down based on width
					Transform buttonXform = {};
					buttonXform.position = position;
					buttonXform.scale = vec3(0.25f);
					buttonXform.rotQuat = rd_ui.camQuat();
					button->setXform(buttonXform);
					button->activate(true);
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_WARNING, "Empty button in button list? this shouldn't happen!");
					STOP_DEBUGGER_HERE();
				}

				//increment button index after it has been used
				//++animInButtonIdx; //this is now curve driven
				//animInTime = 0.f;
			}
		}
	}

}

