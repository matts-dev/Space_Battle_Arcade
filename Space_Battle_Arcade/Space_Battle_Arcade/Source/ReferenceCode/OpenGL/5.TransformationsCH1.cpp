///Challenge3 Try to display only the center pixels of the texture image on the rectangle in such a way that the individual pixels are getting visible by changing the texture coordinates. Try to set the texture filtering method to GL_NEAREST to see the pixels more clearly:
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

namespace TransformationsCH1 {
	void pollArrowKeys(GLFWwindow * window, float& valueToControl);
	void pollFlipPress(GLFWwindow * window, bool& flipped, bool& pressRegistered);

	int main() {
		//SKIP TO TRANFORMATION SECTION
		bool bShouldFlip = false;
		bool bPressRegistered = false;

		GLFWwindow* window = Utils::initOpenGl(800, 600);
		if (!window) return -1;

		//Special shader that allows uniforms to be set based on arrow keys
		Shader shader3attribs("5.VertShader.glsl", "5.FragShader.glsl");

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
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);

		shader3attribs.use();
		GLuint texLoc1 = glGetUniformLocation(shader3attribs.getId(), "texture1");
		GLuint texLoc2 = glGetUniformLocation(shader3attribs.getId(), "texture2");

		//using GL_TEXTURE0 to reenforce that this is the number we're setting
		glUniform1i(texLoc1, 0);
		glUniform1i(texLoc2, GL_TEXTURE1 - GL_TEXTURE0);

		GLuint mixRatioUniform = glGetUniformLocation(shader3attribs.getId(), "mixRatio");
		float mixSetting = 0.5f;
		glUniform1f(mixRatioUniform, mixSetting);

		//TRANFORMATION 
		glm::vec4 vec(1.0f, 0.0f, 0.0f, 1.0f);
		glm::mat4 translationTransform; //is the identity matrix by default
		translationTransform = glm::translate(translationTransform, glm::vec3(1.0f, 1.0f, 0.0f));
		vec = translationTransform * vec; //matrix multiplication is not commutative; order matters 
		std::cout << "translation vector: " << vec.x << " " << vec.y << " " << vec.z << std::endl;

		//glm::mat4 rotateAndScaleTransform;
		//rotateAndScaleTransform = glm::rotate(rotateAndScaleTransform, glm::radians(90.0f), glm::vec3(0.f, 0.f, 1.0f));
		//rotateAndScaleTransform = glm::scale(rotateAndScaleTransform, glm::vec3(0.5f, 0.5f, 0.5f));

		//GLuint uniformLocation = glGetUniformLocation(shader3attribs.getId(), "transform");
		//glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(rotateAndScaleTransform));
		std::cout << "press f to flip rotation and translation order" << std::endl;
		while (!glfwWindowShouldClose(window))
		{

			pollFlipPress(window, bShouldFlip, bPressRegistered);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			shader3attribs.use();
			//UPDATE TRANSFORM
			glm::mat4 transform;
			if (bShouldFlip)
			{
				transform = glm::rotate(transform, static_cast<float>(glfwGetTime()), glm::vec3(0.f, 0.f, 1.0f));
				transform = glm::translate(transform, glm::vec3(0.5f, -0.5f, 0.f)); //translation should occur first, this prevents rotation/scaling from affecting translation
			}
			else
			{
				transform = glm::translate(transform, glm::vec3(0.5f, -0.5f, 0.f)); //translation should occur first, this prevents rotation/scaling from affecting translation
				transform = glm::rotate(transform, static_cast<float>(glfwGetTime()), glm::vec3(0.f, 0.f, 1.0f));
			}
			transform = glm::scale(transform, glm::vec3(0.5f, 0.5f, 0.5f));


			GLuint uniformLocation = glGetUniformLocation(shader3attribs.getId(), "transform");
			glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(transform));

			//DRAW
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures[0]);

			glUniform1f(mixRatioUniform, mixSetting);

			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, textures[1]);

			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


			glfwPollEvents();
			Utils::setWindowCloseOnEscape(window);
			pollArrowKeys(window, mixSetting);
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
		static const float incAmount = 0.0005f;
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		{
			valueToControl += incAmount;
		}
		else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		{
			valueToControl -= incAmount;
		}
	}

	void pollFlipPress(GLFWwindow * window, bool& flipped, bool& pressRegistered)
	{
		if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !pressRegistered)
		{
			flipped = !flipped;
			pressRegistered = true;
		}
		else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE && pressRegistered)
		{
			//if the key isn't pressed, then clear the previous registration of the key
			//this is a simple mechanism to prevent spamming of flipped bool
			pressRegistered = false;
		}
	}


}
//
//int main() {
//	return TransformationsCH1::main();
//}