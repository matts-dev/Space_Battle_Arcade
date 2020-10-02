
#include "Widget3D_CampaignScreen.h"
#include "../../../../GameSystems/SAModSystem.h"
#include "../../../../SpaceArcade.h"
#include "../../../../AssetConfigs/CampaignConfig.h"
#include "../../../../GameSystems/SAUISystem_Game.h"
#include "../../../../../GameFramework/SADebugRenderSystem.h"
#include "../../../../../Tools/color_utils.h"
#include "../../LaserUIPool.h"
#include "../../../../../GameFramework/SALog.h"
#include "../../../../Environment/Planet.h"
#include <vector>
#include "../../../../../Rendering/RenderData.h"
#include "../Widget3D_GenericMouseHoverable.h"
#include "../../../../../Tools/PlatformUtils.h"
#include "../../../../../Tools/DataStructures/MultiDelegate.h"
#include "../../../../../GameFramework/SALevelSystem.h"
#include "../../../../../GameFramework/SAGameBase.h"
#include "../../../../../GameFramework/SALevel.h"
#include "../../../../../GameFramework/SATimeManagementSystem.h"
#include "../../../../Levels/SASpaceLevelBase.h"
#include "../../../../../Tools/SAUtilities.h"
#include "../../../../Levels/LevelConfigs/SpaceLevelConfig.h"
#include "../../../../Levels/BasicTestSpaceLevel.h"
#include "../../../../AssetConfigs/SaveGameConfig.h"

namespace SA
{
	using namespace glm;

	static void configurePlanetForUI(Planet& planet)
	{
		Transform transform = planet.getTransform();
		//transform.scale *= glm::vec3(0.2f);
		transform.scale = glm::vec3(0.2f);
		planet.setTransform(transform);
		planet.setUseCameraAsLight(true);
		planet.setForceCentered(false);
	}


	Widget3D_CampaignScreen::~Widget3D_CampaignScreen()
	{
		//release all lasers
		if (isActive())
		{
			onActivationChanged(false);
		}
	};

	void Widget3D_CampaignScreen::tick(float dt_sec)
	{
		Parent::tick(dt_sec);

		if (!bTierAnimationComplete)
		{
			bTierAnimationComplete = true; //assume that we just completed animation until we find evidence that it isn't complete (below)

			for (size_t tierIdx = 0; tierIdx < tiers.size(); ++tierIdx)
			{
				TierData& tier = tiers[tierIdx];
				if (!tier.bAnimStarted)
				{
					if (isActive())
					{
						if (bool bReadyToStart = accumulatedTime > (tierIdx * tierAnimDelaySec + timestamp_activation))
						{
							tier.bAnimStarted = true;
							startTierAnimation(tier);
						}
						else
						{
							bTierAnimationComplete = false;
							break;
						}
					}
				}
			}
		}

		if (isActive() || withinTickAfterDeactivationWindow())
		{
			size_t tierNum = 0;
			for (TierData& tier : tiers)
			{
				for (size_t levelIdx : tier.levelIndices)
				{
					if (levelIdx < linearLevels.size() && linearLevels.size() != 0)
					{
						SelectableLevelData& level = linearLevels[levelIdx];
						if (level.uiPlanet)
						{
							float planetScaleFactor = 0.f;
							bool bShouldScaleDownForLevelLoading = (bLoadingLevel && !(/*is selected planet*/level.hoverWidget && level.hoverWidget->isToggled())); //all hover widgets are active while screen is up
							if (isActive() && !bShouldScaleDownForLevelLoading)
							{
								if (tier.bAnimStarted)
								{
									float animTime = accumulatedTime - (timestamp_activation + (tierNum*tierAnimDelaySec));
									planetScaleFactor = glm::clamp(animTime / tierAnimDelaySec, 0.f, 1.f);
								}
							}
							else //deactivating
							{
								planetScaleFactor = 1 - clamp((accumulatedTime - timestamp_deactivated) / tierAnimDelaySec, 0.f, 1.f);
							}

							configurePlanetForUI(*level.uiPlanet); //reset planet to UI size
							Transform transform = level.uiPlanet->getTransform();
							transform.scale *= vec3(planetScaleFactor * level.uiPlanetScale);
							level.uiPlanet->setTransform(transform); //not great for perf to run through transform code over and over like this, but this is UI land 

							if (level.hoverWidget)
							{
								Transform hoverXform = level.hoverWidget->getXform();
								hoverXform.scale = transform.scale * hoverCollisionScaleup;
								level.hoverWidget->setXform(hoverXform);
							}

							//have planets update
							if (playableLevelSet.find(levelIdx) != playableLevelSet.end()) //only tick planets that are unlocked, so ones you can play spin but not locked planets
							{
								level.uiPlanet->tick(dt_sec);
							}
						}
					}
				}
				++tierNum;
			}
		}

		accumulatedTime += dt_sec;
	}

	void Widget3D_CampaignScreen::postConstruct()
	{
		Widget3D_MenuScreenBase::postConstruct();
		backButton = new_sp<Widget3D_LaserButton>("Back");
		enabledButtons.push_back(backButton.get());

		startButton = new_sp<Widget3D_LaserButton>("start");
		startButton->activate(false); //only activate once we have a planet selected
		enabledButtons.push_back(startButton.get());

		startButton->onClickedDelegate.addWeakObj(sp_this(), &Widget3D_CampaignScreen::handleStartClicked);

		SpaceArcade::get().getModSystem()->onActiveModChanging.addWeakObj(sp_this(), &Widget3D_CampaignScreen::handleActiveModChanging);

	}

	void Widget3D_CampaignScreen::onActivationChanged(bool bActive)
	{ 
		Parent::onActivationChanged(bActive);
		if (bActive)
		{
			timestamp_activation = accumulatedTime;
			bTierAnimationComplete = false;

			//activated
			generateSelectableLevelData();

			//prevent a flicker of all planets by scaling to zero, animation will take cave of undoing this
			for (TierData& tier : tiers)
			{
				for (size_t levelIdx : tier.levelIndices)
				{
					if (levelIdx < linearLevels.size() && linearLevels.size() != 0)
					{
						SelectableLevelData& level = linearLevels[levelIdx];
						if (level.uiPlanet)
						{
							Transform transform = level.uiPlanet->getTransform();
							transform.scale *= vec3(0.f);
							level.uiPlanet->setTransform(transform); 
						}
						if (level.hoverWidget)
						{
							level.hoverWidget->activate(playableLevelSet.find(levelIdx) != playableLevelSet.end());
						}
					}
				}
			}

			configureButtonToDefaultBackPosition(*backButton);
			configureGenericBottomLeftButton(*startButton);
			for (Widget3D_LaserButton* button : enabledButtons) 
			{ 
				//for stagged anim, stagger in tick.
				if (button != startButton.get() || bHasPlanetSelected) //if we've selected a planet, then activate start button
				{
					button->activate(bActive); 
				}
			} 
		}
		else
		{
			//deactivated
			timestamp_deactivated = accumulatedTime;

			for (Widget3D_LaserButton* button : enabledButtons) { button->activate(bActive); }
			for (TierData& tier : tiers) 
			{ 
				tier.bAnimStarted = false; 

				for (size_t levelIdx : tier.levelIndices)
				{
					if (levelIdx < linearLevels.size() && linearLevels.size() != 0)
					{
						SelectableLevelData& level = linearLevels[levelIdx];
						if (level.hoverWidget)
						{
							level.hoverWidget->activate(false);
						}
					}
				}

				LaserUIPool& laserPool = LaserUIPool::get();
				for (sp<LaserUIObject>& laser : tier.lasers)
				{
					laserPool.releaseLaser(laser); //will set it to null within the array
				}
				tier.lasers.clear(); //clear out the nullptrs
			}
		}
	}

	void Widget3D_CampaignScreen::renderGameUI(GameUIRenderData& ui_rd)
	{
		using namespace glm;

		//the buttons automatically hook up to be rendered a part of their post construction.

		bool bWithinTickAfterDeactivationWindow = withinTickAfterDeactivationWindow();

		const RenderData* game_rd = ui_rd.renderData();
		if ( bool bShouldRender = (isActive() || bWithinTickAfterDeactivationWindow) && game_rd)
		{
			for (TierData& tier : tiers)
			{
				for (size_t levelIdx : tier.levelIndices)
				{
					if (levelIdx < linearLevels.size() && linearLevels.size() != 0)
					{
						SelectableLevelData& level = linearLevels[levelIdx];
						if (level.uiPlanet)
						{
							bool bUnlocked = playableLevelSet.find(levelIdx) != playableLevelSet.end();
							level.uiPlanet->setUseGrayScale(!bUnlocked);
							//level.uiPlanet->setUseCameraAsLight(bUnlocked); //testing whether this helps distinguish between unlocked/unlocked
							level.uiPlanet->render(game_rd->dt_sec, game_rd->view, game_rd->projection);
						}
					}
				}
			}
		}

		if constexpr (constexpr bool bDebug = false)
		{
			DebugRenderSystem& db = GameBase::get().getDebugRenderSystem();
			Transform debugTransform;
			debugTransform.scale = vec3(0.1f);
			for (TierData& tier : tiers)
			{
				for (size_t lvlIdx : tier.levelIndices)
				{
					debugTransform.position = linearLevels[lvlIdx].worldPos;
					db.renderCube(debugTransform.getModelMatrix(), color::brightGreen());
				}
			}
		}
	}

	bool Widget3D_CampaignScreen::withinTickAfterDeactivationWindow()
	{
		return accumulatedTime - timestamp_deactivated < 3.0f;
	}

	void Widget3D_CampaignScreen::getConfigData()
	{
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			activeCampaign = activeMod->getCampaign(activeCampaignIdx);
			assert(activeCampaign); //only should be null if we're passing an invalid index. Currently there is only 1 but in future may support multiple

			saveGameData = activeMod->getSaveGameConfig();

			//DEBUG -- force a save to generate a template
			//activeCampaign->requestSave();
		}
	}

	struct DefaultPlanetTextureInfo
	{
		size_t clampedIndex;
		size_t numTextures;
		std::string texturePath;

	};

	DefaultPlanetTextureInfo getDefaultPlanetTexturePathForIndex(size_t defaultIndex)
	{
		const std::string defaultTextures[] = {
			std::string(DefaultPlanetTexturesPaths::albedo1),
			std::string(DefaultPlanetTexturesPaths::albedo2),
			std::string(DefaultPlanetTexturesPaths::albedo3),
			std::string(DefaultPlanetTexturesPaths::albedo4),
			std::string(DefaultPlanetTexturesPaths::albedo5),
			std::string(DefaultPlanetTexturesPaths::albedo6),
			std::string(DefaultPlanetTexturesPaths::albedo7),
			std::string(DefaultPlanetTexturesPaths::albedo8)
		};
		const size_t numDefaultPlanets = sizeof(defaultTextures) / sizeof(std::string);

		DefaultPlanetTextureInfo texInfo;
		texInfo.clampedIndex = glm::clamp<size_t>(defaultIndex, 0, numDefaultPlanets);
		texInfo.numTextures = numDefaultPlanets;
		texInfo.texturePath = defaultTextures[texInfo.clampedIndex];
		return texInfo;
	}


	sp<Planet> makePlanetFromDefaults(int64_t defaultIdx, size_t idxSeed)
	{

		DefaultPlanetTextureInfo textureInfo = getDefaultPlanetTexturePathForIndex(size_t(defaultIdx));
		size_t numDefaultPlanets = textureInfo.numTextures;

		size_t planetIdx = size_t(defaultIdx);
		if (defaultIdx >= 0 && planetIdx < numDefaultPlanets)
		{
			Planet::Data init;
			init.albedo1_filepath = textureInfo.texturePath;

			//use the idx as a seed to vary parameters on the planet for UI.

			//choose a rotation axis based on planet index, very adhoc
			init.rotationAxis = glm::vec3(
				float((idxSeed + 1) * 30 % 41) * (idxSeed % 2 == 0)? 1.f : -1.f,
				float((idxSeed + 1) * 30 % 41) * (idxSeed % 3 == 0)? 1.f : -1.f,
				float((idxSeed + 1) * 30 % 41) * (idxSeed % 5 == 0)? 1.f : -1.f
			);
			if (glm::length2(init.rotationAxis) < 0.01f)
			{
				init.rotationAxis = vec3(1.f, 0.f, 0.f);
			}
			init.rotationAxis = glm::normalize(init.rotationAxis);

			//chose a random rotation speed (also very adhoc based on level idx for determinism)
			init.rotSpeedSec_rad = glm::radians<float>((idxSeed % 5) / 5.f * 20.f); //some factor of 45 degrees per second
			if (init.rotSpeedSec_rad < 0.01f)
			{
				init.rotSpeedSec_rad = glm::radians<float>(5.f);
			}

			sp<Planet> newPlanet = new_sp<Planet>(init);
			configurePlanetForUI(*newPlanet);
			return newPlanet;
		}
		else
		{
			return nullptr;
		}
	}

	void Widget3D_CampaignScreen::generateSelectableLevelData()
	{
		using namespace glm;

		if (!activeCampaign || !saveGameData)
		{
			getConfigData();
		}

		if (!activeCampaign || !saveGameData) { STOP_DEBUGGER_HERE(); } //missing some config data

		if (activeCampaign)
		{
			completedSet.clear();
			if (saveGameData)
			{
				std::string campaignFilePath = activeCampaign->getRepresentativeFilePath();
				if (const SaveGameConfig::CampaignData* savedData = saveGameData->findCampaignByName(campaignFilePath))
				{
					for(size_t completedLevel : savedData->completedLevels)
					{
						completedSet.insert(completedLevel);
					}
				}
				else
				{
					//create a new entry in our save data for this campaign
					saveGameData->addCampaign(campaignFilePath); //only save when update happens
				}
			}

			if ( bool bLevelsNeedLoad = (tiers.size() == 0 && linearLevels.size() == 0) )
			{
				bHasPlanetSelected = false; //reset this if we're regenerating planet data
				startButton->activate(false); //make sure the start button is not activated

				const std::vector<CampaignConfig::LevelData>& configLevelData = activeCampaign->getLevels();
				for (size_t levelIdx = 0; levelIdx < configLevelData.size(); ++levelIdx)
				{
					const CampaignConfig::LevelData& level  = configLevelData[levelIdx];

					if (level.tier >= MaxLevelTiers)
					{
						continue; //skip levels that are outside of the tier range
					}

					////////////////////////////////////////////////////////
					//copy relevant data from config
					////////////////////////////////////////////////////////
					linearLevels.push_back(SelectableLevelData{});
					SelectableLevelData& thisLevel = linearLevels.back();
					thisLevel.levelIndexNumberInCampaignConfig = levelIdx;
					thisLevel.outGoingLevelPathIndices = level.outGoingPathIndices;
					thisLevel.uiPlanet = makePlanetFromDefaults(level.optional_defaultPlanetIdx, levelIdx);
					thisLevel.uiPlanetScale = glm::clamp(level.optional_ui_planetSizeFactor, 0.1f, 10.f);
					thisLevel.hoverWidget = new_sp<Widget3D_GenericMouseHoverable>();
					if (thisLevel.hoverWidget)
					{
						thisLevel.hoverWidget->onClicked.addWeakObj(sp_this(), &Widget3D_CampaignScreen::handlePlanetClicked);
					}
					if (thisLevel.uiPlanet)
					{
						Transform transform = thisLevel.uiPlanet->getTransform();
						//transform.scale *= clamp(level.optional_ui_planetSizeFactor, 0.1f, 10.f);
						transform.scale *= vec3(thisLevel.uiPlanetScale);
						thisLevel.uiPlanet->setTransform(transform);
					}

					////////////////////////////////////////////////////////
					// add to stack of levels in tier
					////////////////////////////////////////////////////////
					while (tiers.size() < (level.tier + 1) && tiers.size() < MaxLevelTiers)
					{
						tiers.push_back(TierData{});
					}

					TierData& thisLevelTier = tiers[level.tier];
					thisLevelTier.levelIndices.push_back(linearLevels.size() - 1); //this index will be used later as a lookup into linear levels to adjust position etc.
				}
			}

			buildPlayableSet(completedSet);

			////////////////////////////////////////////////////////
			// position tiers now that final numbers are available
			////////////////////////////////////////////////////////
			GameUIRenderData rd_ui{};
			const HUDData3D& hud = rd_ui.getHUDData3D();

			const vec3 tierBasePoint_Start = hud.frontOffsetDist*hud.camFront + -hud.camRight*hud.savezoneMax_x * 0.75f + hud.camUp*hud.savezoneMax_y * 0.95f;
			const vec3 tierBasePoint_End = hud.frontOffsetDist*hud.camFront + hud.camRight*hud.savezoneMax_x * 0.75f + hud.camUp*hud.savezoneMax_y * 0.95f;
			float horizontalLength = glm::length(tierBasePoint_End - tierBasePoint_Start);
			float tierHorizontalOffsetDist = horizontalLength / (tiers.size() - 1); //-1 to put last tier at end of screen, rather than 1 index early
			
			//provide some additional variation between tiers so they do not vertically align at certain points
			size_t wrapAroundValue = 0;
			size_t wrap = 3;

			for (size_t tierIdx = 0; tierIdx < tiers.size(); ++tierIdx)
			{
				const TierData& tier = tiers[tierIdx];

				float additionalSpacingVactor = glm::clamp<float>( ((float(wrapAroundValue)+1) / wrap), 0.f, 1.f);  //this is a little adhoc, don't read too much into it.
				wrapAroundValue = (wrapAroundValue + 1) % wrap;

				size_t numLevelsInThisTier = tier.levelIndices.size();
				const float tierVerticalSize = (hud.savezoneMax_y* (0.5f + (0.12f*additionalSpacingVactor)))*2.f;
				const float verticalSpacing = tierVerticalSize / float(numLevelsInThisTier);
				for (size_t tierLevelIdx = 0; tierLevelIdx < tier.levelIndices.size(); tierLevelIdx++)
				{
					size_t linearIdx = tier.levelIndices[tierLevelIdx]; //look up the level in the 1d array, which can have data in any tier order (hence why we're doing this)
					SelectableLevelData& uiLevelData = linearLevels[linearIdx];

					float vertOffset = (tier.levelIndices.size() != 1 )?
						verticalSpacing * (1 + tierLevelIdx) //space even first level by some amount (so there isn't a vertical bar at top)
						: verticalSpacing / 2.f;	//if we only have 1 level in tier, put it in the middle

					uiLevelData.worldPos = tierBasePoint_Start + (vertOffset*-hud.camUp) + (tierIdx*tierHorizontalOffsetDist*hud.camRight);

					//now that position is calculated, use it to set planet
					if (uiLevelData.uiPlanet)
					{
						Transform transform = uiLevelData.uiPlanet->getTransform();
						transform.position = uiLevelData.worldPos;
						uiLevelData.uiPlanet->setTransform(transform);

						if (uiLevelData.hoverWidget)
						{
							transform.scale *= hoverCollisionScaleup;
							transform.rotQuat = rd_ui.camQuat();
							uiLevelData.hoverWidget->radius = uiLevelData.uiPlanetScale * 0.3f;
							uiLevelData.hoverWidget->bClickToggles = true;
							uiLevelData.hoverWidget->setXform(transform);
						}
					}
				}
			}
		}
	}

	void Widget3D_CampaignScreen::startTierAnimation(TierData& tier)
	{
		LaserUIPool& laserPool = LaserUIPool::get();

		for(size_t levelidx : tier.levelIndices)
		{
			SelectableLevelData levelData = linearLevels[levelidx];
			vec3 startPos = levelData.worldPos;

			for (size_t outIdx : levelData.outGoingLevelPathIndices)
			{
				vec3 endPos = linearLevels[outIdx].worldPos;

				sp<LaserUIObject> laser = laserPool.requestLaserObject();
				laser->animateStartTo(startPos);
				laser->animateEndTo(endPos);
				laser->randomizeAnimSpeed(tierAnimDelaySec); //use this anim delay sec so that laser will align right as we start animating another tier
				laser->scaleAnimSpeeds(0.75f, 1.f); //make start faster than end

				bool bBothLaserEndsValid = playableLevelSet.find(outIdx) != playableLevelSet.end() && completedSet.find(levelidx) != completedSet.end();
				laser->setColorImmediate(bBothLaserEndsValid ? LaserUIPool::defaultColor : glm::vec3(0.25f));

				tier.lasers.push_back(laser);
			}
		}
	}

	void Widget3D_CampaignScreen::buildPlayableSet(const std::set<size_t>& completedLevelSet)
	{
		//uses completedLevelSet parameter to make sure caller knows for fact that completedLevelSet needs to be populated before calling this function
		if (linearLevels.size() == 0)
		{
			STOP_DEBUGGER_HERE(); //were linear levels not built yet?
		}

		playableLevelSet.clear();

		LaserUIPool& laserPool = LaserUIPool::get();
		for (const TierData& tier : tiers)
		{
			for (size_t levelidx : tier.levelIndices)
			{
				SelectableLevelData levelData = linearLevels[levelidx];

				bool bOutgoingConnectionsUnlocked = completedLevelSet.find(levelidx) != completedLevelSet.end();
				for (size_t outIdx : levelData.outGoingLevelPathIndices)
				{
					if (bOutgoingConnectionsUnlocked)
					{
						playableLevelSet.insert(outIdx); //we can play these levels
					}
				}
			}
		}

		//always let first column be playable regardless if unlocks above happened
		if (TierData* firstTier = (tiers.size() > 0) ? &tiers[0] : nullptr)
		{
			for (size_t levelIdx : firstTier->levelIndices) { playableLevelSet.insert(levelIdx); } //add first row as always playable
		}

	}

	std::optional<size_t> Widget3D_CampaignScreen::findSelectedPlanetIdx()
	{
		for (TierData& tier : tiers)
		{
			for (size_t levelIdx : tier.levelIndices)
			{
				if (levelIdx < linearLevels.size() && linearLevels.size() != 0)
				{
					SelectableLevelData& level = linearLevels[levelIdx];
					if (level.hoverWidget)
					{
						if (level.hoverWidget->isToggled())
						{
							return levelIdx;
						}
					}
				}
			}
		}

		return std::nullopt;
	}

	void Widget3D_CampaignScreen::handleActiveModChanging(const sp<Mod>& previous, const sp<Mod>& active)
	{
		log(__FUNCTION__, LogLevel::LOG, "Active mod changing, clearing cached level data used for UI");

		//clear all data and have it repopulate when going to campaign
		activeCampaign = nullptr;
		saveGameData = nullptr;
		linearLevels.clear();
		tiers.clear();
	}

	void Widget3D_CampaignScreen::handlePlanetClicked()
	{
		for (TierData& tier : tiers)
		{
			for (size_t levelIdx : tier.levelIndices)
			{
				if (levelIdx < linearLevels.size() && linearLevels.size() != 0)
				{
					SelectableLevelData& level = linearLevels[levelIdx];
					if (level.hoverWidget && playableLevelSet.find(levelIdx) != playableLevelSet.end())
					{
						level.hoverWidget->setToggled(false);
					}
				}
			}
		}

		if (!bHasPlanetSelected)
		{
			startButton->activate(true);
		}

		bHasPlanetSelected = true; //planet will select itself after this event
	}

	void Widget3D_CampaignScreen::handleStartClicked()
	{
		if (std::optional<size_t> idx = findSelectedPlanetIdx()) //only start a timer if we have a valid level
		{
			SelectableLevelData uiSelectedLevelData = linearLevels[*idx];
			const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod();
			const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel();

			if (sp<CampaignConfig> campaignConfig = activeMod && currentLevel ? activeMod->getCampaign(activeCampaignIdx) : nullptr)
			{
				const std::vector<CampaignConfig::LevelData>& campaignLevels = campaignConfig->getLevels();
				const sp<TimeManager>& timerManager = currentLevel->getWorldTimeManager();
				if (timerManager && Utils::isValidIndex(campaignLevels, uiSelectedLevelData.levelIndexNumberInCampaignConfig))
				{
					const CampaignConfig::LevelData& selectedCampaignLevelData = campaignLevels[uiSelectedLevelData.levelIndexNumberInCampaignConfig];

					cachedLevelConfig = selectedCampaignLevelData.spaceLevelConfig;
					cachedLevelConfig->transientData.levelIdx = uiSelectedLevelData.levelIndexNumberInCampaignConfig;

					if (cachedLevelConfig)
					{
						cachedLevelConfig->applyDemoDataIfEmpty();

						////////////////////////////////////////////////////////
						// use ui data to overwrite first planet texture
						////////////////////////////////////////////////////////
						if (bool bOverwritePlanetTextures = (selectedCampaignLevelData.optional_defaultPlanetIdx >= 0))
						{
							const std::vector<PlanetData>& planets = cachedLevelConfig->getPlanets();
							if (planets.size() > 0)
							{
								DefaultPlanetTextureInfo planetTextureData = getDefaultPlanetTexturePathForIndex(size_t(selectedCampaignLevelData.optional_defaultPlanetIdx));
								PlanetData planetCopy = planets.back();
								planetCopy.texturePath = planetTextureData.texturePath;
								cachedLevelConfig->overridePlanetData(0, planetCopy);
							}
						}

						////////////////////////////////////////////////////////
						// set up timer
						////////////////////////////////////////////////////////
						levelTransitionTimerHandle = new_sp<MultiDelegate<>>();
						levelTransitionTimerHandle->addWeakObj(sp_this(), &Widget3D_CampaignScreen::handleLevelTransitionTimerUp);
						timerManager->createTimer(levelTransitionTimerHandle, levelTransitionDelaySec);

						starJumpVfxStartTimerHandle = new_sp<MultiDelegate<>>();
						starJumpVfxStartTimerHandle->addWeakObj(sp_this(), &Widget3D_CampaignScreen::handleDelayStarJumpVfxTimerOver);
						timerManager->createTimer(starJumpVfxStartTimerHandle, 1.f);

						////////////////////////////////////////////////////////
						// deactivate all but our target
						////////////////////////////////////////////////////////
						loadingPlanetAtUILocation.broadcast(uiSelectedLevelData.worldPos);

						bLoadingLevel = true;
						timestamp_deactivated = accumulatedTime; //cause planets to animate away
						for (Widget3D_LaserButton* button : enabledButtons) { button->activate(false); }
						for (TierData& tier : tiers)
						{
							LaserUIPool& laserPool = LaserUIPool::get();
							for (sp<LaserUIObject>& laser : tier.lasers)
							{
								laser->setColorImmediate(LaserUIPool::defaultColor);
								laserPool.releaseLaser(laser); //will set it to null within the array
							}
							tier.lasers.clear(); //clear out the nullptrs
						}
					}
				}
			}
		}
		else
		{
			STOP_DEBUGGER_HERE();//start was clicked but we couldn't find a selected planet, how can this happen?
		}
	}

	void Widget3D_CampaignScreen::handleLevelTransitionTimerUp()
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
		else
		{
			STOP_DEBUGGER_HERE();//level transition delay timer over, but level data is not available
		}
	}

	void Widget3D_CampaignScreen::handleDelayStarJumpVfxTimerOver()
	{
		//if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		//{
		//	if (SpaceLevelBase* currentSpaceLevel = dynamic_cast<SpaceLevelBase*>(currentLevel.get()))
		//	{
		//		currentSpaceLevel->enableStarJump(true, false);
		//	}
		//}

		SpaceLevelBase::staticEnableStarJump(true, false);
	}

}

