#include "SpaceArcade.h"

#include "..\Rendering\SAWindow.h"
#include "..\Tools\SmartPointerAlias.h"
#include "..\Rendering\OpenGLHelpers.h"
#include "..\Rendering\Camera\SACameraFPS.h"
#include "..\Rendering\SAShader.h"
#include "..\GameFramework\SAGameBase.h"
#include "..\GameFramework\SAWindowSubsystem.h"
#include "..\Tools\SAUtilities.h"
#include "..\Tools\ModelLoading\SAModel.h"

#include "..\Rendering\BuiltInShaders.h"


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
		sp<Model3D> fighterModel = new_sp<Model3D>("Models/TestModels/SpaceArcade/Fighter/SGFighter.obj");
		loadedModels.push_back(fighterModel);

		sp<Model3D> carrierModel = new_sp<Model3D>("Models/TestModels/SpaceArcade/Carrier/SGCarrier.obj");
		loadedModels.push_back(carrierModel);

		return window;
	}

	void SpaceArcade::shutDown() 
	{
		fpsCamera->deregisterToWindowCallbacks();

		ec(glDeleteVertexArrays(1, &cubeVAO));
		ec(glDeleteBuffers(1, &cubeVBO));
	}

	//putting tick below rest class so it will be close to bottom of file.
	void SpaceArcade::tickGameLoopDerived(float deltaTimeSecs) 
	{
		using namespace SA;
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


		mat4 view = fpsCamera->getView();
		mat4 projection = glm::perspective(fpsCamera->getFOV(), window->getAspect(), 0.1f, 300.0f);
		 
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

		forwardShadedModelShader->use();

		{//render fighter model
			mat4 model = glm::mat4(1.f);
			model = glm::translate(model, vec3(5.f, 0.f, -5.f));
			forwardShadedModelShader->use();
			forwardShadedModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			forwardShadedModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			forwardShadedModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			forwardShadedModelShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("cameraPosition", fpsCamera->getPosition());
			forwardShadedModelShader->setUniform1i("material.shininess", 32);
			loadedModels[0]->draw(*forwardShadedModelShader);
		}

		{//render fighter carrier model
			mat4 model = glm::mat4(1.f);
			model = glm::translate(model, vec3(-10.f, 0.f, -10.f));
			forwardShadedModelShader->use();
			forwardShadedModelShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			forwardShadedModelShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			forwardShadedModelShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			forwardShadedModelShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
			forwardShadedModelShader->setUniform3f("cameraPosition", fpsCamera->getPosition());
			forwardShadedModelShader->setUniform1i("material.shininess", 32);
			loadedModels[1]->draw(*forwardShadedModelShader);
		}

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