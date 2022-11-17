///Exercise1 not updating camera class, but just updating .cpp since it will be quicker and essentially to the same effect

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

namespace CameraNamespaceX1
{
	void pollArrowKeys(GLFWwindow * window, glm::vec3& position, const glm::vec3& cameraFront, const glm::vec3& cameraUp, const float deltaTime);

	float yaw = -90.f;
	float pitch, lastX, lastY = 0.f;
	void mouseCallback(GLFWwindow* window, double xpos, double ypos);

	float FOV = 45.f;
	void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

	static const int screenHeight = 600;
	static const int screenWidth = 800;

	int main()
	{
		GLFWwindow* window = Utils::initOpenGl(screenWidth, screenHeight);
		if (!window) return -1;

		//Special shader that allows uniforms to be set based on arrow keys
		Shader shader2attribs("6.VertShader2.glsl", "6.FragShader2.glsl");

		if (shader2attribs.createFailed()) { glfwTerminate(); return -1; }

		static const float vertices[]{
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
			0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
			-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

			-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
			0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
			0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
			-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
		};

		GLuint EAO, VAO, VBO;
		if (!Utils::generateObject2Attrib(vertices, sizeof(vertices), VAO, VBO))
		{
			glfwTerminate(); return -1;
		}

		//LOAD TEXTURE DATA
		int tWidth[2], tHeight[2], nrChannels[2];
		unsigned char* data = stbi_load("Textures/container.jpg", &(tWidth[0]), &(tHeight[0]), &(nrChannels[0]), 0);
		if (!data) { glfwTerminate(); return-1; }

		GLuint textures[2];
		glGenTextures(2, textures);
		glBindTexture(GL_TEXTURE_2D, textures[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tWidth[0], tHeight[0], 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		stbi_image_free(data);

		data = stbi_load("Textures/awesomeface3.png", &(tWidth[1]), &(tHeight[1]), &(nrChannels[1]), 0);
		if (!data) { glfwTerminate(); return-1; }

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tWidth[1], tWidth[1], 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		stbi_image_free(data);

		shader2attribs.use();
		GLuint texLoc1 = glGetUniformLocation(shader2attribs.getId(), "texture1");
		GLuint texLoc2 = glGetUniformLocation(shader2attribs.getId(), "texture2");

		//using GL_TEXTURE0 to re-enforce that this is the number we're setting
		glUniform1i(texLoc1, 0);
		glUniform1i(texLoc2, GL_TEXTURE1 - GL_TEXTURE0);

		GLuint mixRatioUniform = glGetUniformLocation(shader2attribs.getId(), "mixRatio");
		float mixSetting = 0.5f;
		glUniform1f(mixRatioUniform, mixSetting);

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
		glEnable(GL_DEPTH_TEST);

		float lastFrameTime = static_cast<float>(glfwGetTime());
		float deltaTime = lastFrameTime;

		glm::vec3 cameraPosition(0.f, 0.f, 3.f);
		glm::vec3 cameraFrontBasis(0.f, 0.f, -1.f);
		glm::vec3 cameraUpBasis(0.f, 1.f, 0.f);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetCursorPosCallback(window, &mouseCallback);
		glfwSetScrollCallback(window, &mouseScrollCallback);
		while (!glfwWindowShouldClose(window))
		{
			float currentTime = static_cast<float>(glfwGetTime());
			deltaTime = currentTime - lastFrameTime;
			lastFrameTime = currentTime;

			cameraFrontBasis.y = sin(glm::radians(pitch));
			cameraFrontBasis.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
			cameraFrontBasis.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
			cameraFrontBasis = glm::normalize(cameraFrontBasis);

			//------------ CAMERA ------------------
			//SET UP CAMERA
			//uses cross product to figure out all the basis vectors of a camera looking at a position,
			//see part 1 for how this calculation is performed, but to summarize it is basically:
			//1. Determine the direction the camera is looking by taking the vector difference of its position and target position; this will be the Z axis
			//2. Choose an upward vector (0.f, 1.f, 0.f), this isn't the camera's y axis but is on the same plane as the z and y axis.
			//3. use cross product of the z-axis and the pseudo-up axis to get a vector perpendicular to this plane and normalize it; this is our right (x) axis
			//4. now that we have the z and x basis vectors, take the cross product between them and normalize it to get the last perpendicular axis, our y axis
			//5. use this to construct a 4d homgenous matrix where the basis vectors are specifyed by rows (X, Y, Z, ...)
			glm::mat4 lookAt = glm::lookAt(cameraPosition, cameraPosition + cameraFrontBasis, cameraUpBasis);

			//------------ END CAMERA ---------------
			pollArrowKeys(window, cameraPosition, cameraFrontBasis, cameraUpBasis, deltaTime);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Notice this was changed to clear depth

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures[0]);

			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, textures[1]);

			shader2attribs.use();

			size_t numElementsInCubeArray = sizeof(cubePositions) / (3 * sizeof(float));
			for (size_t i = 0; i < numElementsInCubeArray; ++i)
			{
				glm::mat4 model;
				glm::mat4 view;
				glm::mat4 projection;

				model = glm::translate(model, cubePositions[i]);
				model = glm::rotate(model, static_cast<float>(glm::radians(50.0f) * i), glm::vec3(0.5 * i, 0.5f * i, 0.5f * i));
				view = lookAt;
				projection = glm::perspective(glm::radians(FOV), static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.f);

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

			glfwPollEvents();
			Utils::setWindowCloseOnEscape(window);
			glfwSwapBuffers(window);

			static long delayMsFor60FPS = static_cast<long>(1 / 60.f * 1000);
			//std::this_thread::sleep_for(std::chrono::milliseconds(delayMsFor60FPS)); //smoother mouse :)
		}

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EAO);

		return 0;
	}

	void pollArrowKeys(GLFWwindow * window, glm::vec3& position, const glm::vec3& cameraFront, const glm::vec3& cameraUp, const float deltaTime)
	{
		static float movementSpeed = 10.f;
		float adjustedMovementSpeed = movementSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
			position += glm::normalize(glm::cross(glm::vec3(0.f, 1.f, 0.f), right)) * adjustedMovementSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
			position -= glm::normalize(glm::cross(glm::vec3(0.f, 1.f, 0.f), right)) * adjustedMovementSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			position -= glm::normalize(glm::cross(cameraFront, cameraUp)) * adjustedMovementSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			position += glm::normalize(glm::cross(cameraFront, cameraUp)) * adjustedMovementSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		{
			//camera up should be a normalized vector
			//position += cameraUp * adjustedMovementSpeed;

			//figure out upward basis vector on fly;
			//1. we get the right basis vector from the inner most cross product, we don't need to normalize it since we only need for its direction in the next cross product
			//2. the second (outter) cross product returns the upward basis vector using the front basis vector and the derived right basis vector (from the inner cross product)
			//3. normalize it since we are going to use its magnitude 
			position += glm::normalize(glm::cross(glm::cross(cameraFront, cameraUp), cameraFront)) * adjustedMovementSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		{
			//camera up should be a normalized vector
			//position -= cameraUp * adjustedMovementSpeed; 

			//figure out upward basis vector on fly (see above note)
			position -= glm::normalize(glm::cross(glm::cross(cameraFront, cameraUp), cameraFront)) * adjustedMovementSpeed;
		}
	}

	static bool firstMouseCall = true;
	void mouseCallback(GLFWwindow* window, double xpos, double ypos)
	{
		if (firstMouseCall)
		{
			lastX = static_cast<float>(xpos);
			lastY = static_cast<float>(ypos);
			firstMouseCall = false;
			return;
		}

		static float sensitivity = 0.1f;
		float deltaX = static_cast<float>(xpos - lastX);
		float deltaY = static_cast<float>(lastY - ypos);
		lastX = static_cast<float>(xpos);
		lastY = static_cast<float>(ypos);

		yaw += deltaX * sensitivity;
		pitch += deltaY * sensitivity;

		if (pitch > 89.f)
			pitch = 89.f;
		if (pitch < -89.f)
			pitch = -89.f;
	}


	void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
	{
		FOV -= static_cast<float>(yOffset);

		//clamp FOV between the two values
		if (FOV < 1.0f)
			FOV = 1.0f;
		if (FOV > 45.0f)
			FOV = 45.0f;
	}
}

//int main() {
//	return CameraNamespaceX1::main();
//}