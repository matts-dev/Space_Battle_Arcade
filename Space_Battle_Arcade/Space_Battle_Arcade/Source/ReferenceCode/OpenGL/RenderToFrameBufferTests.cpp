

#include<iostream>
#include"Utilities.h"
#include"Shader.h"
#include "Libraries/stb_image.h"
#include"glm/glm.hpp"
#include"glm/gtc/matrix_transform.hpp"
#include"glm/gtc/type_ptr.hpp"
#include <chrono>
#include <thread>
#include<memory>
#include"Camera.h"
#include "Texture2D.h"

namespace RenderToFrameBuffer
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

		Shader fbTextureShader_writesColorOut("6.VertShader2.glsl", "FrameBufferFrag.glsl");
		if (fbTextureShader_writesColorOut.createFailed()) { glfwTerminate(); return -1; }

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

		fbTextureShader_writesColorOut.use();
		fbTextureShader_writesColorOut.addTexture(containerTexture, "texture1");
		fbTextureShader_writesColorOut.addTexture(awesomefaceTexture, "texture2");
		fbTextureShader_writesColorOut.setUniform1f("mixRatio", 0.5f);

		glm::vec3 cubePositions[] = {
			glm::vec3(0.00f, 0.0f, 0.0f),
			glm::vec3(2.0f, 2.0f, -10.0f),
			glm::vec3(-5.0f, 5.0f, -20.0f),
		};

		//set OpenGL to consider zbuffer in rendering
		glEnable(GL_DEPTH_TEST);

		float lastFrameTime = static_cast<float>(glfwGetTime());
		float deltaTime = lastFrameTime;

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPosCallback(window, &mouseCallback);
		glfwSetScrollCallback(window, &mouseScrollCallback);

		//FRAME BUFFER SET UP
		GLuint frameBuffer;
		glGenFramebuffers(1, &frameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

		GLuint renderedTexture;
		glGenTextures(1, &renderedTexture);

		glBindTexture(GL_TEXTURE_2D, renderedTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		GLuint fbDepthBuffer;
		glGenRenderbuffers(1, &fbDepthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, fbDepthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbDepthBuffer);
		
		//configure frame buffer
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
		GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawBuffers);
		
		//check that framebuffer creation/setup was successful
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
			std::cout << "frame buffer failed" << std::endl;
			return 0;
		}

		while (!glfwWindowShouldClose(window))
		{
			bool renderToFrameBuffer = true;
			
			for (int i = 0; i < 2; ++i) 
			{
				if (renderToFrameBuffer)
				{
					glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
					//glViewport(0, 0, screenWidth, screenHeight); //don't need if screen is same size?
				}
				else
				{
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
				}

				float currentTime = static_cast<float>(glfwGetTime());
				deltaTime = currentTime - lastFrameTime;
				lastFrameTime = currentTime;
				camera.pollMovements(window, deltaTime);
				glm::mat4 lookat = camera.getViewMatrix(); //cache here so generation isn't repeated in loop

				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Notice this was changed to clear depth

				if (renderToFrameBuffer)
				{
					fbTextureShader_writesColorOut.use();
					fbTextureShader_writesColorOut.activateTextures();
				}
				else
				{
					shader2attribs.use();
					shader2attribs.activateTextures();
				}

				size_t numElementsInCubeArray = sizeof(cubePositions) / (3 * sizeof(float));
				for (size_t i = 0; i < numElementsInCubeArray; ++i)
				{
					glm::mat4 model;
					glm::mat4 view;
					glm::mat4 projection;

					model = glm::translate(model, cubePositions[i]);
					model = glm::rotate(model, static_cast<float>(glm::radians(50.0f) * i), glm::vec3(0.5 * (i + 1), 0.5f * (i + 1), 0.5f * (i + 1)));
					view = lookat;// camera.getViewMatrix() = lookat;
					projection = glm::perspective(glm::radians(camera.getFOV()), static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.f);

				

					//probably slow operations, this could be done before any rendering.
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

				if (!renderToFrameBuffer)
				{
					shader2attribs.use();
					shader2attribs.activateTextures();
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, renderedTexture);

					glm::mat4 model;
					glm::mat4 view;
					glm::mat4 projection;

					model = glm::translate(model, glm::vec3(0, 0, -10));
					
					view = lookat;// camera.getViewMatrix() = lookat;
					projection = glm::perspective(glm::radians(camera.getFOV()), static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.f);

					GLuint modelIndex = glGetUniformLocation(fbTextureShader_writesColorOut.getId(), "model");
					GLuint viewIndex = glGetUniformLocation(fbTextureShader_writesColorOut.getId(), "view");
					GLuint projectionIndex = glGetUniformLocation(fbTextureShader_writesColorOut.getId(), "projection");
					shader2attribs.setUniform1f("mixRatio", 0.1f);

					glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(model));
					glUniformMatrix4fv(viewIndex, 1, GL_FALSE, glm::value_ptr(view));
					glUniformMatrix4fv(projectionIndex, 1, GL_FALSE, glm::value_ptr(projection));

					//DRAW
					glBindVertexArray(VAO);
					glDrawArrays(GL_TRIANGLES, 0, 36);
					shader2attribs.setUniform1f("mixRatio", 0.5f); //restore
				}
				renderToFrameBuffer = false;
				
			}

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

//int main()
//{
//	RenderToFrameBuffer::main();
//}