#pragma once

#include<iostream>
#include"Utilities.h"
#include"Shader.h"
#include"libraries/stb_image.h"
#include"glm.hpp"
#include"gtc/matrix_transform.hpp"
#include"gtc/type_ptr.hpp"
#include <chrono>
#include <thread>
#include<memory>
#include"Camera.h"
#include "Texture2D.h"

namespace StencilTesting
{
	Camera camera;
	void mouseCallback(GLFWwindow* window, double xpos, double ypos);
	void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

	static const int screenHeight = 600;
	static const int screenWidth = 800;

	int main()
	{
		//INIT OPEN GL
		GLFWwindow* window = Utils::initOpenGl(screenWidth, screenHeight);
		if (!window) return -1;

		//SHADER
		Shader shader2attribs("6.VertShader2.glsl", "6.FragShader2.glsl");
		if (shader2attribs.createFailed()) { glfwTerminate(); return -1; }

		Shader singleColorShader("6.VertShader2.glsl", "SingleColorFrag.glsl");
		if (singleColorShader.createFailed()) { glfwTerminate(); return -1; }

		//VERTEX BUFFER AND VERTEX ARRAY OBJECTS
		GLuint VAO, VBO;
		if (!Utils::generateObject2Attrib(Utils::cubeVertices, sizeof(Utils::cubeVertices), VAO, VBO))
		{
			glfwTerminate(); return -1;
		}

		//TEXTURES
		std::shared_ptr<Texture2D> containerTexture = std::make_shared<Texture2D>("Textures/container.jpg");
		std::shared_ptr<Texture2D> awesomefaceTexture = std::make_shared<Texture2D>("Textures/awesomeface3.png");
		if (!containerTexture->getLoadSuccess() || !awesomefaceTexture->getLoadSuccess())
		{
			glfwTerminate();
			return -1;
		}

		//CONFIGURING SHADER
		shader2attribs.use();
		shader2attribs.addTexture(containerTexture, "texture1");
		shader2attribs.addTexture(awesomefaceTexture, "texture2");
		shader2attribs.setUniform1f("mixRatio", 0.5f);

		singleColorShader.use();
		singleColorShader.addTexture(containerTexture, "texture1");
		singleColorShader.addTexture(awesomefaceTexture, "texture2");
		singleColorShader.setUniform1f("mixRatio", 0.5f);

		glm::vec3 cubePositions[] = {
			glm::vec3(0.00f, 0.0f, 0.0f),
			glm::vec3(2.0f, 2.0f, -10.0f),
			glm::vec3(-5.0f, 5.0f, -20.0f),
			glm::vec3(-2.f,	-4.f, -10.f),
			glm::vec3(2.0f,  5.0f, -15.0f),
			glm::vec3(-1.5f, -2.2f, -2.5f),
			glm::vec3(-3.8f, -2.0f, -12.3f),
			glm::vec3(2.4f, -0.4f, -3.5f),
			glm::vec3(-1.7f,  3.0f, -7.5f),
			glm::vec3(1.3f, -2.0f, -2.5f),
			glm::vec3(1.5f,  2.0f, -2.5f),
			glm::vec3(1.5f,  0.2f, -1.5f),
			glm::vec3(-1.3f,  1.0f, -1.5f)
		};

		//set OpenGL to consider zbuffer in rendering

		float lastFrameTime = static_cast<float>(glfwGetTime());
		float deltaTime = lastFrameTime;

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPosCallback(window, &mouseCallback);
		glfwSetScrollCallback(window, &mouseScrollCallback);
		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;
			camera.pollMovements(window, deltaTime);
			glm::mat4 lookat = camera.getViewMatrix(); //cache here so generation isn't repeated in loop

			glEnable(GL_DEPTH_TEST);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //Notice this was changed to clear depth

			//enable writing to stencil buffer, we're going to use it as a filter to not overwrite objects we've highlighted. 
			glEnable(GL_STENCIL_TEST);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glStencilMask(0xFF);

			//now normal drawing will update the stencil buffer with 1.
			//we should only draw what we want highlighted at this point, because any renders will update the stencil buffer. 

			shader2attribs.use();
			shader2attribs.activateTextures();

			size_t numElementsInCubeArray = sizeof(cubePositions) / (3 * sizeof(float));
			for (size_t i = 0; i < numElementsInCubeArray; ++i)
			{
				glm::mat4 model;
				glm::mat4 view;
				glm::mat4 projection;

				model = glm::translate(model, cubePositions[i]);
				model = glm::rotate(model, static_cast<float>(glm::radians(50.0f) * i), glm::vec3(0.5 * i, 0.5f * i, 0.5f * i));
				view = lookat;// camera.getViewMatrix() = lookat;
				projection = glm::perspective(glm::radians(camera.getFOV()), static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.f);

				shader2attribs.use();
				GLuint modelIndex = glGetUniformLocation(shader2attribs.getId(), "model");
				GLuint viewIndex = glGetUniformLocation(shader2attribs.getId(), "view");
				GLuint projectionIndex = glGetUniformLocation(shader2attribs.getId(), "projection");

				glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(model));
				glUniformMatrix4fv(viewIndex, 1, GL_FALSE, glm::value_ptr(view));
				glUniformMatrix4fv(projectionIndex, 1, GL_FALSE, glm::value_ptr(projection));

				//DRAW
				glBindVertexArray(VAO);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}

			//------------------------ DRAW HIGHLIGHTS -------------------------------
			glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
			glStencilMask(0x00); //don't update (write to) stencil buffer
			glDisable(GL_DEPTH_TEST);
			singleColorShader.use();

			for (size_t i = 0; i < numElementsInCubeArray; ++i)
			{
				glm::mat4 model;
				glm::mat4 view;
				glm::mat4 projection;

				model = glm::translate(model, cubePositions[i]);
				model = glm::rotate(model, static_cast<float>(glm::radians(50.0f) * i), glm::vec3(0.5 * i, 0.5f * i, 0.5f * i));
				float scaleAmount = 1.25f;
				model = glm::scale(model, glm::vec3(scaleAmount, scaleAmount, scaleAmount));
				view = lookat;// camera.getViewMatrix() = lookat;
				projection = glm::perspective(glm::radians(camera.getFOV()), static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.f);

				GLuint modelIndex = glGetUniformLocation(singleColorShader.getId(), "model");
				GLuint viewIndex = glGetUniformLocation(singleColorShader.getId(), "view");
				GLuint projectionIndex = glGetUniformLocation(singleColorShader.getId(), "projection");

				glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(model));
				glUniformMatrix4fv(viewIndex, 1, GL_FALSE, glm::value_ptr(view));
				glUniformMatrix4fv(projectionIndex, 1, GL_FALSE, glm::value_ptr(projection));

				//DRAW
				glBindVertexArray(VAO);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
			glEnable(GL_DEPTH_TEST);
			glStencilMask(0xFF); // !!! -------- MUST RE-ENABLE A MASK THAT WILL ALLOW CLEARING ---- !!!


			glfwPollEvents();
			Utils::setWindowCloseOnEscape(window);
			glfwSwapBuffers(window);

			static long delayMsFor60FPS = static_cast<long>(1 / 60.f * 1000);
			std::this_thread::sleep_for(std::chrono::milliseconds(delayMsFor60FPS));
		}

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);

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
}

int main() {
	return StencilTesting::main();
}