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

namespace CoordinateSystems {
	void pollArrowKeys(GLFWwindow * window, float& valueToControl);
	
	static const int screenHeight = 600;
	static const int screenWidth = 800;

	int main() {
		//SKIP TO TRANFORMATION SECTION
		bool bShouldFlip = false;
		bool bPressRegistered = false;

		GLFWwindow* window = Utils::initOpenGl(screenWidth, screenHeight);
		if (!window) return -1;

		//Special shader that allows uniforms to be set based on arrow keys
		//Shader shader3attribs("5.VertShader.glsl", "5.FragShader.glsl");
		Shader shader3attribs("6.VertShader.glsl", "6.FragShader.glsl");

		if (shader3attribs.createFailed()) { glfwTerminate(); return -1; }

		static const float vertices[] = {
			// positions          // colors           // texture coords
			0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
			0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
			-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
			-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
		};

		GLuint EAO, VAO, VBO;
		Utils::generateRectForTextChallenge2(vertices, sizeof(vertices), EAO, VAO, VBO);


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

		shader3attribs.use();
		GLuint texLoc1 = glGetUniformLocation(shader3attribs.getId(), "texture1");
		GLuint texLoc2 = glGetUniformLocation(shader3attribs.getId(), "texture2");

		//using GL_TEXTURE0 to re-enforce that this is the number we're setting
		glUniform1i(texLoc1, 0);
		glUniform1i(texLoc2, GL_TEXTURE1 - GL_TEXTURE0);

		GLuint mixRatioUniform = glGetUniformLocation(shader3attribs.getId(), "mixRatio");
		float mixSetting = 0.5f;
		glUniform1f(mixRatioUniform, mixSetting);

		//rotate the plane
		glm::mat4 model;
		model = glm::rotate(model, glm::radians(-55.f), glm::vec3(1.0, 0.0f, 0.0f));

		// move the camera backwards by pushing the entire scene forward (negative z direction)
		glm::mat4 view;
		view = glm::translate(view, glm::vec3(0.f, 0.f, -3.f));

		//create the project matrix for the perspective 
		glm::mat4 projection;
		projection = glm::perspective(glm::radians(45.f), static_cast<float>(screenWidth) / screenHeight, 0.1f, 100.f);
		
		shader3attribs.use();
		GLuint modelIndex = glGetUniformLocation(shader3attribs.getId(), "model");
		GLuint viewIndex = glGetUniformLocation(shader3attribs.getId(), "view");
		GLuint projectionIndex = glGetUniformLocation(shader3attribs.getId(), "projection");

		glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(viewIndex, 1, GL_FALSE, glm::value_ptr(view));
		//glUniformMatrix4fv(viewIndex, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(projectionIndex, 1, GL_FALSE, glm::value_ptr(projection));

		while (!glfwWindowShouldClose(window))
		{
			pollArrowKeys(window, mixSetting);
			glUniform1f(mixRatioUniform, mixSetting);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures[0]);

			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, textures[1]);

			shader3attribs.use();
			//UPDATE TRANSFORM
			//glm::mat4 transformBottomRight;
			//transformBottomRight = glm::translate(transformBottomRight, glm::vec3(0.5f, -0.5f, 0.f)); //translation should occur first, this prevents rotation/scaling from affecting translation
			//transformBottomRight = glm::rotate(transformBottomRight, static_cast<float>(glfwGetTime()), glm::vec3(0.f, 0.f, 1.0f));
			//transformBottomRight = glm::scale(transformBottomRight, glm::vec3(0.5f, 0.5f, 0.5f));

			//GLuint uniformLocation = glGetUniformLocation(shader3attribs.getId(), "transform");
			//glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(transformBottomRight));

			//DRAW
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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

//int main() {
//	return CoordinateSystems::main();
//}