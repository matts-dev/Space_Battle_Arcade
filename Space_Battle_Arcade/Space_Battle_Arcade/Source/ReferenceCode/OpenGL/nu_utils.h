#pragma once
#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "Libraries/stb_image.h"

void verifyShaderCompiled(const char* shadername, GLuint shader);

void verifyShaderLink(GLuint shaderProgram);

GLFWwindow* init_window(int width, int height);

GLuint textureLoader(const char* relative_filepath, int texture_unit = -1, bool useGammaCorrection = false);