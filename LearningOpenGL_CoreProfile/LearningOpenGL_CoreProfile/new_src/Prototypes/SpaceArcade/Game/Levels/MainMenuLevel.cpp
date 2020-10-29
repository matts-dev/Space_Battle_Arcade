#include "MainMenuLevel.h"
#include<ctime>
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "../../GameFramework/SAGameBase.h"
#include "../Environment/Planet.h"
#include "../Environment/Star.h"
#include "../UI/GameUI/text/DigitalClockFont.h"
#include "../SpaceArcade.h"
#include "../GameSystems/SAUISystem_Game.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../GameFramework/SAWindowSystem.h"
#include "../../Rendering/Camera/SAQuaternionCamera.h"
#include "../UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_GameMainMenuScreen.h"
#include "../UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_MenuScreenBase.h"
#include "../../Tools/PlatformUtils.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_CampaignScreen.h"
#include "../UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_SkirmishScreen.h"
#include "../UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_SettingScreen.h"
#include "../UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_ModsScreen.h"
#include "../UI/GameUI/Widgets3D/MainMenuScreens/Widget3D_ExitScreen.h"
#include "../../Tools/SAUtilities.h"
#include "../UI/GameUI/SAHUD.h"
#include "../SAPlayer.h"
#include "../../GameFramework/SAAudioSystem.h"
#include "../AssetConfigs/SASettingsProfileConfig.h"

namespace SA
{

	void MainMenuLevel::postConstruct()
	{
		setScreensData(); //configure data before parent calls generate planets/stars

		Parent::postConstruct();

		DigitalClockFont::Data debugTextInit;
		debugTextInit.shader = getDefaultGlyphShader_instanceBased();
		debugTextInit.text = "debug";
		debugText = new_sp<DigitalClockFont>(debugTextInit);

		menuCamera = new_sp<QuaternionCamera>();
		menuCamera->setCameraRequiresCursorMode(true);
		menuCamera->setEnableCameraRoll(false);
		menuCamera->setEnableCameraMovement(false);

		if (const sp<HUD> hud = SpaceArcade::get().getHUD())
		{
			hud->setVisibility(false);
		}

		if (bFreeCamera) //debug
		{
			//undo camera set up 
			menuCamera->setCameraRequiresCursorMode(false);
			menuCamera->setEnableCameraRoll(true);
			menuCamera->setEnableCameraMovement(true);

			SpaceArcade::get().setEscapeShouldOpenEditorMenu(true); //if using free camera, allow escape to eenter cursor mode so that things can be clicked
		}

		SpaceArcade& game = SpaceArcade::get();
		game.getGameUISystem()->onUIGameRender.addWeakObj(sp_this(), &MainMenuLevel::handleGameUIRenderDispatch);

		camCurve = GameBase::get().getCurveSystem().generateSigmoid_medp(20.0f);		//this feels the best

		mainMenuScreen = new_sp<Widget3D_GameMainMenuScreen>();
		mainMenuScreen->getCampaignClicked().addWeakObj(sp_this(), &MainMenuLevel::handleCampaignClicked);
		mainMenuScreen->getSkirmishClicked().addWeakObj(sp_this(), &MainMenuLevel::handleSkirmishClicked);
		mainMenuScreen->getModsClicked().addWeakObj(sp_this(), &MainMenuLevel::handleModsClicked);
		mainMenuScreen->getSettingsClicked().addWeakObj(sp_this(), &MainMenuLevel::handleSettingsClicked);
		mainMenuScreen->getExitClicked().addWeakObj(sp_this(), &MainMenuLevel::handleExitClicked);
		menuScreens.push_back(mainMenuScreen.get());

		campaignScreen = new_sp<Widget3D_CampaignScreen>();
		campaignScreen->getBackButton().addWeakObj(sp_this(), &MainMenuLevel::handleReturnToMainMenuClicked);
		campaignScreen->activate(false);
		campaignScreen->loadingPlanetAtUILocation.addWeakObj(sp_this(), &MainMenuLevel::handleMoveCameraToCampaignPlanet);
		menuScreens.push_back(campaignScreen.get());

		skirmishScreen = new_sp<Widget3D_SkirmishScreen>();
		skirmishScreen->getBackButton().addWeakObj(sp_this(), &MainMenuLevel::handleReturnToMainMenuClicked);
		menuScreens.push_back(skirmishScreen.get());

		settingsScreen = new_sp<Widget3D_SettingsScreen>();
		settingsScreen->getBackButton().addWeakObj(sp_this(), &MainMenuLevel::handleReturnToMainMenuClicked);
		menuScreens.push_back(settingsScreen.get());

		modsScreen = new_sp<Widget3D_ModsScreen>();
		modsScreen->getBackButton().addWeakObj(sp_this(), &MainMenuLevel::handleReturnToMainMenuClicked);
		menuScreens.push_back(modsScreen.get());

		exitScreen = new_sp<Widget3D_ExitScreen>();
		exitScreen->getBackButton().addWeakObj(sp_this(), &MainMenuLevel::handleReturnToMainMenuClicked);
		menuScreens.push_back(exitScreen.get());




	}

	void MainMenuLevel::startLevel_v()
	{
		Parent::startLevel_v();

		SpaceArcade& game = SpaceArcade::get();

		WindowSystem& windowSystem = game.getWindowSystem();
		windowSystem.onPrimaryWindowChangingEvent.addWeakObj(sp_this(), &MainMenuLevel::handlePrimaryWindowChanging);
		if (const sp<Window>& primaryWindow = game.getWindowSystem().getPrimaryWindow())
		{
			handlePrimaryWindowChanging(nullptr, primaryWindow);
		}

		mainMenuStartLevelDelayActiveTimer->addWeakObj(sp_this(), &MainMenuLevel::handleMainMenuStartupDelayOver);
		getWorldTimeManager()->createTimer(mainMenuStartLevelDelayActiveTimer, MainMenuActivationDelaySec);
	}

	void MainMenuLevel::onCreateLocalPlanets()
	{
		Parent::onCreateLocalPlanets();

		using namespace glm;

		constexpr bool generateRandomPlanet = true;
		sp<RNG> rng = GameBase::get().getRNGSystem().getSeededRNG(generateRandomPlanet ? uint32_t(std::time(NULL)) : 13);


		std::vector<sp<class Planet>> planetArray = makeRandomizedPlanetArray(*rng);
		size_t planetIdx = 0;

		////////////////////////////////////////////////////////
		// main menu planet 1
		////////////////////////////////////////////////////////
		if (planetIdx < planetArray.size())
		{
			planets.push_back(planetArray[planetIdx++]);

			mainScreen_planet1 = planets.back();
			Transform planetXform;
			planetXform.position = glm::normalize(glm::vec3(0.425f, -0.225f, -0.8f)) * 4.0f;
			planetXform.rotQuat = glm::angleAxis(glm::radians(200.f), normalize(vec3(1, -1, 0))); //hide defect with some planet model textures, at least initially
			mainScreen_planet1->setTransform(planetXform);
			mainScreen_planet1->setRotationAxis(glm::normalize(vec3(1, 1, 0)));
			mainScreen_planet1->setRotationSpeed_radsec(glm::radians(1.f));
		}

		////////////////////////////////////////////////////////
		// main menu planet 2 far away
		////////////////////////////////////////////////////////
		if (planetIdx < planetArray.size())
		{
			planets.push_back(planetArray[planetIdx++]);

			mainScreen_planet1 = planets.back();
			Transform planetXform;
			planetXform.position = glm::normalize(glm::vec3(-0.45f, 0.25f, -0.8f)) * 30.0f;
			mainScreen_planet1->setTransform(planetXform);
			mainScreen_planet1->setRotationAxis(glm::normalize(vec3(-1, 1, 0)));
			mainScreen_planet1->setRotationSpeed_radsec(glm::radians(1.f));
		}

	}

	void MainMenuLevel::onCreateLocalStars()
	{
		Parent::onCreateLocalStars();

		const sp<Star>& star = localStars.back();
		SA::Transform newStarXform;
		//newStarXform.position = glm::normalize(glm::vec3(1.0f,0.25f, -0.25f)) * 50.0f;
		//newStarXform.position = glm::normalize(glm::vec3(-0.8f, -0.1f, 0.5f)) * 50.0f;
		//newStarXform.position = glm::normalize(glm::vec3(-1.f, -0.1f, 0.0f)) * 50.0f;
		newStarXform.position = glm::normalize(glm::vec3(-1.f, 0.5f, -0.5f)) * 50.0f;
		star->setXform(newStarXform);
	}

	//void MainMenuLevel::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	//{
	//	Parent::render(dt_sec, view, projection);

	//}

	void MainMenuLevel::tick_v(float dt_sec)
	{
		Parent::tick_v(dt_sec);

		for (Widget3D_MenuScreenBase* menuScreen : menuScreens)
		{
			if (menuScreen)
			{
				menuScreen->tick(dt_sec);
			}
			else{STOP_DEBUGGER_HERE();}
		}
		updateCamera(dt_sec);
	}

	void MainMenuLevel::handleGameUIRenderDispatch(GameUIRenderData& uiRenderData)
	{
		if (const RenderData* gameRD = uiRenderData.renderData())
		{
			if (bRenderDebugText)
			{
				if (const sp<HUD> hud = SpaceArcade::get().getHUD())
				{
					hud->setVisibility(true); //show crosshairs where we are pointing
				}

				Transform debugTextXform;
				debugTextXform.rotQuat = menuCamera->getQuat();
				debugTextXform.position = menuCamera->getPosition() + menuCamera->getFront() * 50.f;

				const glm::vec3 front = menuCamera->getFront();
				char textBuffer[1024];
				snprintf(textBuffer, sizeof(textBuffer), "(%3.1f),(%3.1f),(%3.1f)", front.x, front.y, front.z);
				debugText->setText(textBuffer);
				debugText->setXform(debugTextXform);

				debugText->renderGlyphsAsInstanced(*gameRD);
			}
		}
	}

	void MainMenuLevel::handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window)
	{
		if (new_window)
		{
			SpaceArcade& game = SpaceArcade::get();
			if (const sp<PlayerBase>& player = game.getPlayerSystem().getPlayer(0))
			{
				//set up player camera
				player->setCamera(menuCamera);
				menuCamera->registerToWindowCallbacks_v(new_window);
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_ERROR, "Main menu started but no player is available. Main menu will be broken!");
			}
		}
	}

	void MainMenuLevel::setScreensData()
	{

	}

	void MainMenuLevel::handleMainMenuStartupDelayOver()
	{
		mainMenuScreen->activate(true); 

		static bool firstLoad = true;
		if (firstLoad)
		{
			firstLoad = false;
			if (const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0))
			{
				if (SAPlayer* SaPlayer = dynamic_cast<SAPlayer*>(player.get()))
				{
					if (const sp<SettingsProfileConfig>& settingsProfile = SaPlayer->getSettingsProfile())
					{
						////////////////////////////////////////////////////////
						// set up audio
						////////////////////////////////////////////////////////
						AudioSystem& audioSystem = GameBase::get().getAudioSystem();
						audioSystem.setSystemVolumeMultiplier(settingsProfile->volumeMultiplier);

						////////////////////////////////////////////////////////
						// set up team
						////////////////////////////////////////////////////////
						//settingsProfile->selectedTeamIdx; //this is respected when joining level, so no need to do any work here with current implementation
					}
				}
			}
		}
	}

	void MainMenuLevel::updateCamera(float dt_sec)
	{
		using namespace glm;

		if (cameraAnimData.has_value())
		{
			cameraAnimData->timePassedSec += dt_sec;

			//very addhoc way of implementing this, but each time range does a different thing and is assumed to be discrete steps
			if (cameraAnimData->timePassedSec <= cameraAnimData->cameraAnimDuration)
			{
				float percDone = cameraAnimData->timePassedSec / cameraAnimData->cameraAnimDuration; //[0,1]
				vec3 toStart_n = normalize(cameraAnimData->startPoint - menuCamera->getPosition());
				vec3 toEnd_n = normalize(cameraAnimData->endPoint - menuCamera->getPosition());

				//NOTE: below is NOT CORRECT, which is why it is off. I don't think it is actually needed as all screens basically undo their rotations to get back to main menu.
				//NOTE: better approach is probably to actually rotate the end quaternion's so that they're aligned with front/up plane, not do roll during animation.
				//Figure out if up vector is drifting out of plane to camera front, and correct it
				if constexpr (constexpr bool bCorrectCameraDrift = false)
				{
					const vec3 camUp_n = menuCamera->getUp();
					const vec3 camFront_n = menuCamera->getFront();
					const vec3 targetWorldUp_n{ 0.f,1.f,0.f };
					const vec3 planeU_n = glm::normalize(glm::cross(-camFront_n, targetWorldUp_n));

					//project to worldUp_front_plane, then see if different and rotate roll if different. plane is similar to cam right/up plane
					const vec3 uProj = glm::dot(camUp_n, planeU_n) * planeU_n; //probably need to make projection and plane projection a function in math library
					const vec3 vProj = glm::dot(camUp_n, targetWorldUp_n) * targetWorldUp_n; //probably need to make projection and plane projection a function in math library
					const vec3 proj_n = normalize(uProj + vProj);
					float normalRelatedness_costheta = glm::dot(proj_n, targetWorldUp_n);
					//if (normalRelatedness < 0.99f) //same vectors will product value close to 1.0f
					{
						//up vector is drifting, roll camera to correct drift
						float roll_rad = glm::acos(glm::clamp(normalRelatedness_costheta, -1.f, 1.f));
						bool bOnLeft = (glm::dot(proj_n, planeU_n) < 0.f);

						vec3 rollAxis = glm::cross(planeU_n, targetWorldUp_n);
						quat rollQ = glm::angleAxis(bOnLeft ? -roll_rad : roll_rad, rollAxis);

						//sometimes the roll will product nan because we've supplied a bad camera move (ie makes 180 with up vector), rather than account
						//for that in the logic, just ignore the roll. this will likely cause a camera flip letting user (me) know they're doing something I don't
						//want to support!
						if (!Utils::anyValueNAN(rollQ))
						{
							//update roll before doing other logic
							menuCamera->setQuat(rollQ * menuCamera->getQuat());
						}
					}
				}

				menuCamera->lookAt_v(cameraAnimData->startPoint);
				quat startQ = menuCamera->getQuat();

				menuCamera->lookAt_v(cameraAnimData->endPoint);
				quat endQ = menuCamera->getQuat();

				quat fullRotQ = glm::slerp(startQ, endQ, camCurve.eval_smooth(percDone));
				menuCamera->setQuat(fullRotQ);
			}
			else
			{
				//make sure we finish the interpolation
				menuCamera->lookAt_v(cameraAnimData->endPoint);

				if (cameraAnimData->pendingScreenToActivate) { cameraAnimData->pendingScreenToActivate->activate(true);}
				else if (cameraAnimData->bIsSubScreenAnimation) {/*don't check/ensure if we're looking to activate a screen*/ }
				else { STOP_DEBUGGER_HERE(); } // did you forget to set a screen to animate to?

				//prevent this from ticking until next animation
				cameraAnimData.reset();
			}

		}
	}

	void MainMenuLevel::animateCameraTo(glm::vec3 endPoint, float animDuration)
	{
		cameraAnimData = std::make_optional<CameraAnimData>();
		cameraAnimData->timePassedSec = 0.f;
		cameraAnimData->startPoint = /*shouldbe(0,0,0)*/menuCamera->getPosition() + menuCamera->getFront(); //may be better to scale this vector a bit to generate a point
		cameraAnimData->endPoint = endPoint;
		cameraAnimData->cameraAnimDuration = animDuration;
	}

	void MainMenuLevel::setPendingScreenToActivate(Widget3D_ActivatableBase* screen)
	{
		if (cameraAnimData.has_value())
		{
			cameraAnimData->pendingScreenToActivate = screen;
		}
		else
		{
			STOP_DEBUGGER_HERE(); //should always have camera anim data if we call this function
			assert(false);
		}
	}

	void MainMenuLevel::deactivateAllScreens()
	{
		for (Widget3D_MenuScreenBase* screen : menuScreens)
		{
			if (screen->isActive()) 
			{
				screen->activate(false);
			}
		}
	}

	void MainMenuLevel::handleCampaignClicked()
	{
		deactivateAllScreens();
		animateCameraTo(glm::vec3(-0.7f, 0.2f, 0.7f), 3.0f);
		setPendingScreenToActivate(campaignScreen.get());
	}

	void MainMenuLevel::handleSkirmishClicked()
	{
		deactivateAllScreens();
		animateCameraTo(glm::vec3(0.6f, -0.3f, 0.8f), 3.0f);
		setPendingScreenToActivate(skirmishScreen.get());
	}

	void MainMenuLevel::handleModsClicked()
	{
		deactivateAllScreens();
		animateCameraTo(glm::vec3(-0.7f, -0.7f, 0.1f), 3.0f);
		setPendingScreenToActivate(modsScreen.get());
	}

	void MainMenuLevel::handleSettingsClicked()
	{
		deactivateAllScreens();
		animateCameraTo(glm::vec3(0.7f, 0.8f, 0.0f), 3.0f);
		setPendingScreenToActivate(settingsScreen.get());
	}

	void MainMenuLevel::handleExitClicked()
	{
		deactivateAllScreens();
		animateCameraTo(glm::vec3(-0.1f, -0.4f, 0.9f), 3.0f);
		setPendingScreenToActivate(exitScreen.get());
	}

	void MainMenuLevel::handleReturnToMainMenuClicked()
	{
		deactivateAllScreens();
		animateCameraTo(glm::vec3(-0.f, -0.f, -1.f), 3.0f);
		setPendingScreenToActivate(mainMenuScreen.get());
	}

	void MainMenuLevel::handleMoveCameraToCampaignPlanet(const glm::vec3& worldPos)
	{
		animateCameraTo(worldPos - menuCamera->getPosition(), 1.f);
		cameraAnimData->bIsSubScreenAnimation = true;
	}

}














































