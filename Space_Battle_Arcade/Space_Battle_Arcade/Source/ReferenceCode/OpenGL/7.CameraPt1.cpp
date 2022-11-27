#pragma once

#include<iostream>
#include"Utilities.h"
#include"Shader.h"
#include "Libraries/stb_image.h"
#include"glm/glm.hpp"
#include"glm/gtc/matrix_transform.hpp"
#include"glm/gtc/type_ptr.hpp"
#include <chrono>
#include <thread>

namespace CameraNamespacePt1 {
	void pollArrowKeys(GLFWwindow * window, float& valueToControl);

	static const int screenHeight = 600;
	static const int screenWidth = 800;

	int main() {
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

		while (!glfwWindowShouldClose(window))
		{
			//------------ CAMERA ------------------
			//SET UP CAMERA
			float cameraSpinRadius = 10.f;
			glm::vec3 cameraPosition(sin(glfwGetTime()) * cameraSpinRadius, 0.f, cos(glfwGetTime()) * cameraSpinRadius);
			glm::vec3 cameraTarget(0.f, 0.f, 0.f);
			//glm::vec3 cameraPosition(6.f, 2.f, 3.f); //testing how if order of custom look at is correct
			//glm::vec3 cameraTarget(1.f, 4.f, 2.f);

			glm::vec3 cameraDirection = glm::normalize(cameraPosition - cameraTarget);

			glm::vec3 up(0.0f, 1.0f, 0.0f);
			//remember cross product gives us a vector perpendicular to the other two, normalize vector to make it a base vector
			glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
			glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
			//glm::mat4 customLookAt(
			//	cameraRight.x, cameraRight.y, cameraRight.z, 0.f,
			//	cameraUp.x, cameraUp.y, cameraUp.z, 0.f,
			//	cameraDirection.x, cameraDirection.y, cameraDirection.z, 0.f,
			//	0.f, 0.f, 0.f, 1.f);

			//glm::mat4 customLookAt( //while this looks right, the constructor order isn't column basis vectors, but row basis vectors :\
			//	cameraRight.x, cameraUp.x, cameraDirection.x, 0.f,
			//	cameraRight.y, cameraUp.y, cameraDirection.y, 0.f,
			//	cameraRight.z, cameraUp.z, cameraDirection.z, 0.f,
			//	0.f, 0.f, 0.f, 1.f);

			//create based on columns
			//glm::mat4 customLookAt(glm::vec4(cameraRight, 0.f), glm::vec4(cameraUp, 0.f), glm::vec4(cameraDirection, 0.f), glm::vec4(0.f, 0.f, 0.f, 1.f));
			//glm::mat4 customLookAt(
			//	glm::vec4(cameraRight.x, cameraUp.x, cameraDirection.x, 0.f),
			//	glm::vec4(cameraRight.y, cameraUp.y, cameraDirection.y, 0.f),
			//	glm::vec4(cameraRight.z, cameraUp.z, cameraDirection.z, 0.f),
			//	glm::vec4(0.f, 0.f, 0.f, 1.f));
			// Note: This is column major ordering, thus the first index specifies the column ([0][4] is column 0, [2][3] is column 2)
			glm::mat4 customLookAt;
			customLookAt[0][0] = cameraRight.x;
			customLookAt[0][1] = cameraUp.x;
			customLookAt[0][2] = cameraDirection.x;
			customLookAt[0][3] = 0.f;

			customLookAt[1][0] = cameraRight.y;
			customLookAt[1][1] = cameraUp.y;
			customLookAt[1][2] = cameraDirection.y;
			customLookAt[1][3] = 0.f;

			customLookAt[2][0] = cameraRight.z;
			customLookAt[2][1] = cameraUp.z;
			customLookAt[2][2] = cameraDirection.z;
			customLookAt[2][3] = 0.f;

			customLookAt[3][0] = 0.f;
			customLookAt[3][1] = 0.f;
			customLookAt[3][2] = 0.f;
			customLookAt[3][3] = 1.f;

			glm::mat4 translateCameraPosition;
			translateCameraPosition = glm::translate(translateCameraPosition, -cameraPosition);
			customLookAt = customLookAt * translateCameraPosition;

			glm::mat4 lookAt = glm::lookAt(cameraPosition, cameraTarget, up);
			//std::cout << (customLookAt == lookAt ? "true" : "false") << std::endl;

			//------------ END CAMERA ---------------
			pollArrowKeys(window, mixSetting);
			glUniform1f(mixRatioUniform, mixSetting);

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
				//view = lookAt;
				view = customLookAt;
				projection = glm::perspective(glm::radians(45.f), static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.f);

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
			std::this_thread::sleep_for(std::chrono::milliseconds(delayMsFor60FPS));
		}

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EAO);

		return 0;
	}

	void pollArrowKeys(GLFWwindow * window, float& valueToControl)
	{
		float incAmount = 0.05f;
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		{
			valueToControl += incAmount;
		}
		else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		{
			valueToControl -= incAmount;
		}
	}
}
//
//int main() {
//	return CameraNamespacePt1::main();
//}