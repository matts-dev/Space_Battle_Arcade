#pragma once
#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

void verifyShaderCompiled(const char* shadername, GLuint shader);

void verifyShaderLink(GLuint shaderProgram);

GLFWwindow* init_window(int width, int height);