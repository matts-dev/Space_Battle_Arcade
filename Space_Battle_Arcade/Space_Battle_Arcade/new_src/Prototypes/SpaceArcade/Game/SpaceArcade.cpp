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
#include "GameSystems/SAUISystem_Game.h"
#include "UI/GameUI/Widgets3D/Widget3D_Base.h"
#include "Levels/MainMenuLevel.h"
#include "Levels/EnigmaTutorials/EnigmaTutorialsLevel.h"
#include "GameSystems/SystemData/SATickGroups.h"
#include "UI/GameUI/Widgets3D/HUD/PlayerPilotAssistUI.h"
#include "../Rendering/DeferredRendering/DeferredRendererStateMachine.h"
#include "../Rendering/ForwardRendering/ForwardRenderingStateMachine.h"
#include "Levels/SpaceLevelEditor_Level.h"
#include "../GameFramework/SALog.h"

namespace SA
{
	/*static*/ SpaceArcade& SpaceArcade::get()
	{
		static sp<SpaceArcade> singleton = new_sp<SpaceArcade>();
		return *singleton;
	}

	sp<SA::Window> SpaceArcade::makeInitialWindow()
	{
		int width = 1440, height = 810;
		sp<SA::Window> window = new_sp<SA::Window>(width, height);
		ec(glViewport(0, 0, width, height)); //#TODO, should we do this in the gamebase level on "glfwSetFramebufferSizeCallback" changed?
		return window;
	}

	void SpaceArcade::startUp()
	{
#if		SA_CAPTURE_SPATIAL_HASH_CELLS
		log("SpaceArcade", LogLevel::LOG_WARNING, "Capturing debug spatial hash information, this has is a perf hit and should be disabled for release");
#endif
		sp<Window> window = GameBase::get().getWindowSystem().getPrimaryWindow();
		if (!window)
		{
			log("SpaceArcade", LogLevel::LOG_ERROR, "Failed to get window during startup.");
			startShutdown();
			return;
		}
		if (GLFWwindow* windowRaw = window->get())
		{
			glfwSetWindowTitle(windowRaw, "Space Battle Arcade");
		}

		getLevelSystem().onPostLevelChange.addWeakObj(sp_this(), &SpaceArcade::handlePostLevelChange);

		litObjShader = new_sp<SA::Shader>(litObjectShader_VertSrc, litObjectShader_FragSrc, false);
		lampObjShader = new_sp<SA::Shader>(lightLocationShader_VertSrc, lightLocationShader_FragSrc, false);
		//forwardShaded_EmissiveModelShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_Emissive_fragSrc, false);
		debugLineShader = new_sp<Shader>(SH::DebugLinesVertSrc, SH::DebugLinesFragSrc, false);

		sp<SAPlayer> playerZero = getPlayerSystem().createPlayer<SAPlayer>();

		collisionShapeFactory = new_sp<CollisionShapeFactory>();

		//camera //#TODO remove this or make it so that it is only spawned if level doesn't configure a camera
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

		ui_root_editor = new_sp<UIRootWindow>();
		hud = new_sp<HUD>();
		console = new_sp<DeveloperConsole>();
		playerPilotAssistUI = new_sp<PlayerPilotAssistUI>();
			
		//make sure resources are loaded before the level starts
		sp<LevelBase> startupLevel = new_sp<MainMenuLevel>();
		//sp<LevelBase> startupLevel = new_sp<SpaceLevelEditor_Level>();
		//sp<LevelBase> startupLevel = new_sp<BasicTestSpaceLevel>();
		//sp<LevelBase> startupLevel = new_sp<EnigmaTutorialLevel>();
		//sp<LevelBase> startupLevel = new_sp<StressTestLevel>();
		//sp<LevelBase> startupLevel = new_sp<ModelConfigurerEditor_Level>();
		getLevelSystem().loadLevel(startupLevel);

		if (bEnableDebugEngineKeybinds)
		{
			log(__FUNCTION__, LogLevel::LOG_WARNING, "SpaceArcade::bEnableDebugEngineKeybinds is true, this should be false for shipping builds.");
		}

#if !IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
		todo_deferred_rendering_is_not_compelte___change_this_preprocessor_flag_to_find_remaining_work___then_debug;
		todo_enabling_of_deferred_renderer_needs_to_happen_before_system_initalization___NOT_HERE____otherwise_some_early_bird_system_shader_initaliation_will_not_create_deferred_shaders_i_think;
		getRenderSystem().enableDeferredRenderer(true);
#endif //IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
	}

	void SpaceArcade::onShutDown() 
	{
		fpsCamera->deregisterToWindowCallbacks_v();
	}

	void SpaceArcade::toggleEditorUIMainMenuVisible()
	{
		ui_root_editor->toggleUIVisible();
	}

	bool SpaceArcade::isEditorMainMenuOnScreen() const
	{
		return ui_root_editor->getUIVisible();
	}

	void SpaceArcade::setEscapeShouldOpenEditorMenu(bool bValue)
	{
		bEscapeShouldOpenEditorMenu = bValue;
	}

	void SpaceArcade::setClearColor(glm::vec3 inClearColor)
	{
		renderClearColor = inClearColor;
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

		ui_root_editor->tick(deltaTimeSecs);

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

			if (const sp<TimeManager>& worldTimeManager = loadedLevel->getWorldTimeManager())
			{
				FRD.dt_sec = worldTimeManager->getDeltaTimeSecs();
			}
			FRD.ambientLightIntensity = loadedLevel->getAmbientLight();

			const std::vector<sp<PlayerBase>>& allPlayers = getPlayerSystem().getAllPlayers();
			FRD.playerCamerasPositions.reserve(allPlayers.size());

			FRD.renderClearColor = renderClearColor;

			//#todo #splitscreen
			if (const sp<PlayerBase>& player = getPlayerSystem().getPlayer(0))
			{
				if (const sp<CameraBase>& camera = player->getCamera())
				{
					FRD.view = camera->getView();
					FRD.projection = camera->getPerspective();
					FRD.projection_view = FRD.projection * FRD.view;
					FRD.playerCamerasPositions[0] = camera->getPosition();
				}
			}
		}
	}

	void SpaceArcade::renderLoop_begin(float deltaTimeSecs)
	{
		using glm::vec3; using glm::vec4; using glm::mat4;

		const sp<Window>& window = getWindowSystem().getPrimaryWindow();
		if (!window)
		{
			return;
		}

		if (ForwardRenderingStateMachine* forwardRenderer = getRenderSystem().getForwardRenderer())
		{
			//do nothing, the forward render stages will handle clearing
		}
		else
		{//clear frame buffer
			//#TODO remove this clear after rendering refactor is done
			ec(glEnable(GL_DEPTH_TEST));
			ec(glEnable(GL_STENCIL_TEST)); //enabling to ensure that we clear stencil every frame (may not be necessary)
			ec(glStencilMask(0xff)); //enable complete stencil writing so that clear will clear stencil buffer (also, not tested if necessary)
			ec(glClearColor(renderClearColor.r, renderClearColor.g, renderClearColor.b, 1));

			ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

			ec(glStencilMask(0x0)); //we cleared stencil buffer, stop writing to stencil buffer.
			ec(glDisable(GL_STENCIL_TEST)); //only enable stencil test on demand
		}

		if (const sp<LevelBase>& loadedLevel = getLevelSystem().getCurrentLevel())
		{
			if (const RenderData* FRD = getRenderSystem().getFrameRenderData_Read(getFrameNumber()))
			{
				if (DeferredRendererStateMachine* deferredRenderer = getRenderSystem().getDeferredRenderer())
				{ //////////////////////// Deferred Rendering //////////////////////////////////////////
					deferredRenderer->beginGeometryPass(renderClearColor);

					//render world entities
					loadedLevel->render(deltaTimeSecs, FRD->view, FRD->projection);
				}
				else 
				{ //////////////////////// Forward Rendering //////////////////////////////////////////
					if (ForwardRenderingStateMachine* forwardRenderer = getRenderSystem().getForwardRenderer())
					{
						forwardRenderer->stage_HDR(FRD->renderClearColor);
					}

					//render world entities
					loadedLevel->render(deltaTimeSecs, FRD->view, FRD->projection);
					renderDebug(FRD->view, FRD->projection);

					//rendering UI here will cause it to be tone mapped
					//uiSystem_Game->runGameUIPass(); //render non-editor ui, like HUD and 3D widgets
				}
			}
		}
	}

	void SpaceArcade::renderLoop_end(float deltaTimeSecs)
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// start lighting pass if using a deferred renderer
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (DeferredRendererStateMachine* deferredRenderer = getRenderSystem().getDeferredRenderer())
		{
			deferredRenderer->beginLightPass();
			deferredRenderer->beginPostProcessing();

			//the legacy debug rendering still needs to use forward shading, so it should come after deferred post processing
			if (const sp<LevelBase>& loadedLevel = getLevelSystem().getCurrentLevel())
			{
				if (const RenderData* FRD = getRenderSystem().getFrameRenderData_Read(getFrameNumber()))
				{
					renderDebug(FRD->view, FRD->projection);
				}
			}
		}
		else if (ForwardRenderingStateMachine* forwardRenderer = getRenderSystem().getForwardRenderer())
		{
			//tone map world rendering
			forwardRenderer->stage_ToneMapping(renderClearColor); //todo should use frame render data for clear color, but isn't that important atm

			//render UI and other things that do not need to be tone mapped
			uiSystem_Game->runGameUIPass(); //render non-editor ui, like HUD and 3D widgets #TODO hook this into delegate below
			onForwardToneMappingComplete.broadcast();

			//as a last step, apply MSAA and render to default frame buffer.
			forwardRenderer->stage_MSAA();
		}
	}

	void SpaceArcade::onRegisterCustomSystem()
	{
		projectileSystem = new_sp<ProjectileSystem>();
		RegisterCustomSystem(projectileSystem);

		uiSystem_Editor = new_sp<UISystem_Editor>();
		RegisterCustomSystem(uiSystem_Editor);

		uiSystem_Game = new_sp<UISystem_Game>();
		RegisterCustomSystem(uiSystem_Game);

		modSystem = new_sp<ModSystem>();
		RegisterCustomSystem(modSystem);
	}

	sp<SA::CheatSystemBase> SpaceArcade::createCheatSystemSubclass()
	{
		//custom implementation of cheat manager for this game.
		return new_sp<SpaceArcadeCheatSystem>();
	}

	sp<SA::TickGroups> SpaceArcade::onRegisterTickGroups()
	{
		//creating a tick group (or table) is the only time they will be registered as valid tick groups.
		//this is a form of system encapsulation that is 100% intentional to prevent any weird misuse of API
		//that allows changing of tick group priorities
		_SATickGroups = new_sp<SATickGroups>();
		return _SATickGroups;
	}

	void SpaceArcade::updateInput(float detltaTimeSec)
	{
		if (const sp<Window> windowObj = getWindowSystem().getPrimaryWindow())
		{
			GLFWwindow* window = windowObj->get();

			//#TODO need a proper input system that has input bubbling/capturing and binding support from player down to entities
			static InputTracker input;
			input.updateState(window);

			//moving default input to player class
			//if (input.isKeyJustPressed(window, GLFW_KEY_ESCAPE))
			//{
			//	if (input.isKeyDown(window, GLFW_KEY_LEFT_SHIFT))
			//	{
			//		startShutdown();
			//	}
			//	else
			//	{
			//		ui_root->toggleUIVisible();
			//		if (const sp<PlayerBase>& player = getPlayerSystem().getPlayer(0))
			//		{
			//			if (sp<CameraBase> camera = player->getCamera())
			//			{
			//				camera->setCursorMode(ui_root->getUIVisible());
			//			}
			//		}
			//	}
			//}

			if (bEnableDevConsoleFeature && console)
			{
				if (input.isKeyJustPressed(window, GLFW_KEY_GRAVE_ACCENT))
				{
					console->toggle();
				}
			}
			 
			//debug
			if (bEnableDebugEngineKeybinds)
			{
				if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
				{
					if (input.isKeyJustPressed(window, GLFW_KEY_C)) { bRenderDebugCells = !bRenderDebugCells; }
					if (input.isKeyJustPressed(window, GLFW_KEY_V)) { bRenderProjectileOBBs = !bRenderProjectileOBBs; }
					if (input.isKeyJustPressed(window, GLFW_KEY_U))
					{
						//sp<Level> currentLevel = getLevelSystem().getCurrentLevel();
						//getLevelSystem().unloadLevel(currentLevel);
						sp<LevelBase> projectileEditor = new_sp<ModelConfigurerEditor_Level>();
						getLevelSystem().loadLevel(projectileEditor);
					}
					if (input.isKeyJustPressed(window, GLFW_KEY_S))
					{
						if(ForwardRenderingStateMachine* forwardRenderer = getRenderSystem().getForwardRenderer())
						{
							forwardRenderer->setUseMultiSample(!forwardRenderer->isUsingMultiSample());
						}
					}
				}
				if (DeferredRendererStateMachine* deferredRenderer = getRenderSystem().getDeferredRenderer())
				{
					if (input.isKeyJustPressed(window, GLFW_KEY_1)) { deferredRenderer->setDisplayBuffer(DeferredRendererStateMachine::BufferType::NORMAL); }
					if (input.isKeyJustPressed(window, GLFW_KEY_2)) { deferredRenderer->setDisplayBuffer(DeferredRendererStateMachine::BufferType::POSITION); }
					if (input.isKeyJustPressed(window, GLFW_KEY_3)) { deferredRenderer->setDisplayBuffer(DeferredRendererStateMachine::BufferType::ALBEDO_SPEC); }
					if (input.isKeyJustPressed(window, GLFW_KEY_4)) { deferredRenderer->setDisplayBuffer(DeferredRendererStateMachine::BufferType::LIGHTING); }
				}
				else if (!SHIPPING_BUILD)
				{
					std::optional<size_t> renderMode;
					static std::optional<bool> useNormalMapping;
					static std::optional<bool> useNormalMappingMirrorCorrection;
					static std::optional<bool> correctNormalMapSeams;
					if (input.isKeyJustPressed(window, GLFW_KEY_1)) { renderMode = 0; }
					if (input.isKeyJustPressed(window, GLFW_KEY_2)) { renderMode = 1; } // normal
					if (input.isKeyJustPressed(window, GLFW_KEY_3)) { renderMode = 2; } // normal red
					if (input.isKeyJustPressed(window, GLFW_KEY_4)) { renderMode = 3; } // normal green
					if (input.isKeyJustPressed(window, GLFW_KEY_5)) { renderMode = 4; } // normal blue
					if (input.isKeyJustPressed(window, GLFW_KEY_6)) { useNormalMapping = !useNormalMapping.value_or(true); } // normal blue
					if (input.isKeyJustPressed(window, GLFW_KEY_7)) { useNormalMappingMirrorCorrection = !useNormalMappingMirrorCorrection.value_or(true); } // normal blue
					if (input.isKeyJustPressed(window, GLFW_KEY_8)) { correctNormalMapSeams = !correctNormalMapSeams.value_or(false); } // normal blue
					if (renderMode || correctNormalMapSeams.has_value() || useNormalMapping.has_value() || useNormalMappingMirrorCorrection.has_value())
					{
						const sp<LevelBase>& currentLevel = getLevelSystem().getCurrentLevel();
						if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(currentLevel.get()))
						{
							if (renderMode)
							{
								spaceLevel->debug_renderMode(*renderMode);
							}
							if (correctNormalMapSeams.has_value())
							{
								spaceLevel->debug_correctNormalMapSeamsOverride(*correctNormalMapSeams);
							}
							if (useNormalMapping.has_value())
							{
								spaceLevel->debug_useNormalMappingOverride(*useNormalMapping);
							}
							if (useNormalMappingMirrorCorrection.has_value())
							{
								logf_sa(__FUNCTION__, LogLevel::LOG_WARNING, "normal map mirror TBN flip on [%d]", int(*useNormalMappingMirrorCorrection));
								spaceLevel->debug_useNormalMappingMirrorCorrection(*useNormalMappingMirrorCorrection);
							}
						}
					}
				}
			}
		}
	}

	void SpaceArcade::handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel)
	{
		if (SHIPPING_BUILD)
		{
			log(__FUNCTION__, LogLevel::LOG, "Configuring escape menu due to SHIPPING_BUILD flag");
			//Utils::configureDebugEscapeMenuForLevel(newCurrentLevel);

			if (newCurrentLevel)
			{
				if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(newCurrentLevel.get()))
				{
					//if we are a space level, then the editor levels will be both a test level and a menu level, they should see the escape menu in shipping builds
					bool bShouldDevEscapeMenuBeUsed = spaceLevel->isTestLevel() || spaceLevel->isEditorLevel();
					setEscapeShouldOpenEditorMenu(bShouldDevEscapeMenuBeUsed);
				}
				else
				{
					//if we're not in a space level base, then we're in some sort of editor level
					setEscapeShouldOpenEditorMenu(newCurrentLevel->isEditorLevel());
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