#include "SpaceArcade.h"

#include <assert.h>
#include <random>

#include "../Rendering/SAWindow.h"
#include "../Rendering/OpenGLHelpers.h"
#include "../Rendering/Camera/SACameraFPS.h"
#include "../Rendering/SAShader.h"
#include "../Rendering/BuiltInShaders.h"

#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SAWindowSubsystem.h"
#include "../GameFramework/Input/SAInput.h"
#include "../GameFramework/SATextureSubsystem.h"

#include "../Tools/SAUtilities.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../../../Algorithms/SpatialHashing/SHDebugUtils.h"

#include "SACollisionUtils.h"
#include "SAProjectileSubsystem.h"
#include "SAUISubsystem.h"

#include "SAShip.h"
#include "Levels/BasicTestSpaceLevel.h"
#include "Levels/ProjectileEditor_Level.h"
#include "UI/SAUIRootWindow.h"
#include "SAPlayer.h"
#include "../GameFramework/SAPlayerSubsystem.h"

namespace SA
{
	/*static*/ SpaceArcade& SpaceArcade::get()
	{
		static sp<SpaceArcade > singleton = new_sp<SpaceArcade>();
		return *singleton;
	}

	sp<SA::Window> SpaceArcade::startUp()
	{
		int width = 1500, height = 900;
		sp<SA::Window> window = new_sp<SA::Window>(width, height);
		ec(glViewport(0, 0, width, height)); //TODO, should we do this in the gamebase level on "glfwSetFramebufferSizeCallback" changed?

		litObjShader = new_sp<SA::Shader>(litObjectShader_VertSrc, litObjectShader_FragSrc, false);
		lampObjShader = new_sp<SA::Shader>(lightLocationShader_VertSrc, lightLocationShader_FragSrc, false);
		forwardShaded_EmissiveModelShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_Emissive_fragSrc, false);
		debugLineShader = new_sp<Shader>(SH::DebugLinesVertSrc, SH::DebugLinesFragSrc, false);

		sp<Player> playerZero = getPlayerSystem().createPlayer<Player>();

		//set up unit cube
		ec(glGenVertexArrays(1, &cubeVAO));
		ec(glBindVertexArray(cubeVAO));
		ec(glGenBuffers(1, &cubeVBO));
		ec(glBindBuffer(GL_ARRAY_BUFFER, cubeVBO));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(Utils::unitCubeVertices_Position_Normal), Utils::unitCubeVertices_Position_Normal, GL_STATIC_DRAW));
		ec(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0)));
		ec(glEnableVertexAttribArray(0));
		ec(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float))));
		ec(glEnableVertexAttribArray(1));
		ec(glBindVertexArray(0));


		//camera
		fpsCamera = new_sp<SA::CameraFPS>(45.f, 0.f, 0.f);
		fpsCamera->registerToWindowCallbacks_v(window);
		fpsCamera->setCursorMode(false);
		playerZero->setCamera(fpsCamera);

		//load models
		laserBoltModel = new_sp<Model3D>("Models/TestModels/SpaceArcade/LazerBolt/LazerBolt.obj");
		loadedModels.insert({laserBoltModelKey, laserBoltModel });

		//this transform should probably be configured within a designer; hard coding reasonable values for now.
		Transform projectileAABBTransform;
		projectileAABBTransform.scale.z = 4.5;
		laserBoltHandle = ProjectileSS->createProjectileType(laserBoltModel, projectileAABBTransform);

		sp<Model3D> fighterModel = new_sp<Model3D>("Models/TestModels/SpaceArcade/Fighter/SGFighter.obj");
		loadedModels.insert({ fighterModelKey, fighterModel});

		sp<Model3D> carrierModel = new_sp<Model3D>("Models/TestModels/SpaceArcade/Carrier/SGCarrier.obj");
		loadedModels.insert({ carrierModelKey, carrierModel });

		GLuint radialGradientTex = 0;
		if (getTextureSubsystem().loadTexture("Textures/SpaceArcade/RadialGradient.png", radialGradientTex))
		{
			//loaded!
		}

		ui_root = new_sp<UIRootWindow>();

		//make sure resources are loaded before the level starts
		sp<Level> startupLevel = new_sp<BasicTestSpaceLevel>();
		getLevelSubsystem().loadLevel(startupLevel);

		return window;
	}

	void SpaceArcade::shutDown() 
	{
		fpsCamera->deregisterToWindowCallbacks_v();

		ec(glDeleteVertexArrays(1, &cubeVAO));
		ec(glDeleteBuffers(1, &cubeVBO));
	}

	sp<SA::Model3D> SpaceArcade::getModel(const std::string& key)
	{
		const auto& iter = loadedModels.find(key);
		if (iter != loadedModels.end())
		{
			return iter->second;
		}
		else
		{
			return nullptr;
		}
	}

	void SpaceArcade::renderDebug(const glm::mat4& view, const glm::mat4& projection)
	{
#ifdef SA_CAPTURE_SPATIAL_HASH_CELLS
		const sp<Level>& world = getLevelSubsystem().getCurrentLevel();
		if (world)
		{
			auto& worldGrid = world->getWorldGrid();
			if (bRenderDebugCells)
			{
				glm::vec3 color{ 0.5f, 0.f, 0.f };
				SpatialHashCellDebugVisualizer::render(worldGrid, view, projection, color);
			}
			//TODO: this is a candidate for a ticker and a ticker subsystem
			SpatialHashCellDebugVisualizer::clearCells(worldGrid);
		}
#endif //SA_CAPTURE_SPATIAL_HASH_CELLS
		if (bRenderProjectileOBBs)
		{
			ProjectileSS->renderProjectileBoundingBoxes(*debugLineShader, glm::vec3(0, 1, 0), view, projection);
		}
	}

	//putting tick below rest class so it will be close to bottom of file.
	void SpaceArcade::tickGameLoop(float deltaTimeSecs) 
	{
		using glm::vec3; using glm::vec4; using glm::mat4;

		const sp<Window>& window = getWindowSubsystem().getPrimaryWindow();
		if (!window)
		{
			startShutdown();
			return;
		}

		//this probably will need to become event based and have handler stack
		updateInput(deltaTimeSecs);

		fpsCamera->handleInput(window->get(), deltaTimeSecs);

		ui_root->tick(deltaTimeSecs);

	}

	void SpaceArcade::renderLoop(float deltaTimeSecs)
	{
		using glm::vec3; using glm::vec4; using glm::mat4;

		const sp<Window>& window = getWindowSubsystem().getPrimaryWindow();
		if (!window)
		{
			return;
		}

		ec(glEnable(GL_DEPTH_TEST));
		ec(glClearColor(0, 0, 0, 1));
		ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

		mat4 view = fpsCamera->getView();
		mat4 projection = glm::perspective(fpsCamera->getFOV(), window->getAspect(), 0.1f, 500.0f);

		renderDebug(view, projection);

		//{ //render cube
		//	mat4 model = glm::mat4(1.f);
		//	model = glm::translate(model, vec3(5.f, 0.f, -5.f));
		//	litObjShader->use();
		//	litObjShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
		//	litObjShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
		//	litObjShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
		//	litObjShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
		//	litObjShader->setUniform3f("cameraPosition", fpsCamera->getPosition());
		//	ec(glBindVertexArray(cubeVAO));
		//	ec(glDrawArrays(GL_TRIANGLES, 0, sizeof(Utils::unitCubeVertices_Position_Normal) / (6 * sizeof(float))));
		//}

		//render world entities
		forwardShaded_EmissiveModelShader->use();
		forwardShaded_EmissiveModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
		forwardShaded_EmissiveModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
		forwardShaded_EmissiveModelShader->setUniform3f("lightColor", glm::vec3(0.8f, 0.8f, 0));
		ProjectileSS->renderProjectiles(*forwardShaded_EmissiveModelShader);

		if (const sp<Level>& loadedLevel = getLevelSubsystem().getCurrentLevel())
		{
			loadedLevel->render(deltaTimeSecs, view, projection);
		}

		getUISubsystem()->render();
	}

	void SpaceArcade::onRegisterCustomSubsystem()
	{
		ProjectileSS = new_sp<ProjectileSubsystem>();
		RegisterCustomSubsystem(ProjectileSS);

		UI_SS = new_sp<UISubsystem>();
		RegisterCustomSubsystem(UI_SS);
	}

	void SpaceArcade::updateInput(float detltaTimeSec)
	{
		if (const sp<Window> windowObj = getWindowSubsystem().getPrimaryWindow())
		{
			GLFWwindow* window = windowObj->get();

			//TODO need a proper input system that has input bubbling/capturing and binding support from player down to entities
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
					fpsCamera->setCursorMode(ui_root->getUIVisible());
				}
			}
			 
			//debug
			if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) 
			{ 
				if(input.isKeyJustPressed(window, GLFW_KEY_C)) { bRenderDebugCells = !bRenderDebugCells; }
				if (input.isKeyJustPressed(window, GLFW_KEY_V)) { bRenderProjectileOBBs = !bRenderProjectileOBBs; }
				if (input.isKeyJustPressed(window, GLFW_KEY_U)) 
				{
					//sp<Level> currentLevel = getLevelSubsystem().getCurrentLevel();
					//getLevelSubsystem().unloadLevel(currentLevel);
					sp<Level> projectileEditor = new_sp<ProjectileEditor_Level>();
					getLevelSubsystem().loadLevel(projectileEditor);
				}
			}
		}
	}
}
namespace
{
	int trueMain()
	{
		SA::SpaceArcade& game = SA::SpaceArcade::get();
		game.start();
		return 0;
	}
}

int main()
{
	return trueMain();
}