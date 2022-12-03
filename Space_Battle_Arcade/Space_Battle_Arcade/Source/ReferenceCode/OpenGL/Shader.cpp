#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include "Shader.h"
#include "Utilities.h"

#include"Texture2D.h"


/** 
	Utility class that uses RAII to cache and restore the currently bound shader in its ctor.
	When the dtor is called it restores the previous shader.

	Useful for setting uniforms. Since a shader must be active when the uniform is set, this can be used in the scope of a setter.
	the setter will create an instance of this class, update the uniform, and when the function goes out scope the previous shader will be restored.
*/
class RAII_ScopedShaderSwitcher
{
//use below to control whether this class functions at compile time.
#define UNIFORM_UPDATE_CACHE_PREVIOUS_SHADER 1

private:
#if UNIFORM_UPDATE_CACHE_PREVIOUS_SHADER
	GLint cachedPreviousShader;
#endif // UNIFORM_UPDATE_CACHE_PREVIOUS_SHADER

public:
	RAII_ScopedShaderSwitcher(GLint switchToThisShader)
	{
#if UNIFORM_UPDATE_CACHE_PREVIOUS_SHADER
		//Cache previous shader and restore it after update; NOTE: I've read that the get* can cause performance hits in multi-threaded opengl drivers: https://www.opengl.org/discussion_boards/showthread.php/177044-How-do-I-get-restore-the-current-shader //Article on opengl perf https://software.intel.com/en-us/articles/opengl-performance-tips-avoid-opengl-calls-that-synchronize-cpu-and-gpu
		glGetIntegerv(GL_CURRENT_PROGRAM, &cachedPreviousShader);
		glUseProgram(switchToThisShader);
#endif // UNIFORM_UPDATE_CACHE_PREVIOUS_SHADER
	}
	~RAII_ScopedShaderSwitcher()
	{
#if UNIFORM_UPDATE_CACHE_PREVIOUS_SHADER
		//restore previous shader
		glUseProgram(cachedPreviousShader);
#endif // UNIFORM_UPDATE_CACHE_PREVIOUS_SHADER
	}
};

void Shader::initShaderHelper(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath, const std::string& geometryShaderFilePath, bool stringsAreFilePaths /*= true*/)
{
	bool hasGeometryShader = geometryShaderFilePath != "";

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);

	//LOAD SOURCES FROM FILE
	std::string vertShaderSrc, fragShaderSrc, geoShaderSrc;
	if (stringsAreFilePaths)
	{
		if ( (failed = !Utils::convertFileToString(vertexShaderFilePath, vertShaderSrc)) )
		{
			std::cerr << "failed to load vertex shader from file" << std::endl;
			return;
		}
		if ( (failed = !Utils::convertFileToString(fragmentShaderFilePath, fragShaderSrc)) )
		{
			std::cerr << "failed to load fragment shader from file" << std::endl;
			return;
		}
		if (hasGeometryShader)
		{
			if ( (failed = !Utils::convertFileToString(geoShaderSrc, geoShaderSrc)) )
			{
				std::cerr << "failed to load fragment shader from file" << std::endl;
				return;
			}
		}
	}
	else
	{
		//This is a refactor of the original class to allow directly passing the shader source.
		//could probably refactor this ctor so that we don't need to do this superfluous copy; 
		//but since this is only generally called at start up I'll let it make an extra copy
		vertShaderSrc = vertexShaderFilePath;
		fragShaderSrc = fragmentShaderFilePath;
		geoShaderSrc = geometryShaderFilePath;
	}

	//SET SHADER SOURCES
	const char* vertShaderSource_cstr = vertShaderSrc.c_str();
	glShaderSource(vertexShader, 1, &vertShaderSource_cstr, nullptr);

	const char* fragShaderSource_cstr = fragShaderSrc.c_str();
	glShaderSource(fragmentShader, 1, &fragShaderSource_cstr, nullptr);

	const char* geoShaderSource_cstr = geoShaderSrc.c_str();
	glShaderSource(geometryShader, 1, &geoShaderSource_cstr, nullptr);

	//COMPILE SHADERS
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);
	if (hasGeometryShader)
	{
		glCompileShader(geometryShader);
	}

	if ( (failed = !shaderCompileSuccess(vertexShader)) )
	{
		std::cerr << "failed to compile the vertex shader" << std::endl;
		return;
	}
	if ( (failed = !shaderCompileSuccess(fragmentShader)) )
	{
		std::cerr << "failed to compile the fragment shader" << std::endl;
		return;
	}

	if (hasGeometryShader)
	{
		if ( (failed = !shaderCompileSuccess(geometryShader)) )
		{
			std::cerr << "failed to compile the geom shader" << std::endl;
			return;
		}
	}

	//ATTACH AND LINK SHADERS
	linkedProgram = glCreateProgram();
	glAttachShader(linkedProgram, vertexShader);
	glAttachShader(linkedProgram, fragmentShader);
	if (hasGeometryShader)
		glAttachShader(linkedProgram, geometryShader);
	glLinkProgram(linkedProgram);
	if ( (failed = !programLinkSuccess(linkedProgram)) )
	{
		std::cerr << "failed to link shader program" << std::endl;
		return;
	}

	//CLEAN UP
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteShader(geometryShader);
}

Shader::Shader(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath, bool stringsAreFilePaths /*= true*/)
	:
	failed(false),
	linkedProgram(0),
	active(false),
	textures()
{
	std::string blankGeometryShader = "";
	initShaderHelper(vertexShaderFilePath, fragmentShaderFilePath, blankGeometryShader, stringsAreFilePaths);
}

Shader::Shader(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath, const std::string& geometryShaderFilePath, bool stringsAreFilePaths) :
	failed(false),
	linkedProgram(0),
	active(false),
	textures()
{
	initShaderHelper(vertexShaderFilePath, fragmentShaderFilePath, geometryShaderFilePath, stringsAreFilePaths);
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
	RAII_ScopedShaderSwitcher scoped(linkedProgram);

	//do not need to be using shader to query location of uniform
	int uniformLocation = glGetUniformLocation(linkedProgram, uniform);

	//must be using the shader to update uniform value
	glUseProgram(linkedProgram);
	glUniform4f(uniformLocation, red, green, blue, alpha);
}

void Shader::setUniform4f(const char* uniform, const glm::vec4& values)
{
	setUniform4f(uniform, values.r, values.g, values.b, values.a);
}

void Shader::setUniform3f(const char* uniform, float red, float green, float blue)
{
	RAII_ScopedShaderSwitcher scoped(linkedProgram);

	int uniformLocation = glGetUniformLocation(linkedProgram, uniform);
	glUseProgram(linkedProgram);
	glUniform3f(uniformLocation, red, green, blue);
}

void Shader::setUniform3f(const char* uniform, const glm::vec3& vals)
{
	RAII_ScopedShaderSwitcher scoped(linkedProgram);

	int uniformLocation = glGetUniformLocation(linkedProgram, uniform);
	glUseProgram(linkedProgram);
	glUniform3f(uniformLocation, vals.r, vals.g, vals.b);
}

void Shader::setUniform1i(const char* uniform, int newValue)
{
	RAII_ScopedShaderSwitcher scoped(linkedProgram);

	int uniformLocation = glGetUniformLocation(linkedProgram, uniform);
	glUseProgram(linkedProgram);
	glUniform1i(uniformLocation, newValue);
}


void Shader::setUniformMatrix4fv(const char* uniform, int numberMatrices, GLuint transpose, const float* data)
{
	RAII_ScopedShaderSwitcher scoped(linkedProgram);

	int uniformLocation = glGetUniformLocation(linkedProgram, uniform);
	glUseProgram(linkedProgram);
	glUniformMatrix4fv(uniformLocation, numberMatrices, transpose, data);
}

void Shader::setUniform1f(const char* uniformName, float value)
{
	GLuint uniformLocationInShader = glGetUniformLocation(getId(), uniformName);
	glUseProgram(linkedProgram);
	glUniform1f(uniformLocationInShader, value);
}

void Shader::addTexture(std::shared_ptr<Texture2D>& texture, const std::string& textureSampleName)
{
	/** DEPRECATED: I made this earlier on and now handle textures differently. Leaving so my early files still work.*/
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
	/** DEPRECATED: I made this earlier on and now handle textures differently. Leaving so my early files still work.*/
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
