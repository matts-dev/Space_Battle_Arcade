#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include "Shader.h"
#include "Utilities.h"

#include"Texture2D.h"

Shader::Shader(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath, bool stringsAreFilePaths) :
	failed(false),
	linkedProgram(0),
	active(false),
	textures()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	//LOAD SOURCES FROM FILE
	std::string vertShaderSrc, fragShaderSrc;
	if (stringsAreFilePaths)
	{
		if (failed = !Utils::convertFileToString(vertexShaderFilePath, vertShaderSrc))
		{
			std::cerr << "failed to load vertex shader from file" << std::endl;
			return;
		}
		if (failed = !Utils::convertFileToString(fragmentShaderFilePath, fragShaderSrc))
		{
			std::cerr << "failed to load fragment shader from file" << std::endl;
			return;
		}
	}
	else
	{
		//This is a refactor of the original class to allow directly passing the shader source.
		//could probably refactor this ctor so that we don't need to do this superfluous copy; 
		//but since this is only generally called at start up I'll let it make an extra copy
		vertShaderSrc = vertexShaderFilePath;
		fragShaderSrc = fragmentShaderFilePath;
	}

	//SET SHADER SOURCES
	const char* vertShaderSource_cstr = vertShaderSrc.c_str();
	glShaderSource(vertexShader, 1, &vertShaderSource_cstr, nullptr);

	const char* fragShaderSource_cstr = fragShaderSrc.c_str();
	glShaderSource(fragmentShader, 1, &fragShaderSource_cstr, nullptr);

	//COMPILE SHADERS
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	if (failed = !shaderCompileSuccess(vertexShader))
	{
		std::cerr << "failed to compile the vertex shader" << std::endl;
		return;
	}
	if (failed = !shaderCompileSuccess(fragmentShader))
	{
		std::cerr << "failed to compile the fragment shader" << std::endl;
		return;
	}

	//ATTACH AND LINK SHADERS
	linkedProgram = glCreateProgram();
	glAttachShader(linkedProgram, vertexShader);
	glAttachShader(linkedProgram, fragmentShader);
	glLinkProgram(linkedProgram);
	if (failed = !programLinkSuccess(linkedProgram))
	{
		std::cerr << "failed to link shader program" << std::endl;
		return;
	}

	//CLEAN UP
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

}

Shader::~Shader()
{
}

void Shader::use(bool activate)
{
	if (activate)
	{
		glUseProgram(linkedProgram);
		activateTextures();
	}
	else
	{
		glUseProgram(0);
	}
}

GLuint Shader::getId()
{
	return linkedProgram;
}

void Shader::setUniform4f(const char* uniform, float red, float green, float blue, float alpha)
{
	//do not need to be using shader to query location of uniform
	int uniformLocation = glGetUniformLocation(linkedProgram, uniform);

	//Cache previous shader and restore it after update; NOTE: I've read that the get* can cause performance hits in multi-threaded opengl drivers: https://www.opengl.org/discussion_boards/showthread.php/177044-How-do-I-get-restore-the-current-shader //Article on opengl perf https://software.intel.com/en-us/articles/opengl-performance-tips-avoid-opengl-calls-that-synchronize-cpu-and-gpu
	GLint cachedPreviousShader;
	glGetIntegerv(GL_CURRENT_PROGRAM, &cachedPreviousShader);

	//must be using the shader to update uniform value
	glUseProgram(linkedProgram);
	glUniform4f(uniformLocation, red, green, blue, alpha);

	//restore previous shader
	glUseProgram(cachedPreviousShader);

}

void Shader::setUniform3f(const char* uniform, float red, float green, float blue)
{
	//do not need to be using shader to query location of uniform
	int uniformLocation = glGetUniformLocation(linkedProgram, uniform);

	//Cache previous shader and restore it after update; NOTE: I've read that the get* can cause performance hits in multi-threaded opengl drivers: https://www.opengl.org/discussion_boards/showthread.php/177044-How-do-I-get-restore-the-current-shader //Article on opengl perf https://software.intel.com/en-us/articles/opengl-performance-tips-avoid-opengl-calls-that-synchronize-cpu-and-gpu
	GLint cachedPreviousShader;
	glGetIntegerv(GL_CURRENT_PROGRAM, &cachedPreviousShader);

	//must be using the shader to update uniform value
	glUseProgram(linkedProgram);
	glUniform3f(uniformLocation, red, green, blue);

	//restore previous shader
	glUseProgram(cachedPreviousShader);
}

void Shader::setUniform1i(const char* uniform, int newValue)
{
	//TODO figure out a method for caching to prevent code duplication

	//do not need to be using shader to query location of uniform
	int uniformLocation = glGetUniformLocation(linkedProgram, uniform);

	//Cache previous shader and restore it after update; NOTE: I've read that the get* can cause performance hits in multi-threaded opengl drivers: https://www.opengl.org/discussion_boards/showthread.php/177044-How-do-I-get-restore-the-current-shader //Article on opengl perf https://software.intel.com/en-us/articles/opengl-performance-tips-avoid-opengl-calls-that-synchronize-cpu-and-gpu
	GLint cachedPreviousShader;
	glGetIntegerv(GL_CURRENT_PROGRAM, &cachedPreviousShader);

	//must be using the shader to update uniform value
	glUseProgram(linkedProgram);
	glUniform1i(uniformLocation, newValue);

	//restore previous shader
	glUseProgram(cachedPreviousShader);
}


void Shader::setUniform1f(const char* uniformName, float value)
{
	GLuint uniformLocationInShader = glGetUniformLocation(getId(), uniformName);
	glUniform1f(uniformLocationInShader, value);
}

void Shader::addTexture(std::shared_ptr<Texture2D>& texture, const std::string& textureSampleName)
{
	if (texture)
	{
		GLuint textureShaderSampleLocation = glGetUniformLocation(getId(), textureSampleName.c_str());
		textures.push_back(texture);
		//NOTE: we're basically setting to use which: GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, ... GL_TEXTURE16
		GLuint GL_TEXTURENUM = textures.size() - 1;
		glUniform1i(textureShaderSampleLocation, GL_TEXTURENUM);
	}
}

void Shader::activateTextures()
{
	for (size_t i = 0; i < textures.size(); ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, textures[i]->getTextureId());
	}
}

bool Shader::shaderCompileSuccess(GLuint shaderID)
{
	char infolog[256];
	int success = true;

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shaderID, 256, nullptr, infolog);
		std::cerr << "shader compile fail, infolog:\n" << infolog << std::endl;
	}

	return success != 0; //probably more efficient to just return success and allow implicit cast 
}

bool Shader::programLinkSuccess(GLuint programID)
{
	char infolog[256];
	int success = true;

	glGetProgramiv(programID, GL_LINK_STATUS, &success);

	if (!success)
	{
		glGetProgramInfoLog(programID, 256, nullptr, infolog);
		std::cerr << "OpenGL program link fail, infolog:\n" << infolog << std::endl;
	}

	return success != 0; //probably more efficient to just return success and allow implicit cast 
}
