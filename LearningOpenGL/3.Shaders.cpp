#pragma once
#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include<string>
#include<iostream>
#include<fstream>
#include<sstream>

#include"Utilities.h"
#include"Shader.h"

namespace shaders {
	int main() {
		GLFWwindow* window = Utils::initOpenGl(800, 400);
		if (!window) { return -1; }

		Shader basicShader("BasicVertexShader.glsl", "BasicFragShader.glsl");
		if (basicShader.createFailed()) { glfwTerminate(); return -1; }

		GLuint VAO, VBO, EAO;
		Utils::createSingleElementTriangle(EAO, VAO, VBO);

		while (!glfwWindowShouldClose(window)) {
			glClearColor(0.f, 0.f, 0.f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			basicShader.use();
			glBindVertexArray(VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

			glfwPollEvents();
			Utils::setWindowCloseOnEscape(window);
			glfwSwapBuffers(window);
		}

		glfwTerminate();
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EAO);
		return 0;
	}

}


int main() {
	//std::ifstream inFileStream;
	//inFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);//enable these type of exceptions
	//std::stringstream strStream;

	//std::string vertexShaderSrc, fragShaderSrc;
	//if (!Utils::convertFileToString("BasicVertexShader.glsl", vertexShaderSrc)) { std::cerr << "failed to load vertex shader src" << std::endl; return -1; }
	//if (!Utils::convertFileToString("BasicFragShader.glsl", fragShaderSrc)) { std::cerr << "failed to load frag shader src" << std::endl; return -1; }

	//std::cout << vertexShaderSrc << std::endl;
	//std::cout << fragShaderSrc << std::endl;

	//int x;
	//std::cin >> x;

	return shaders::main();
}
