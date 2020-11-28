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

GLuint textureLoader(const char* relative_filepath, int texture_unit /*= -1*/, bool useGammaCorrection /*= false*/)
{
	int img_width, img_height, img_nrChannels;
	unsigned char* textureData = stbi_load(relative_filepath, &img_width, &img_height, &img_nrChannels, 0);
	if (!textureData)
	{
		std::cerr << "failed to load texture" << std::endl;
		exit(-1);
	}

	GLuint textureID;
	glGenTextures(1, &textureID);

	if (texture_unit >= 0)
	{
		glActiveTexture(texture_unit);
	}
	glBindTexture(GL_TEXTURE_2D, textureID);

	int mode = -1;
	int dataFormat = -1;
	if (img_nrChannels == 3)
	{
		mode = useGammaCorrection ? GL_SRGB: GL_RGB;
		dataFormat = GL_RGB;
	}
	else if (img_nrChannels == 4)
	{
		mode = useGammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
		dataFormat = GL_RGBA;
	}
	else if (img_nrChannels == 1)
	{
		mode = GL_RED;
		dataFormat = GL_RED;
	}
	else
	{
		std::cerr << "unsupported image format for texture at " << relative_filepath << " there are " << img_nrChannels << "channels" << std::endl;
		exit(-1);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, mode, img_width, img_height, 0, dataFormat, GL_UNSIGNED_BYTE, textureData);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); //causes issue with materials on models
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT); //causes issue with materials on models
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	stbi_image_free(textureData);

	return textureID;
}

