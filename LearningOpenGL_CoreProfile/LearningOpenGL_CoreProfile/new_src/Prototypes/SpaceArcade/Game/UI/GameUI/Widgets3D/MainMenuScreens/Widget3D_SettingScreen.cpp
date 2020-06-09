#include "Widget3D_SettingScreen.h"
#include "../../../../SpaceArcade.h"
#include "../Widget3D_DiscreteSelector.h"
#include "../../../../GameSystems/SAUISystem_Game.h"
#include "../Widget3D_Slider.h"

namespace SA
{

	void Widget3D_SettingsScreen::tick(float dt_sec)
	{
		Parent::tick(dt_sec);
	}

	void Widget3D_SettingsScreen::postConstruct()
	{
		backButton = new_sp<Widget3D_LaserButton>("Back");
		enabledButtons.push_back(backButton.get());

		selector_devConsole = new_sp<Widget3D_DiscreteSelector<size_t>>();
		allSelectors.push_back(selector_devConsole.get());
		std::vector<size_t> devConsoleOnChoices = { 0, 1 };
		selector_devConsole->setSelectables(devConsoleOnChoices);
		selector_devConsole->setIndex(SpaceArcade::get().bEnableDevConsoleFeature ? 1 : 0);
		selector_devConsole->setToString([](size_t value)
		{
			return std::string("Developer Console: ") + std::string((value == 1) ? "on" : "off");
		});
		selector_devConsole->onValueChangedDelegate.addWeakObj(sp_this(), &Widget3D_SettingsScreen::handleDevConsoleChanged); //TODO really need weaklambda support for these kinds of things

		slider_masterAudio = new_sp<Widget3D_Slider>();
		allSliders.push_back(slider_masterAudio.get());

		//defines the order of selectors/sliders
		ordered_options.push_back(selector_devConsole.get());
		ordered_options.push_back(slider_masterAudio.get());
	}

	void Widget3D_SettingsScreen::onActivationChanged(bool bActive)
	{
		Parent::onActivationChanged(bActive);
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
		for (Widget3D_DiscreteSelectorBase* selector : allSelectors) { selector->activate(bActive); };
		for (Widget3D_Slider* slider : allSliders) { slider->activate(bActive); };

		layoutSettings();
	}

	void Widget3D_SettingsScreen::renderGameUI(GameUIRenderData& ui_rd)
	{
		//the buttons automatically hook up to be rendered a part of their post construction.
	}

	void Widget3D_SettingsScreen::handleDevConsoleChanged(const size_t& newValue)
	{
		SpaceArcade::get().bEnableDevConsoleFeature = bool(newValue);
	}

	void Widget3D_SettingsScreen::layoutSettings()
	{
		using namespace glm;


		GameUIRenderData ui_rd = GameUIRenderData{}; //performance sin.. this is going to regenerate caches -- but at least it is lazy access so perhaps not too bad.
		const HUDData3D& hd = ui_rd.getHUDData3D();
		vec3 centerPnt = hd.camPos + hd.camFront * hd.frontOffsetDist;
		vec3 position = centerPnt + hd.camUp*(hd.savezoneMax_y*0.75f);
		constexpr float verticalOffset = 1.0f;
		constexpr float defaultSelectorScale = 0.15f;

		Transform xform = {};
		xform.position = position;
		xform.scale = vec3(defaultSelectorScale);
		xform.rotQuat = ui_rd.camQuat();

		float sliderHalfWidth = sliderWidths / 2.f;

		for (Widget3D_ActivatableBase* setting_raw : ordered_options)
		{
			bool bUpdateNextPos = false;

			//okay, this is a dynamic cast sin but I don't want to store position data on the activatable base, as it isn't related to activation. perhaps better is 
			//a base class that has positioning... but that could get messy too. I'd prefer a positioned component that things use.
			if (Widget3D_DiscreteSelectorBase* setting_selector = dynamic_cast<Widget3D_DiscreteSelectorBase*>(setting_raw))
			{
				setting_selector->setWorldXform(xform);
				bUpdateNextPos = true;
			}
			else if (Widget3D_Slider* slider = dynamic_cast<Widget3D_Slider*>(setting_raw))
			{
				slider->setStart(xform.position - sliderHalfWidth * hd.camRight);
				slider->setEnd(xform.position + sliderHalfWidth * hd.camRight);
			}

			//only update if we found a setting that we know the type of
			if (bUpdateNextPos)
			{
				xform.position += -hd.camUp * verticalOffset; //update for NEXT position, not this position
			}

		}
	}

}

