#pragma once

#include<string>
#include<memory>
#include<vector>

class Texture2D;

class Shader
{
public:
	explicit Shader(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath);
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
	void setUniform1f(const char* uniformName, float value);
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

