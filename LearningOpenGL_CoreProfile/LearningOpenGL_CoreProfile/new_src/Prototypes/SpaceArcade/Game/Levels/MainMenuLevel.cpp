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

		SpaceArcade& game = SpaceArcade::get();
		game.getGameUISystem()->onUIGameRender.addWeakObj(sp_this(), &MainMenuLevel::handleGameUIRenderDispatch);

		mainMenuScreen = new_sp<Widget3D_GameMainMenuScreen>();
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

	void MainMenuLevel::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		Parent::render(dt_sec, view, projection);


	}

	void MainMenuLevel::handleGameUIRenderDispatch(GameUIRenderData& uiRenderData)
	{
		if (const RenderData* gameRD = uiRenderData.renderData())
		{
			if (bRenderDebugText)
			{
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
	}

}














































