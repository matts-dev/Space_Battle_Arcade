#include "..\..\Rendering\SAWindow.h"
#include "..\..\Rendering\OpenGLHelpers.h"
#include "..\SAGameBase.h"
#include "..\SAWindowSystem.h"
#include "..\..\Rendering\Camera\SACameraFPS.h"
#include "..\..\Rendering\SAShader.h"
#include "..\SAGameEntity.h"

namespace
{
	using namespace SA;
	const char* const litObjectShaderVertSrc = R"(
				#version 330 core
				layout (location = 0) in vec3 position;			
				layout (location = 1) in vec3 normal;	
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				out vec3 fragNormal;
				out vec3 fragPosition;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					fragPosition = vec3(model * vec4(position, 1));

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					fragNormal = normalize(mat3(transpose(inverse(model))) * normal);
				}
			)";
	const char* const litObjectShaderFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor = vec3(1,1,1);
				uniform vec3 objectColor = vec3(1,1,1);
				uniform float ambientStrength = 0.1f; 
				uniform vec3 lightPosition;
				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;

				void main(){
					vec3 ambientLight = ambientStrength * lightColor;					

					vec3 toLight = normalize(lightPosition - fragPosition);
					vec3 normal = normalize(fragNormal);									//interpolation of different normalize will cause loss of unit length
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * lightColor;

					float specularStrength = 0.5f;
					vec3 toView = normalize(cameraPosition - fragPosition);
					vec3 toReflection = reflect(-toView, normal);							//reflect expects vector from light position
					float specularAmount = pow(max(dot(toReflection, toLight), 0), 32);
					vec3 specularLight = specularStrength * lightColor * specularAmount;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * objectColor;
					
					fragmentColor = vec4(lightContribution, 1.0f);
				}
			)";
	const char* const lightLocationShaderVertSrc = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* const lightLocationShaderFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor;
				
				void main(){
					fragmentColor = vec4(lightColor, 1.f);
				}
			)";

	//unit create cube (that matches the size of the collision cube)
	float unitCubeVertices[] = {
		//x     y       z      _____normal_xyz___
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};


	class ProtoGame : public SA::GameBase
	{
		
	public:

		static ProtoGame& get() 
		{
			static sp<ProtoGame> singleton = new_sp<ProtoGame>();
			return *singleton;
		}
	private:
		virtual sp<SA::Window> startUp() override
		{
			using namespace SA;

			int width = 1000, height = 500;
			sp<SA::Window> window = new_sp<SA::Window>(width, height);
			ec(glViewport(0,0, width, height)); //#TODO, should we do this in the gamebase level on "glfwSetFramebufferSizeCallback" changed?

			litObjShader = new_sp<SA::Shader>(litObjectShaderVertSrc, litObjectShaderFragSrc, false);
			lampObjShader = new_sp<SA::Shader>(lightLocationShaderVertSrc, lightLocationShaderFragSrc, false);

			//set up unit cube
			ec(glGenVertexArrays(1, &cubeVAO));
			ec(glBindVertexArray(cubeVAO));
			ec(glGenBuffers(1, &cubeVBO));
			ec(glBindBuffer(GL_ARRAY_BUFFER, cubeVBO));
			ec(glBufferData(GL_ARRAY_BUFFER, sizeof(unitCubeVertices), unitCubeVertices, GL_STATIC_DRAW));
			ec(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0)));
			ec(glEnableVertexAttribArray(0));
			ec(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float))));
			ec(glEnableVertexAttribArray(1));
			ec(glBindVertexArray(0));

			fpsCamera = new_sp<SA::CameraFPS>(45.f, 0.f, 0.f);
			fpsCamera->registerToWindowCallbacks_v(window);

			glfwSetInputMode(window->get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			return window;
		}

		virtual void shutDown() override
		{
			fpsCamera->deregisterToWindowCallbacks_v();

			ec(glDeleteVertexArrays(1, &cubeVAO));
			ec(glDeleteBuffers(1, &cubeVBO));
		}

		sp<SA::CameraFPS> fpsCamera;
		sp<SA::Shader> litObjShader;
		sp<SA::Shader> lampObjShader;

		//unit cube data
		GLuint cubeVAO, cubeVBO;

		//putting tick below rest class so it will be close to bottom of file.
		virtual void tickGameLoop(float deltaTimeSecs) override
		{
			using namespace SA;
			using glm::vec3; using glm::vec4; using glm::mat4; 

			const sp<Window>& window = getWindowSystem().getPrimaryWindow();
			if (!window)
			{
				startShutdown();
				return;
			}

			if (glfwGetKey(window->get(), GLFW_KEY_1) == GLFW_PRESS)
			{
				window->markWindowForClose(true);
			}

			fpsCamera->handleInput(window->get(), deltaTimeSecs);

			//RENDER
			ec(glEnable(GL_DEPTH_TEST));
			ec(glClearColor(0, 0, 0, 1));
			ec(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

			litObjShader->use();

			mat4 model = glm::mat4(1.f);
			model = glm::translate(model, vec3(5.f, 0.f, -5.f));
			mat4 view = fpsCamera->getView();
			mat4 projection = glm::perspective(fpsCamera->getFOV(), window->getAspect(), 0.1f, 300.0f);
			
			litObjShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
			litObjShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			litObjShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			litObjShader->setUniform3f("lightPosition", glm::vec3(0, 0, 0));
			litObjShader->setUniform3f("cameraPosition", fpsCamera->getPosition());

			ec(glBindVertexArray(cubeVAO));
			ec(glDrawArrays(GL_TRIANGLES, 0, sizeof(unitCubeVertices) / (6 * sizeof(float))));
		}

		//adding this because changing interface of game, still letting prototype do its rendering within the game loop
		virtual void renderLoop(float deltaTimeSecs) override {}
	};


	int trueMain()
	{
		ProtoGame& game = ProtoGame::get();
		game.start();
		return 0;
	}
}

//int main()
//{
//	return trueMain();
//}