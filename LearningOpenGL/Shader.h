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

	void use();

private:
	bool failed;
	GLuint linkedProgram;

private:
	bool shaderCompileSuccess(GLuint shaderID);
	bool programLinkSuccess(GLuint programID);

};

