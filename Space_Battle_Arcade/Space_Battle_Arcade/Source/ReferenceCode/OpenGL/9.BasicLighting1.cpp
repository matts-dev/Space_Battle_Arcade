#pragma once

#include<iostream>
#include"Utilities.h"
#include"Shader.h"
#include"libraries/stb_image.h"
#include"glm/glm.hpp"
#include"glm/gtc/matrix_transform.hpp"
#include"glm/gtc/type_ptr.hpp"
#include <chrono>
#include <thread>
#include<memory>
#include"Camera.h"
#include "Texture2D.h"

namespace BasicLighting
{
	Camera camera;
	void mouseCallback(GLFWwindow* window, double xpos, double ypos);
	void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
	void pollKeybinds(GLFWwindow* window, float deltaTime);

	static const int screenHeight = 600;
	static const int screenWidth = 800;
	static glm::vec3 lightPos(2.0f, 2.f, 1.f);

	int main()
	{
		//INIT OPEN GL
		GLFWwindow* window = Utils::initOpenGl(screenWidth, screenHeight);
		if (!window) return -1;

		//SHADER
		Shader shaderForCubes("8.ColorTutVertShader1.glsl", "8.ColorTutFragShader1.glsl");
		if (shaderForCubes.createFailed()) { glfwTerminate(); return -1; }

		Shader shaderForLightObject("8.ColorTutVertShader1.glsl", "8.LightSourceFragShader.glsl");
		if (shaderForLightObject.createFailed()) { glfwTerminate(); return -1; }

		Shader ambientLightShader("8.ColorTutVertShader1.glsl", "9.AmbientLight.fs");
		if (ambientLightShader.createFailed()) { glfwTerminate(); return -1; }

		Shader diffuseWithAmbientShader("9.BasicLight.vs", "9.DiffuseAmbient.fs");
		if (diffuseWithAmbientShader.createFailed()) { glfwTerminate(); return -1; }

		Shader phongShader("9.BasicLight.vs", "9.PhongShader_SpecularDiffuseAmbient.fs");
		if (phongShader.createFailed()) { glfwTerminate(); return -1; }

		//VERTEX BUFFER AND VERTEX ARRAY OBJECTS
		//Lamp Object
		GLuint LampVAO, LampVBO;
		glGenVertexArrays(1, &LampVAO);
		glBindVertexArray(LampVAO);
		glGenBuffers(1, &LampVBO);
		glBindBuffer(GL_ARRAY_BUFFER, LampVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Utils::cubeVerticesWithNormals), &Utils::cubeVerticesWithNormals, GL_STATIC_DRAW);
		//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<GLvoid*>(0)); //cube position contains 2 texture coordinates, thus stride of 5
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<GLvoid*>(0)); //cube with normals take up 6 floats per stride
		glEnableVertexAttribArray(0);
		//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<GLvoid*>(3*sizeof(float))); //enable last 3 in stride to be normals
		//glEnableVertexAttribArray(1); //enable normals, this wasn't in tutorial but I think it is needed.
		glBindVertexArray(0); //stop saving state

		//Non-light emiting objects
		GLuint CubeVAO, CubeVBO;
		glGenVertexArrays(1, &CubeVAO);
		glGenBuffers(1, &CubeVBO);
		glBindVertexArray(CubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, CubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Utils::cubeVerticesWithNormals), &Utils::cubeVerticesWithNormals, GL_STATIC_DRAW);
		//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<GLvoid*>(0)); //cube position contains 2 texture coordinates, thus stride of 5
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<GLvoid*>(0)); //cube with normals take up 6 floats per stride
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<GLvoid*>(3 * sizeof(float))); //enable last 3 in stride to be normals
		glEnableVertexAttribArray(1); //enable normals, this wasn't in tutorial but I think it is needed.
		glBindVertexArray(0); //stop saving state to this VAO



							  //TEXTURES
							  //std::shared_ptr<Texture2D> containerTexture = std::make_shared<Texture2D>("Textures/container.jpg");
							  //std::shared_ptr<Texture2D> awesomefaceTexture = std::make_shared<Texture2D>("Textures/awesomeface3.png");
							  //if (!containerTexture->getLoadSuccess() || !awesomefaceTexture->getLoadSuccess())
							  //{
							  //	glfwTerminate();
							  //	return -1;
							  //}

							  //CONFIGURING SHADER
		shaderForCubes.use();
		//shader2attribs.addTexture(containerTexture, "texture1");
		//shader2attribs.addTexture(awesomefaceTexture, "texture2");
		//shader2attribs.setUniform1f("mixRatio", 0.5f);

		glm::vec3 cubePositions[] = {
			glm::vec3(0.0f, 0.0f, 0.0f),
			//glm::vec3(2.0f, 2.0f, -10.0f),
			//glm::vec3(-5.0f, 5.0f, -20.0f),
			//glm::vec3(-2.f,	-4.f, -10.f),
			//glm::vec3(2.0f,  5.0f, -15.0f),
			//glm::vec3(-1.5f, -2.2f, -2.5f),
			//glm::vec3(-3.8f, -2.0f, -12.3f),
			//glm::vec3(2.4f, -0.4f, -3.5f),
			//glm::vec3(-1.7f,  3.0f, -7.5f),
			//glm::vec3(1.3f, -2.0f, -2.5f),
			//glm::vec3(1.5f,  2.0f, -2.5f),
			//glm::vec3(1.5f,  0.2f, -1.5f),
			glm::vec3(-1.3f,  1.0f, -1.5f)
		};

		//set OpenGL to consider zbuffer in rendering
		glEnable(GL_DEPTH_TEST);

		float lastFrameTime = static_cast<float>(glfwGetTime());
		float deltaTime = lastFrameTime;
		glm::vec3 lightColor(1.f, 1.f, 1.f);

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPosCallback(window, &mouseCallback);
		glfwSetScrollCallback(window, &mouseScrollCallback);
		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;
			camera.pollMovements(window, deltaTime);
			pollKeybinds(window, deltaTime);
			glm::mat4 lookat = camera.getViewMatrix(); //cache here so generation isn't repeated in loop

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Notice this was changed to clear depth

			//Shader& shaderForCubes = ambientLightShader;
			//Shader& shaderForCubes = diffuseWithAmbientShader;
			Shader& shaderForCubes = phongShader;

			shaderForCubes.use();
			shaderForCubes.activateTextures();
			shaderForCubes.setUniform3f("objectColor", 1.f, 0.5f, 0.f);
			shaderForCubes.setUniform3f("lightColor", lightColor.r, lightColor.g, lightColor.b);
			//if (&shaderForCubes == &diffuseWithAmbientShader){
			shaderForCubes.setUniform3f("lightPosInWorldSpace", lightPos.x, lightPos.y, lightPos.z);
			//}

			glm::vec3 camPos = camera.getCameraPositionCopy();
			shaderForCubes.setUniform3f("viewerPos", camPos.x, camPos.y, camPos.z);
			

			size_t numElementsInCubeArray = sizeof(cubePositions) / (3 * sizeof(float));
			for (size_t i = 0; i < numElementsInCubeArray; ++i)
			{
				glm::mat4 model;
				glm::mat4 view;
				glm::mat4 projection;

				model = glm::translate(model, cubePositions[i]);
				if (i != 0)
				{
					//calling rotate on a vector of 0 results in NAN because nested in the function is a divide by the normalized vector (which will be zero!)
					model = glm::rotate(model, static_cast<float>(glm::radians(50.0f) * i), glm::vec3(0.5 * i, 0.5f * i, 0.5f * i));
				}
				view = camera.getViewMatrix() = lookat; //shouldn't be in loop, but leaving for complete understanding
				projection = glm::perspective(glm::radians(camera.getFOV()), static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.f); //shouldn't be in loop, but leaving for complete understanding

				shaderForCubes.use();
				GLuint modelIndex = glGetUniformLocation(shaderForCubes.getId(), "model");
				GLuint viewIndex = glGetUniformLocation(shaderForCubes.getId(), "view"); //shouldn't be in loop, but leaving for complete understanding
				GLuint projectionIndex = glGetUniformLocation(shaderForCubes.getId(), "projection"); //shouldn't be in loop, but leaving for complete understanding

				glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(model));
				glUniformMatrix4fv(viewIndex, 1, GL_FALSE, glm::value_ptr(view));
				glUniformMatrix4fv(projectionIndex, 1, GL_FALSE, glm::value_ptr(projection));

				//DRAW
				glBindVertexArray(CubeVAO);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}

			shaderForLightObject.use();
			shaderForLightObject.setUniform3f("lightColor", lightColor.r, lightColor.g, lightColor.b);
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 projection;
			model = glm::translate(model, lightPos);
			model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
			view = camera.getViewMatrix() = lookat;
			projection = glm::perspective(glm::radians(camera.getFOV()), static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.f);
			GLuint modelIndex = glGetUniformLocation(shaderForLightObject.getId(), "model");
			GLuint viewIndex = glGetUniformLocation(shaderForLightObject.getId(), "view");
			GLuint projectionIndex = glGetUniformLocation(shaderForLightObject.getId(), "projection");
			glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(viewIndex, 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(projectionIndex, 1, GL_FALSE, glm::value_ptr(projection));

			glBindVertexArray(LampVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);

			glfwPollEvents();
			Utils::setWindowCloseOnEscape(window);
			glfwSwapBuffers(window);

			static long delayMsFor60FPS = static_cast<long>(1 / 60.f * 1000);
			std::this_thread::sleep_for(std::chrono::milliseconds(delayMsFor60FPS));
		}

		//clean up resources
		glDeleteVertexArrays(1, &LampVAO);
		glDeleteBuffers(1, &LampVBO);
		glDeleteBuffers(1, &CubeVAO);
		glDeleteBuffers(1, &CubeVBO);
		return 0;
	}

	void mouseCallback(GLFWwindow* window, double xpos, double ypos)
	{
		camera.updateRotation(static_cast<float>(xpos), static_cast<float>(ypos));
	}


	void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
	{
		camera.incrementFOV(static_cast<float>(yOffset));
	}

	void pollKeybinds(GLFWwindow* window, float deltaTime){
		static float movementSpeed = 10.0f;

		float adjustedMovementSpeed = movementSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		{
			lightPos.z -= adjustedMovementSpeed;
		}
		else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		{
			lightPos.z += adjustedMovementSpeed;
		}
		else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		{
			lightPos.x -= adjustedMovementSpeed;
		} 
		else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		{
			lightPos.x += adjustedMovementSpeed;
		}
	}
}

//int main()
//{
//	BasicLighting::main();
//}