#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include<cmath>

#include <gtx/quaternion.hpp>
#include <tuple>
#include <array>
#include <functional>

#include "../../Tools/SADemoInterface.h"
#include "../../Rendering/SAShader.h"
#include "../../Tools/SAUtilities.h"

#include "../../Rendering/Camera/SACameraFPS.h"
#include "../../../../Algorithms/SpatialHashing/SpatialHashingComponent.h"
#include "../../../../Algorithms/SpatialHashing/SHDebugUtils.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../SAPlayer.h"
#include "../SAShip.h"
#include "../../GameFramework/Input/SAInput.h"
namespace
{
	const char* litObjectShaderVertSrc = R"(
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
	const char* litObjectShaderFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor;
				uniform vec3 objectColor;
				uniform vec3 lightPosition;
				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;

				/* uniforms can have initial value in GLSL 1.20 and up */
				uniform float ambientStrength = 0.1f; 
				uniform vec3 objColor = vec3(1,1,1);

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
	const char* lightLocationShaderVertSrc = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* lightLocationShaderFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor;
				
				void main(){
					fragmentColor = vec4(lightColor, 1.f);
				}
			)";


	   
	////////////////////////////////////////////////////////////////////////////////////////////
	// class used to hold state of demo
	////////////////////////////////////////////////////////////////////////////////////////////
	class PlayerControlTest final : public SA::DemoInterface
	{
		//Cached Window Data
		int height;
		int width;
		float lastFrameTime = 0.f;
		float deltaTime = 0.0f;

		//OpenGL data
		GLuint cubeVAO, cubeVBO;

		//State
		SA::Player player;
		SA::Ship playersShip;

		SA::CameraFPS camera{ 45.0f, -90.f, 0.f };
		SA::Transform lightTransform;
		glm::vec3 lightColor{ 1.0f, 1.0f, 1.0f };
		SA::Shader objShader{ litObjectShaderVertSrc , litObjectShaderFragSrc, false };
		SA::Shader lampShader{ lightLocationShaderVertSrc, lightLocationShaderFragSrc, false };
		SA::Shader debugGridShader{ SH::DebugLinesVertSrc, SH::DebugLinesFragSrc, false };

	public:
		PlayerControlTest(int width, int height)
			: SA::DemoInterface(width, height)
		{
			using glm::vec2;
			using glm::vec3;
			using glm::mat4;

			camera.setPosition(0.0f, 0.0f, 3.0f);
			this->width = width;
			this->height = height;

			camera.setPosition(-7.29687f, 3.22111f, 5.0681f);
			camera.setYaw(-22.65f);
			camera.setPitch(-15.15f);

			/////////////////////////////////////////////////////////////////////
			// OpenGL/Collision Object Creation
			/////////////////////////////////////////////////////////////////////

			//unit create cube (that matches the size of the collision cube)
			float vertices[] = {
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
			glGenVertexArrays(1, &cubeVAO);
			glBindVertexArray(cubeVAO);

			glGenBuffers(1, &cubeVBO);
			glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
			glBindVertexArray(0);

			lightTransform.position = glm::vec3(5, 3, 0);
			lightTransform.scale = glm::vec3(0.1f);

			////////////////////////////////////////////////////////////
			// initialization state
			////////////////////////////////////////////////////////////
		}

		~PlayerControlTest()
		{
			glDeleteVertexArrays(1, &cubeVAO);
			glDeleteBuffers(1, &cubeVBO);
		}

		virtual void handleModuleFocused(GLFWwindow* window)
		{
			camera.registerToWindowCallbacks(window);
		}

		virtual void tickGameLoop(GLFWwindow* window)
		{
			using glm::vec2; using glm::vec3; using glm::mat4;

			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			processInput(window);

			mat4 view = camera.getView();
			mat4 projection = glm::perspective(glm::radians(camera.getFOV()), static_cast<float>(width) / height, 0.1f, 100.0f);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(cubeVAO);
			{//render light
				mat4 model = lightTransform.getModelMatrix();
				lampShader.use();
				lampShader.setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
				lampShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));  //since we don't update for each cube, it would be more efficient to do this outside of the loop.
				lampShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
				lampShader.setUniform3f("lightColor", lightColor);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}

			objShader.use();
			objShader.setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(view));
			objShader.setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
			objShader.setUniform3f("lightPosition", lightTransform.position);
			objShader.setUniform3f("lightColor", lightColor);
			objShader.setUniform3f("cameraPosition", camera.getPosition());

		}

	private:

		inline bool isZeroVector(const glm::vec3& testVec)
		{
			constexpr float EPSILON = 0.001f;
			return glm::length2(testVec) < EPSILON * EPSILON;
		}

		void processInput(GLFWwindow* window)
		{
			using glm::vec3; using glm::vec4;
			static SA::InputTracker input;
			input.updateState(window);

			if (input.isKeyJustPressed(window, GLFW_KEY_ESCAPE))
			{
				glfwSetWindowShouldClose(window, true);
			}
			if (input.isKeyJustPressed(window, GLFW_KEY_L))
			{
				//profiler entry point
				int profiler_helper= 5;
				++profiler_helper;
			}
		}
	};

	void true_main()
	{
		using glm::vec2;
		using glm::vec3;
		using glm::mat4;

		int width = 1200;
		int height = 800;

		GLFWwindow* window = SA::Utils::initWindow(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		PlayerControlTest demo(width, height);
		demo.handleModuleFocused(window);

		/////////////////////////////////////////////////////////////////////
		// Game Loop
		/////////////////////////////////////////////////////////////////////
		while (!glfwWindowShouldClose(window))
		{
			demo.tickGameLoop(window);
			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		glfwTerminate();
	}
}

//int main()
//{
//	true_main();
//}