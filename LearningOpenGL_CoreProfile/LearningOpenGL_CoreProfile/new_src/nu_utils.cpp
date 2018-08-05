#include "nu_utils.h"
#include<iostream>

void verifyShaderCompiled(const char* shadername, GLuint shader)
{
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		constexpr int size = 512;
		char infolog[size];

		glGetShaderInfoLog(shader, size, nullptr, infolog);
		std::cerr << "shader failed to compile: " << shadername << infolog << std::endl;
		exit(-1);
	}
}

void verifyShaderLink(GLuint shaderProgram)
{
	GLint success = 0;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		constexpr int size = 512;
		char infolog[size];

		glGetProgramInfoLog(shaderProgram, size, nullptr, infolog);
		std::cerr << "SHADER LINK ERROR: " << infolog << std::endl;
		exit(-1);
	}
}

GLFWwindow* init_window(int width, int height)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(width, height, "OpenglContext", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "failed to create window" << std::endl;
		exit(-1);
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "failed to initialize glad with processes " << std::endl;
		exit(-1);
	}

	return window;
}

