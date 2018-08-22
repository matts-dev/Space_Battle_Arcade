#pragma once

#include<string>
#include<memory>
#include<vector>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

class Texture2D;

class Shader
{
public:
	explicit Shader(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath, bool stringsAreFilePaths = true);
	virtual ~Shader();

	Shader(const Shader&) = delete;
	Shader(const Shader&&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader& operator=(const Shader&&) = delete;

	bool createFailed() { return failed; }

	void use(bool activate = true);
	GLuint getId();

	void setUniform4f(const char* uniform, float red, float green, float blue, float alpha);
	void setUniform3f(const char* uniform, float red, float green, float blue);
	void setUniform3f(const char* uniform, const glm::vec3& values);
	void setUniform1f(const char* uniformName, float value);
	void setUniform1i(const char* uniformname, int newValue);
	void setUniformMatrix4fv(const char* uniform, int numberMatrices, GLuint normalize, const float* data);

	void addTexture(std::shared_ptr<Texture2D>& texture, const std::string& textureSampleName);
	void activateTextures();


private:
	bool failed;
	bool active;
	GLuint linkedProgram;

	std::vector<std::shared_ptr<Texture2D>> textures;

private:
	bool shaderCompileSuccess(GLuint shaderID);
	bool programLinkSuccess(GLuint programID);
};

