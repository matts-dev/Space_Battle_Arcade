#include "Widget3D_SettingScreen.h"
#include "../../../../SpaceArcade.h"
#include "../Widget3D_DiscreteSelector.h"
#include "../../../../GameSystems/SAUISystem_Game.h"
#include "../Widget3D_Slider.h"
#include "../../../../AssetConfigs/SASettingsProfileConfig.h"
#include "../../../../SAPlayer.h"
#include "../../../../../GameFramework/SAPlayerBase.h"
#include "../../../../../GameFramework/SAPlayerSystem.h"
#include "../../../../../GameFramework/SAGameBase.h"
#include "../../../../GameSystems/SAModSystem.h"
#include "../../../../../GameFramework/SAAudioSystem.h"

namespace SA
{

	void Widget3D_SettingsScreen::tick(float dt_sec)
	{
		Parent::tick(dt_sec);
	}

	void Widget3D_SettingsScreen::postConstruct()
	{
		//get the settings profile
		if (!activeSettingsProfile)
		{
			if (SAPlayer* player = dynamic_cast<SAPlayer*>(GameBase::get().getPlayerSystem().getPlayer(0).get()))
			{
				activeSettingsProfile = player->getSettingsProfile();
			}
		}

		backButton = new_sp<Widget3D_LaserButton>("Back");
		enabledButtons.push_back(backButton.get());

		applyButton = new_sp<Widget3D_LaserButton>("Apply");
		enabledButtons.push_back(applyButton.get());
		applyButton->onClickedDelegate.addWeakObj(sp_this(), &Widget3D_SettingsScreen::applySettings);

		////////////////////////////////////////////////////////
		// set up dev console selector
		////////////////////////////////////////////////////////
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

		////////////////////////////////////////////////////////
		// set up team selector
		////////////////////////////////////////////////////////
		selector_team = new_sp<Widget3D_DiscreteSelector<size_t>>();
		allSelectors.push_back(selector_team.get());
		selector_team->setSelectables({ 0,1 });
		size_t saveTeamIndex = activeSettingsProfile && activeSettingsProfile->selectedTeamIdx == 0 ? 0 : 1;//user ternary to clamp
		selector_team->setIndex(saveTeamIndex); 
		selector_team->setToString([](size_t value) {
			const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
			return std::string("Selected Team: ") + (activeMod->teamHasName(value) ? activeMod->getTeamName(value) : std::to_string(value));
		});

		////////////////////////////////////////////////////////
		// set up audio slider
		////////////////////////////////////////////////////////
		slider_masterAudio = new_sp<Widget3D_Slider>();
		allSliders.push_back(slider_masterAudio.get());
		slider_masterAudio->setTitleText("Master Volume");
		slider_masterAudio->setTitleTextScale(0.125f);
		slider_masterAudio->setValue(activeSettingsProfile ? activeSettingsProfile->masterVolume / AudioSystem::getSystemAudioMaxMultiplier() : 0.5f); //convert setting to a [0,1] range

		ScalabilitySettings defaultScalabilitySettings;
		slider_perfRespawnCooldownMultiplier = new_sp<Widget3D_Slider>();
		slider_perfRespawnCooldownMultiplier->setTitleText("Spawn Rate");
		slider_perfRespawnCooldownMultiplier->setTitleTextScale(0.125f);
		float respawnCooldownMult_inverted = 1.f - defaultScalabilitySettings.multiplier_spawnComponentCooldownSec;
		if (activeSettingsProfile)
		{
			respawnCooldownMult_inverted = 1.f - activeSettingsProfile->scalabilitySettings.multiplier_spawnComponentCooldownSec;
			respawnCooldownMult_inverted /= 0.9f;//convert to [0,1] since we're clamping at 0.1f.
		}
		slider_perfRespawnCooldownMultiplier->setValue(respawnCooldownMult_inverted);
		allSliders.push_back(slider_perfRespawnCooldownMultiplier.get());

		slider_perfRespawnMaxCount = new_sp<Widget3D_Slider>();
		slider_perfRespawnMaxCount->setTitleText("Max Spawns");
		slider_perfRespawnMaxCount->setTitleTextScale(0.125f);
		slider_perfRespawnMaxCount->setValue(activeSettingsProfile ? activeSettingsProfile->scalabilitySettings.multiplier_maxSpawnableShips : defaultScalabilitySettings.multiplier_maxSpawnableShips);
		allSliders.push_back(slider_perfRespawnMaxCount.get());

		//defines the order of selectors/sliders
		ordered_options.push_back(selector_team.get());
		ordered_options.push_back(selector_devConsole.get());
		ordered_options.push_back(slider_masterAudio.get());
		ordered_options.push_back(slider_perfRespawnCooldownMultiplier.get());
		ordered_options.push_back(slider_perfRespawnMaxCount.get());
	}

	void Widget3D_SettingsScreen::onActivationChanged(bool bActive)
	{
		Parent::onActivationChanged(bActive);
		if (bActive)
		{
			//activated
			configureButtonToDefaultBackPosition(*backButton);
			configureGenericBottomLeftButton(*applyButton);
			readPlayerSettings();
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
				bUpdateNextPos = true;
			}

			//only update if we found a setting that we know the type of
			if (bUpdateNextPos)
			{
				xform.position += -hd.camUp * verticalOffset; //update for NEXT position, not this position
			}
		}
	}

	void Widget3D_SettingsScreen::applySettings()
	{
		if (activeSettingsProfile)
		{
			//read all widget values
			activeSettingsProfile->bEnableDevConsole = bool(selector_devConsole->getValue());
			activeSettingsProfile->masterVolume = slider_masterAudio->getValue() * AudioSystem::getSystemAudioMaxMultiplier();
			activeSettingsProfile->selectedTeamIdx = selector_team->getValue();
			activeSettingsProfile->scalabilitySettings.multiplier_maxSpawnableShips = glm::clamp<float>(slider_perfRespawnMaxCount->getValue(), 0.05f, 1.0f);
			float cooldownMult = 1.f - 0.9f*(slider_perfRespawnCooldownMultiplier->getValue()); //invert so higher feels like turning up settings, also convert back to [0.1,1j range
			activeSettingsProfile->scalabilitySettings.multiplier_spawnComponentCooldownSec = glm::clamp<float>(cooldownMult, 0.1f, 1.f); //don't respawn faster than 0.1 the carrier setting in level

			//save
			activeSettingsProfile->requestSave();

			if (const sp<PlayerBase>& playerBase = GameBase::get().getPlayerSystem().getPlayer(selectedPlayer))
			{
				if (SAPlayer* player = dynamic_cast<SAPlayer*>(playerBase.get()))
				{
					player->setSettingsProfile(activeSettingsProfile);
				}
			}
			
			AudioSystem& audioSystem = GameBase::get().getAudioSystem();
			audioSystem.setSystemVolumeMultiplier(slider_masterAudio->getValue() * AudioSystem::getSystemAudioMaxMultiplier()); //transform form from [0,1] to [0,max]
		}
	}

	void Widget3D_SettingsScreen::readPlayerSettings()
	{
		if (const sp<PlayerBase>& playerBase = GameBase::get().getPlayerSystem().getPlayer(selectedPlayer))
		{
			if (SAPlayer* player = dynamic_cast<SAPlayer*>(playerBase.get()))
			{
				activeSettingsProfile = player->getSettingsProfile();
			}
		}

		if (activeSettingsProfile)
		{
			selector_devConsole->setIndex(size_t(activeSettingsProfile->bEnableDevConsole)); 
			
			slider_masterAudio->setValue(activeSettingsProfile->masterVolume / AudioSystem::getSystemAudioMaxMultiplier()); //convert to [0,1]

			//#TODO need to refactor this so we're not doing code duplication in set up....
			ScalabilitySettings defaultScalabilitySettings;
			float respawnCooldownMult_inverted = defaultScalabilitySettings.multiplier_spawnComponentCooldownSec;
			if (activeSettingsProfile)
			{
				respawnCooldownMult_inverted = 1.f - activeSettingsProfile->scalabilitySettings.multiplier_spawnComponentCooldownSec;
				respawnCooldownMult_inverted /= 0.9f;//convert to [0,1] since we're clamping at 0.1f.
			}
			slider_perfRespawnCooldownMultiplier->setValue(respawnCooldownMult_inverted);

			slider_perfRespawnMaxCount->setValue(activeSettingsProfile ? activeSettingsProfile->scalabilitySettings.multiplier_maxSpawnableShips : defaultScalabilitySettings.multiplier_maxSpawnableShips);

			selector_team->setIndex(activeSettingsProfile->selectedTeamIdx);
		}
	}

}

