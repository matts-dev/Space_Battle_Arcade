#include "Widget3D_SkirmishScreen.h"
#include "../../../../Levels/LevelConfigs/SpaceLevelConfig.h"
#include "../../../../../Tools/PlatformUtils.h"
#include "../../../../SpaceArcade.h"
#include "../../../../../GameFramework/SALevel.h"
#include "../../../../../GameFramework/SALevelSystem.h"
#include "../../../../Levels/SASpaceLevelBase.h"
#include "../../../../../GameFramework/SAGameEntity.h"
#include "../../../../AssetConfigs/SASpawnConfig.h"
#include "../../../../GameSystems/SAModSystem.h"
#include "../../text/GlitchText.h"
#include "../../../../Levels/BasicTestSpaceLevel.h"

namespace SA
{
	void Widget3D_SkirmishScreen::tick(float dt_sec)
	{
		Parent::tick(dt_sec);

		//for (Widget3D_DiscreteSelectorBase* selector : allSelectors) { selector->tick(dt_sec)};

		errorText->tick(dt_sec);
	}

	void Widget3D_SkirmishScreen::postConstruct()
	{
		Widget3D_MenuScreenBase::postConstruct();
		backButton = new_sp<Widget3D_LaserButton>("Back");
		enabledButtons.push_back(backButton.get());

		startButton = new_sp<Widget3D_LaserButton>("Start");
		enabledButtons.push_back(startButton.get());
		startButton->onClickedDelegate.addWeakObj(sp_this(), &Widget3D_SkirmishScreen::handleStartClicked);

		selector_numPlanets = new_sp<Widget3D_DiscreteSelector<size_t>>();
		allSelectors.push_back(selector_numPlanets.get());
		std::vector<size_t> numPlanetChoices = { 0,1,2,3,4,5 };
		selector_numPlanets->setSelectables(numPlanetChoices);
		selector_numPlanets->setIndex(1);
		selector_numPlanets->setToString([](size_t value) 
		{
			return std::string("Planets: ") + std::to_string(value);
		});


		selector_numStars = new_sp<Widget3D_DiscreteSelector<size_t>>();
		allSelectors.push_back(selector_numStars.get());
		std::vector<size_t> numStarChoices = { 1,2,3,4,5 };
		selector_numStars->setSelectables(numStarChoices);
		selector_numStars->setToString([](size_t value)
		{
			return std::string("Local stars: ") + std::to_string(value);
		});		

		selector_numCarriersPerTeam = new_sp<Widget3D_DiscreteSelector<size_t>>();
		allSelectors.push_back(selector_numCarriersPerTeam.get());
		std::vector<size_t> numCarriersPerTeamChoices = { 1,2,3 };
		selector_numCarriersPerTeam->setSelectables(numCarriersPerTeamChoices);
		selector_numCarriersPerTeam->setToString([](size_t value)
		{
			return std::string("Carriers per team: ") + std::to_string(value);
		});

		selector_numFightersPerCarrier = new_sp<Widget3D_DiscreteSelector<size_t>>();
		allSelectors.push_back(selector_numFightersPerCarrier.get());
		std::vector<size_t> numFightersPerCarrier = { 250, 500, 1000, 2000, 2500 };
		selector_numFightersPerCarrier->setSelectables(numFightersPerCarrier);
		selector_numFightersPerCarrier->setToString([](size_t value)
		{
			return std::string("Max fighters per carrier: ") + std::to_string(value);
		});


		selector_fighterPercSpawnedAtStart = new_sp<Widget3D_DiscreteSelector<size_t>>();
		allSelectors.push_back(selector_fighterPercSpawnedAtStart.get());
		std::vector<size_t> initFighterSpawnPerc = {0, 10, 25, 50, 75, 100};
		selector_fighterPercSpawnedAtStart->setSelectables(initFighterSpawnPerc);
		selector_fighterPercSpawnedAtStart->setIndex(2);
		selector_fighterPercSpawnedAtStart->setToString([](size_t value)
		{
			return std::string("% fighters to spawn at start: ") + std::to_string(value) + "%";
		});

		DigitalClockFont::Data textInit;
		textInit.text = "";
		errorText = new_sp<GlitchTextFont>(textInit);
	}

	void Widget3D_SkirmishScreen::onActivationChanged(bool bActive)
	{
		Parent::onActivationChanged(bActive);

		if (bActive)
		{
			//activated
			configureButtonToDefaultBackPosition(*backButton);
			configureGenericBottomLeftButton(*startButton);

			GameUIRenderData ui_rd;
			configurSelectorPositions(ui_rd);

			for (Widget3D_LaserButton* button : enabledButtons) { button->activate(bActive); } //for stagged anim, stagger in tick.

			errorText->setText(""); //clear any previous error text.
		}
		else
		{
			//deactivated
			for (Widget3D_LaserButton* button : enabledButtons) { button->activate(bActive); } //for stagger anim, update in tick 
		}

		//perhaps just create a generic "activatables" container, but currently enabledButtons uses derived typed information rather than just base activatables
		for (Widget3D_DiscreteSelectorBase* selector : allSelectors) { selector->activate(bActive); };

		//always hide error text, regardless if activated or deactivated -- either signals a reset of this state.
		if (errorText->isPlaying())
		{
			errorText->setAnimPlayForward(false);
		}
	}

	void Widget3D_SkirmishScreen::renderGameUI(GameUIRenderData& ui_rd)
	{
		//the buttons automatically hook up to be rendered a part of their post construction.
		if (const RenderData* gameRD = ui_rd.renderData())
		{
			errorText->render(*gameRD); 
		}
	}

	void Widget3D_SkirmishScreen::configurSelectorPositions(GameUIRenderData& ui_rd)
	{
		using namespace glm;

		const HUDData3D& hd = ui_rd.getHUDData3D();
		vec3 centerPnt = hd.camPos + hd.camFront * hd.frontOffsetDist;
		vec3 position = centerPnt + hd.camUp*(hd.savezoneMax_y*0.75f);
		constexpr float verticalOffset = 1.0f ;
		constexpr float defaultSelectorScale = 0.15f;

		Transform xform = {};
		xform.position = position;
		xform.scale = vec3(defaultSelectorScale);
		xform.rotQuat = ui_rd.camQuat();

		for (Widget3D_DiscreteSelectorBase* selector : allSelectors)
		{
			selector->setWorldXform(xform);
			xform.position += -hd.camUp * verticalOffset; //update for NEXT position, not this position
		}

	}

	void Widget3D_SkirmishScreen::handleStartClicked()
	{
		const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();

		if (!bTransitioningToLevel && activeMod)
		{
			bTransitioningToLevel = true;


			////////////////////////////////////////////////////////
			// configure the level based on user settings
			////////////////////////////////////////////////////////
			sp<SpaceLevelConfig> levelConfig = new_sp<SpaceLevelConfig>();
			levelConfig->setNumPlanets(selector_numPlanets->getValue());
			levelConfig->setNumStars(selector_numStars->getValue());

			//if (carrierTakedownGamemodetag)
			{
				SpaceLevelConfig::GameModeData_CarrierTakedown data;

				size_t numTeams = 2;
				for (size_t teamIdx = 0; teamIdx < numTeams; ++teamIdx)
				{
					SpaceLevelConfig::GameModeData_CarrierTakedown::TeamData& td = data.teams.emplace_back();
					
					size_t numCarriersPerTeam = selector_numCarriersPerTeam->getValue();
					for (size_t carrierIdx = 0; carrierIdx < numCarriersPerTeam; ++carrierIdx)
					{
						CarrierSpawnData& carrierData = td.carrierSpawnData.emplace_back();

						float percToSpawn = float(selector_fighterPercSpawnedAtStart->getValue()) / 100.f;
						size_t maxFighters = selector_numFightersPerCarrier->getValue();
						carrierData.numInitialFighters = size_t(maxFighters*percToSpawn);
						carrierData.fighterSpawnData.maxNumberOwnedFighterShips = maxFighters;

						sp<SpawnConfig> carrierConfig = activeMod->getDeafultCarrierConfigForTeam(teamIdx);
						carrierData.carrierShipSpawnConfig_name = carrierConfig->getName();

						if (!carrierConfig)
						{
							setError("Cannot find default carrier config in mod for team " + std::to_string(teamIdx));
							return;
						}
					}
				}
				
				levelConfig->setGamemodeData_CarrierTakedown(data);
			}


			cachedLevelConfig = levelConfig;

			cachedLevelConfig->applyDemoDataIfEmpty();
			
			////////////////////////////////////////////////////////
			// start level load / animation
			////////////////////////////////////////////////////////

			if (const sp<LevelBase>& currentLevel = SpaceArcade::get().getLevelSystem().getCurrentLevel())
			{
				if (!levelAnimTransitionTimerHandle)
				{
					levelAnimTransitionTimerHandle = new_sp<MultiDelegate<>>();
					levelAnimTransitionTimerHandle->addWeakObj(sp_this(), &Widget3D_SkirmishScreen::handleLevelTransitionAnimationOver);
				}
				currentLevel->getWorldTimeManager()->createTimer(levelAnimTransitionTimerHandle, transitionToLevelAnimationDurationSec);

				SpaceLevelBase::staticEnableStarJump(true);

				backButton->activate(false);
				startButton->activate(false);

				////////////////////////////////////////////////////////
				//deactivate all ui
				////////////////////////////////////////////////////////
				for (Widget3D_LaserButton* button : enabledButtons) { button->activate(false); }
				for (Widget3D_DiscreteSelectorBase* selector : allSelectors) { selector->activate(false); };
			}
			else { STOP_DEBUGGER_HERE(); }
		}

	}

	void Widget3D_SkirmishScreen::handleLevelTransitionAnimationOver()
	{
		if (cachedLevelConfig)
		{
			LevelSystem& levelSystem = GameBase::get().getLevelSystem();

			sp<SpaceLevelBase> newLevel = bShouldUseDebugLevel ? new_sp<BasicTestSpaceLevel>() : new_sp<SpaceLevelBase>();
			newLevel->setConfig(cachedLevelConfig);
			cachedLevelConfig = nullptr;

			sp<LevelBase> baseLevelPtr = newLevel;
			levelSystem.loadLevel(baseLevelPtr);
		}
		else { STOP_DEBUGGER_HERE(); }
	}

	void Widget3D_SkirmishScreen::setError(const std::string& errorStr)
	{
		for (Widget3D_DiscreteSelectorBase* selector : allSelectors) { selector->activate(false); };
		for (Widget3D_LaserButton* button : enabledButtons) { button->activate(false); }

		//let back button persist
		backButton->activate(true);

		GameUIRenderData ui_rd = {};
		const HUDData3D& hud = ui_rd.getHUDData3D();
		Transform errorXform;
		errorXform.position = hud.camPos + hud.camFront*hud.frontOffsetDist; 
		errorXform.rotQuat = ui_rd.camQuat();
		errorXform.scale = glm::vec3(0.1f);
		errorText->setXform(errorXform);

		errorText->setText(errorStr);

		errorText->setAnimPlayForward(true);
		errorText->play(true);
	}

	//void Widget3D_SkirmishScreen::tryShowNewSelector(float dt_sec)
	//{
	//	if (!animInButtonIdx.has_value() || animInButtonIdx != enabledButtons.size())
	//	{
	//		float buttonAnimFrac = animInTime / (!bSeenOnce ? showAllButtonsAnimDurSec : showAllButtonsAnimDurSec / 2.f);
	//		float curvedButtonAnimFrac = buttonAnimFrac;// sigmoid.eval_smooth(buttonAnimFrac);

	//		//calculate next button index
	//		size_t newButtonIdx = size_t(enabledButtons.size() * curvedButtonAnimFrac);
	//		if (newButtonIdx == enabledButtons.size()) { bSeenOnce = true; } //check before clamped
	//		newButtonIdx = glm::clamp<size_t>(newButtonIdx, 0, enabledButtons.size() - 1);

	//		if (!animInButtonIdx.has_value() || newButtonIdx > *animInButtonIdx) //only do this once we cross into a new button index
	//		{
	//			animInButtonIdx = newButtonIdx;

	//			GameUIRenderData rd_ui = {};
	//			const HUDData3D& hd = rd_ui.getHUDData3D();

	//			if (Widget3D_LaserButton* button = enabledButtons[*animInButtonIdx])
	//			{
	//				vec3 position = vec3(0.f);
	//				if (*animInButtonIdx == 0)
	//				{
	//					vec3 centerPnt = hd.camPos + hd.camFront * hd.frontOffsetDist;
	//					vec3 topPoint = centerPnt + hd.camUp*(hd.savezoneMax_y*0.75f);
	//					position = topPoint;
	//				}
	//				else if (*animInButtonIdx - 1 < enabledButtons.size())
	//				{
	//					if (Widget3D_LaserButton* previousButton = enabledButtons[*animInButtonIdx - 1])
	//					{
	//						position = previousButton->getXform().position;
	//						position += hd.camUp * -1.f * (buttonSpacing + previousButton->getSize().y);

	//					}
	//					else { STOP_DEBUGGER_HERE(); } //should always have a previous button
	//				}

	//				//align buttons vertically, start from top and push down based on width
	//				Transform buttonXform = {};
	//				buttonXform.position = position;
	//				buttonXform.scale = vec3(0.25f);
	//				buttonXform.rotQuat = rd_ui.camQuat();
	//				button->setXform(buttonXform);
	//				button->activate(true);
	//			}
	//			else
	//			{
	//				log(__FUNCTION__, LogLevel::LOG_WARNING, "Empty button in button list? this shouldn't happen!");
	//				STOP_DEBUGGER_HERE();
	//			}

	//			//increment button index after it has been used
	//			//++animInButtonIdx; //this is now curve driven
	//			//animInTime = 0.f;
	//		}
	//	}
	//	else if (animInButtonIdx == enabledButtons.size())
	//	{
	//	}
	//}

}

