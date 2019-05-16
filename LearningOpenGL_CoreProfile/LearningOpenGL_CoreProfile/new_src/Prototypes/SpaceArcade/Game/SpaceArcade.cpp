#include "SpaceArcade.h"

#include "..\Rendering\SAWindow.h"
#include "..\Rendering\OpenGLHelpers.h"
#include "..\Rendering\Camera\SACameraFPS.h"
#include "..\Rendering\SAShader.h"
#include "..\GameFramework\SAGameBase.h"
#include "..\GameFramework\SAWindowSubsystem.h"
#include "..\Tools\SAUtilities.h"
#include "..\Tools\ModelLoading\SAModel.h"

#include "..\Rendering\BuiltInShaders.h"
#include "SAShip.h"
#include <random>
#include "SACollisionSubsystem.h"
#include "..\GameFramework\Input\SAInput.h"
#include <assert.h>
#include "..\GameFramework\SATextureSubsystem.h"
#include "SAProjectileSubsystem.h"

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
		forwardShadedModelShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_SimpleLighting_fragSrc, false);
		forwardShaded_EmissiveModelShader = new_sp<SA::Shader>(forwardShadedModel_SimpleLighting_vertSrc, forwardShadedModel_Emissive_fragSrc, false);

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
		fpsCamera->registerToWindowCallbacks(window);
		glfwSetInputMode(window->get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		//load models
		lazerBoltModel = new_sp<Model3D>("Models/TestModels/SpaceArcade/LazerBolt/LazerBolt.obj");
		loadedModels.push_back(lazerBoltModel); 

		Transform projectileAABBTransform;
		lazerBoltHandle = ProjectileSS->createProjectileType(lazerBoltModel, projectileAABBTransform);

		sp<Model3D> fighterModel = new_sp<Model3D>("Models/TestModels/SpaceArcade/Fighter/SGFighter.obj");
		loadedModels.push_back(fighterModel);

		sp<Model3D> carrierModel = new_sp<Model3D>("Models/TestModels/SpaceArcade/Carrier/SGCarrier.obj");
		loadedModels.push_back(carrierModel);

		Transform carrierTransform;
		carrierTransform.position = { 200,0,0 };
		carrierTransform.scale = { 5, 5, 5 };
		carrierTransform.rotQuat = glm::angleAxis(glm::radians(-33.0f), glm::vec3(0, 1, 0));
		sp<Ship> carrierShip1 = spawnEntity<Ship>(carrierModel, carrierTransform, createUnitCubeCollisionInfo());

		std::random_device rng;
		std::seed_seq seed{ 28 };
		std::mt19937 rng_eng = std::mt19937(seed);
		std::uniform_real_distribution<float> startDist(-200.f, 200.f); //[a, b)
		
		int numFighterShipsToSpawn = 5000; 
#ifdef _DEBUG
		numFighterShipsToSpawn = 500;
#endif//NDEBUG 
		for (int fighterShip = 0; fighterShip < numFighterShipsToSpawn; ++fighterShip)
		{
			glm::vec3 startPos(startDist(rng_eng), startDist(rng_eng), startDist(rng_eng));
			glm::quat rot = glm::angleAxis(startDist(rng_eng), glm::vec3(0, 1, 0)); //angle is a little addhoc, but with radians it should cover full 360 possibilities
			startPos += carrierTransform.position;
			Transform fighterXform = Transform{ startPos, rot, {0.1,0.1,0.1} };
			spawnEntity<Ship>(fighterModel, fighterXform, createUnitCubeCollisionInfo());
		}

		carrierTransform.position.y += 50;
		carrierTransform.position.x += 120;
		carrierTransform.position.z -= 50;
		carrierTransform.rotQuat = glm::angleAxis(glm::radians(-13.0f), glm::vec3(0, 1, 0));
		sp<Ship> carrierShip2 = spawnEntity<Ship>(carrierModel, carrierTransform, createUnitCubeCollisionInfo());

		GLuint radialGradientTex = 0;
		if (getTextureSubsystem().loadTexture("Textures/SpaceArcade/RadialGradient.png", radialGradientTex))
		{
			//loaded!
		}
		

		return window;
	}

	void SpaceArcade::shutDown() 
	{
		fpsCamera->deregisterToWindowCallbacks();

		ec(glDeleteVertexArrays(1, &cubeVAO));
		ec(glDeleteBuffers(1, &cubeVBO));
	}

	void SpaceArcade::renderDebug(const glm::mat4& view,  const glm::mat4& projection)
	{
#ifdef SA_CAPTURE_SPATIAL_HASH_CELLS
		auto& worldGrid = getCollisionSS()->getWorldGrid();
		if (bRenderDebugCells)
		{
			glm::vec3 color{ 0.5f, 0.f, 0.f };
			SpatialHashCellDebugVisualizer::render(worldGrid, view, projection, color);
		}
		//TODO: this is a candidate for a ticker and a ticker subsystem
		SpatialHashCellDebugVisualizer::clearCells(worldGrid);
#endif //SA_CAPTURE_SPATIAL_HASH_CELLS

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

		//RENDER
		ec(glEnable(GL_DEPTH_TEST));
		ec(glClearColor(0, 0, 0, 1));
		ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

		for (const sp<WorldEntity>& entity : worldEntities)
		{
			entity->tick(deltaTimeSecs);
		}

	}

	void SpaceArcade::renderLoop(float deltaTimeSecs)
	{
		using glm::vec3; using glm::vec4; using glm::mat4;

		const sp<Window>& window = getWindowSubsystem().getPrimaryWindow();
		if (!window)
		{
			return;
		}

		mat4 view = fpsCamera->getView();
		mat4 projection = glm::perspective(fpsCamera->getFOV(), window->getAspect(), 0.1f, 500.0f);

		renderDebug(view, projection);


		{ //render cube
			mat4 model = glm::mat4(1.f);
			model = glm::translate(model, vec3(5.f, 0.f, -5.f));
			litObjShader->use();
			litObjShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			litObjShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			litObjShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			litObjShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
			litObjShader->setUniform3f("cameraPosition", fpsCamera->getPosition());
			ec(glBindVertexArray(cubeVAO));
			ec(glDrawArrays(GL_TRIANGLES, 0, sizeof(Utils::unitCubeVertices_Position_Normal) / (6 * sizeof(float))));
		}

		//render world entities
		forwardShadedModelShader->use();
		forwardShadedModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
		forwardShadedModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
		forwardShadedModelShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
		forwardShadedModelShader->setUniform3f("cameraPosition", fpsCamera->getPosition());
		forwardShadedModelShader->setUniform1i("material.shininess", 32);
		for (const sp<RenderModelEntity>& entity : renderEntities)
		{
			mat4 model = glm::mat4(1.f);
			model = glm::translate(model, vec3(5.f, 0.f, -5.f));
			forwardShadedModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(entity->getTransform().getModelMatrix()));
			entity->getModel()->draw(*forwardShadedModelShader);
		}

		forwardShaded_EmissiveModelShader->use();
		forwardShaded_EmissiveModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
		forwardShaded_EmissiveModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
		forwardShaded_EmissiveModelShader->setUniform3f("lightColor", glm::vec3(0.8f, 0.8f, 0));
		ProjectileSS->renderProjectiles(*forwardShaded_EmissiveModelShader);

		//DEBUG render lazer bolt
		{
			mat4 model = glm::mat4(1.f);
			model = glm::translate(model, vec3(-0.f, 0.f, -0.f));
			forwardShadedModelShader->use();
			forwardShadedModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			lazerBoltModel->draw(*forwardShadedModelShader);
		}

	}

	void SpaceArcade::onRegisterCustomSubsystem()
	{
		CollisionSS = new_sp<CollisionSubsystem>();
		RegisterCustomSubsystem(CollisionSS);

		ProjectileSS = new_sp<ProjectileSubsystem>();
		RegisterCustomSubsystem(ProjectileSS);
	}

	void SpaceArcade::updateInput(float detltaTimeSec)
	{
		if (const sp<Window> windowObj = getWindowSubsystem().getPrimaryWindow())
		{
			GLFWwindow* window = windowObj->get();
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			{
				startShutdown();
			}

			//TODO probably should create a system that has input bubbling/capturing and binding support from player down to entities
			static InputTracker input;
			input.updateState(window);

			if (input.isMouseButtonJustPressed(window, GLFW_MOUSE_BUTTON_LEFT))
			{
				//TODO this ideally will go something like:
				//playerShip = get player controlled ship
				//playerShip.fireProjectile
				glm::vec3 start = fpsCamera->getPosition() + glm::vec3(0, -0.25f, 0);
				glm::vec3 direction = fpsCamera->getFront();
				ProjectileSS->spawnProjectile(start, direction, *lazerBoltHandle);
			}
			 
			//debug
			if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) 
			{ 
				if(input.isKeyJustPressed(window, GLFW_KEY_C)) { bRenderDebugCells = !bRenderDebugCells; }
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