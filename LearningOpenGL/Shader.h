#pragma once

#include<string>

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

	void setFloatUniform(const char* uniform, float red, float green, float blue, float alpha);

private:
	bool failed;
	bool active;
	GLuint linkedProgram;

private:
	bool shaderCompileSuccess(GLuint shaderID);
	bool programLinkSuccess(GLuint programID);

};

