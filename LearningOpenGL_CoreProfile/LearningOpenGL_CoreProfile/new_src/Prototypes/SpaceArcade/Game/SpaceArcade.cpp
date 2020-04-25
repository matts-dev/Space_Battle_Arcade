#include "SpaceArcade.h"

#include <assert.h>
#include <random>

#include "../Rendering/SAWindow.h"
#include "../Rendering/OpenGLHelpers.h"
#include "../Rendering/Camera/SACameraFPS.h"
#include "../Rendering/SAShader.h"
#include "../Rendering/BuiltInShaders.h"

#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SAWindowSystem.h"
#include "../GameFramework/Input/SAInput.h"
#include "../GameFramework/SAAssetSystem.h"
#include "../GameFramework/SALevelSystem.h"

#include "../Tools/SAUtilities.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../../../Algorithms/SpatialHashing/SHDebugUtils.h"

#include "../GameFramework/SACollisionUtils.h"
#include "GameSystems/SAProjectileSystem.h"
#include "GameSystems/SAUISystem_Editor.h"
#include "GameSystems/SAModSystem.h"

//for quick level switching, can remove these
#include "Levels/ProjectileEditor_Level.h"
#include "Levels/ModelConfigurerEditor_Level.h"

#include "SAShip.h"
#include "Levels/BasicTestSpaceLevel.h"
#include "UI/EditorUI/SAUIRootWindow.h"
#include "SAPlayer.h"
#include "../GameFramework/SAPlayerSystem.h"
#include "../GameFramework/SATimeManagementSystem.h"
#include "UI/GameUI/SAHUD.h"
#include "SASpaceArcadeGlobalConstants.h"
#include "../Rendering/Lights/SADirectionLight.h"
#include "../Rendering/RenderData.h"
#include "../GameFramework/SARenderSystem.h"
#include "Cheats/SpaceArcadeCheatSystem.h"
#include "../GameFramework/developer_console/DeveloperConsole.h"
#include "Levels/StressTestLevel.h"

namespace SA
{
	/*static*/ SpaceArcade& SpaceArcade::get()
	{
		static sp<SpaceArcade> singleton = new_sp<SpaceArcade>();
		return *singleton;
	}

	sp<SA::Window> SpaceArcade::startUp()
	{
#if		SA_CAPTURE_SPATIAL_HASH_CELLS
		log("SpaceArcade", LogLevel::LOG_WARNING, "Capturing debug spatial hash information, this has is a perf hit and should be disabled for release");
#endif

		int width = 1500, height = 900;
		sp<SA::Window> window = new_sp<SA::Window>(width, height);
		ec(glViewport(0, 0, width, height)); //#TODO, should we do this in the gamebase level on "glfwSetFramebufferSizeCallback" changed?

		litObjShader = new_sp<SA::Shader>(litObjectShader_VertSrc, litObjectShader_FragSrc, false);
		lampObjShader = new_sp<SA::Shader>(lightLocationShader_VertSrc, lightLocationShader_FragSrc, false);
		forwardShaded_EmissiveModelShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_Emissive_fragSrc, false);
		debugLineShader = new_sp<Shader>(SH::DebugLinesVertSrc, SH::DebugLinesFragSrc, false);

		sp<SAPlayer> playerZero = getPlayerSystem().createPlayer<SAPlayer>();

		collisionShapeFactory = new_sp<CollisionShapeFactory>();

		//camera
		fpsCamera = new_sp<SA::CameraFPS>(45.f, 0.f, 0.f);
		fpsCamera->registerToWindowCallbacks_v(window);
		fpsCamera->setCursorMode(false);
		playerZero->setCamera(fpsCamera);

		AssetSystem& assetSS = getAssetSystem();

		//load models
		sp<Model3D> laserBoltModel = assetSS.loadModel(URLs.laserURL);
		sp<Model3D> fighterModel = assetSS.loadModel(URLs.fighterURL);
		sp<Model3D> carrierModel = assetSS.loadModel(URLs.carrierURL);

		GLuint radialGradientTex = 0;
		if (getAssetSystem().loadTexture("Textures/SpaceArcade/RadialGradient.png", radialGradientTex)) //#TODO change file path to use engine files
		{
			//loaded!
		}

		ui_root = new_sp<UIRootWindow>();
		hud = new_sp<HUD>();
		console = new_sp<DeveloperConsole>();

		//make sure resources are loaded before the level starts
		sp<LevelBase> startupLevel = new_sp<BasicTestSpaceLevel>();
		//sp<LevelBase> startupLevel = new_sp<StressTestLevel>();
		//sp<LevelBase> startupLevel = new_sp<ModelConfigurerEditor_Level>();
		getLevelSystem().loadLevel(startupLevel);

		return window;
	}

	void SpaceArcade::shutDown() 
	{
		fpsCamera->deregisterToWindowCallbacks_v();
	}

	void SpaceArcade::renderDebug(const glm::mat4& view, const glm::mat4& projection)
	{
#if SA_CAPTURE_SPATIAL_HASH_CELLS
		const sp<LevelBase>& world = getLevelSystem().getCurrentLevel();
		if (world)
		{
			auto& worldGrid = world->getWorldGrid();
			if (bRenderDebugCells)
			{
				glm::vec3 color{ 0.5f, 0.f, 0.f };
				SpatialHashCellDebugVisualizer::render(worldGrid, view, projection, color);
			}
			const sp<TimeManager>& worldTM = world->getWorldTimeManager();
			if (!worldTM->isTimeFrozen())
			{
				SpatialHashCellDebugVisualizer::clearCells(worldGrid);
			} 
		}
#endif //SA_CAPTURE_SPATIAL_HASH_CELLS
		if (bRenderProjectileOBBs)
		{
			projectileSystem->renderProjectileBoundingBoxes(*debugLineShader, glm::vec3(0, 0, 1), view, projection);
		}
	}

	void SpaceArcade::tickGameLoop(float deltaTimeSecs) 
	{
		using glm::vec3; using glm::vec4; using glm::mat4;

		const sp<Window>& window = getWindowSystem().getPrimaryWindow();
		if (!window)
		{
			startShutdown();
			return;
		}

		//this probably will need to become event based and have handler stack
		updateInput(deltaTimeSecs);

		console->tick(deltaTimeSecs);

		ui_root->tick(deltaTimeSecs);

	}

	void SpaceArcade::cacheRenderDataForCurrentFrame(RenderData& FRD)
	{
		FRD.reset();

		//perhaps this should just be automatically done by the level base?
		if (const sp<LevelBase>& loadedLevel = getLevelSystem().getCurrentLevel())
		{
			const std::vector<DirectionLight>& levelDirLights = loadedLevel->getDirectionalLights();
			for (uint32_t dirLightIdx = 0; dirLightIdx < GameBase::getConstants().MAX_DIR_LIGHTS; ++dirLightIdx)
			{
				if (dirLightIdx < levelDirLights.size())
				{
					FRD.dirLights[dirLightIdx] = levelDirLights[dirLightIdx];
				}
				else
				{
					FRD.dirLights[dirLightIdx] = DirectionLight{};
				}
			}

			FRD.ambientLightIntensity = loadedLevel->getAmbientLight();

			if (const sp<PlayerBase>& player = getPlayerSystem().getPlayer(0))
			{
				if (const sp<CameraBase>& camera = player->getCamera())
				{
					FRD.view = camera->getView();
					FRD.projection = camera->getPerspective();
					FRD.projection_view = FRD.projection * FRD.view;
				}
			}
		}
	}

	void SpaceArcade::renderLoop(float deltaTimeSecs)
	{
		using glm::vec3; using glm::vec4; using glm::mat4;

		const sp<Window>& window = getWindowSystem().getPrimaryWindow();
		if (!window)
		{
			return;
		}

		//prepare directional lights

		ec(glEnable(GL_DEPTH_TEST));
		ec(glClearColor(0, 0, 0, 1));
		ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

		if (const sp<LevelBase>& loadedLevel = getLevelSystem().getCurrentLevel())
		{
			if (const RenderData* FRD = getRenderSystem().getFrameRenderData_Read(getFrameNumber()))
			{
				//render world entities
				loadedLevel->render(deltaTimeSecs, FRD->view, FRD->projection);

				//#TODO rendering this needs to be done more intelligently, perhaps sorted render order or something that is extensible.
				forwardShaded_EmissiveModelShader->use();
				forwardShaded_EmissiveModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(FRD->view));
				forwardShaded_EmissiveModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(FRD->projection));
				forwardShaded_EmissiveModelShader->setUniform3f("lightColor", glm::vec3(0.8f, 0.8f, 0));
				projectileSystem->renderProjectiles(*forwardShaded_EmissiveModelShader);

				renderDebug(FRD->view, FRD->projection);
			}
		}

		hud->render(deltaTimeSecs);
	}

	void SpaceArcade::onRegisterCustomSystem()
	{
		projectileSystem = new_sp<ProjectileSystem>();
		RegisterCustomSystem(projectileSystem);

		uiSystem = new_sp<UISystem_Editor>();
		RegisterCustomSystem(uiSystem);

		modSystem = new_sp<ModSystem>();
		RegisterCustomSystem(modSystem);
	}

	sp<SA::CheatSystemBase> SpaceArcade::createCheatSystemSubclass()
	{
		//custom implementation of cheat manager for this game.
		return new_sp<SpaceArcadeCheatSystem>();
	}

	void SpaceArcade::updateInput(float detltaTimeSec)
	{
		if (const sp<Window> windowObj = getWindowSystem().getPrimaryWindow())
		{
			GLFWwindow* window = windowObj->get();

			//#TODO need a proper input system that has input bubbling/capturing and binding support from player down to entities
			static InputTracker input;
			input.updateState(window);

			if (input.isKeyJustPressed(window, GLFW_KEY_ESCAPE))
			{
				if (input.isKeyDown(window, GLFW_KEY_LEFT_SHIFT))
				{
					startShutdown();
				}
				else
				{
					ui_root->toggleUIVisible();
					if (const sp<PlayerBase>& player = getPlayerSystem().getPlayer(0))
					{
						if (sp<CameraBase> camera = player->getCamera())
						{
							camera->setCursorMode(ui_root->getUIVisible());
						}
					}
				}
			}

			if (bEnableDevConsoleFeature && console)
			{
				if (input.isKeyJustPressed(window, GLFW_KEY_GRAVE_ACCENT))
				{
					console->toggle();
				}
			}
			 
			//debug
			if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) 
			{ 
				if(input.isKeyJustPressed(window, GLFW_KEY_C)) { bRenderDebugCells = !bRenderDebugCells; }
				if (input.isKeyJustPressed(window, GLFW_KEY_V)) { bRenderProjectileOBBs = !bRenderProjectileOBBs; }
				if (input.isKeyJustPressed(window, GLFW_KEY_U)) 
				{
					//sp<Level> currentLevel = getLevelSystem().getCurrentLevel();
					//getLevelSystem().unloadLevel(currentLevel);
					sp<LevelBase> projectileEditor = new_sp<ModelConfigurerEditor_Level>();
					getLevelSystem().loadLevel(projectileEditor);
				}
			}
		}
	}
}
namespace
{
	int trueMain()
	{
		//#SUGGESTED remove memory leak detecting code, it isn't very helpful (it cannot produce detailed output described at MSDN) with the way I've used c++.

		//HEAP snapshots are much more informative.
#if FIND_MEMORY_LEAKS
		std::cout << "memory leak detection ON - remove this before release. This should not ship with release." << std::endl;
#endif //FIND_MEMORY_LEAKS
		{ //scoping object so that memory is freed before memory report
			SA::SpaceArcade& game = SA::SpaceArcade::get();
			game.start();
		}
	
#if FIND_MEMORY_LEAKS
		_CrtDumpMemoryLeaks(); //if multiple exit points are generated, use a the bitflag approach for printing this report 
#endif //FIND_MEMORY_LEAKS

		return 0;
	}
}


int main()
{
	int result = trueMain();
	return result;
}