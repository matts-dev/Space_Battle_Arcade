#pragma once
#include<string>
#include<memory>
#include<vector>

#include<glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Tools/RemoveSpecialMemberFunctionUtils.h"
#include "Rendering/SAGPUResource.h"
#include <optional>

class Texture2D;

namespace SA
{
	struct ShaderInit
	{
		std::optional<std::string> vertexShaderSrc;
		std::optional<std::string> geometryShaderSrc;
		std::optional<std::string> fragmentShaderSrc;
		bool bStringsAreFilePaths = false;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Represents a hardware shader program (eg, the combination of verex, fragment, and other shaders
	//
	// known issues/lack of features
	//		-uniforms do not persist if context information is lost, or if uniforms are assigned before context is present.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Shader : public GPUResource, public RemoveCopies, public RemoveMoves
	{
	private:
		void constructorSharedInitialization(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath, const std::string& geometryShaderFilePath, bool stringsAreFilePaths = true);
		virtual void onReleaseGPUResources();
		virtual void onAcquireGPUResources();
	public:
		explicit Shader(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath, bool stringsAreFilePaths = true);
		explicit Shader(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath, const std::string& geometryShaderFilePath, bool stringsAreFilePaths = true);
		virtual ~Shader();

		bool createFailed() { return failed; }

		void use(bool activate = true);
		GLuint getId();

		void setUniform4f(const char* uniform, float red, float green, float blue, float alpha);
		void setUniform4f(const char* uniform, const glm::vec4& values);
		void setUniform3f(const char* uniform, float red, float green, float blue);
		void setUniform3f(const char* uniform, const glm::vec3& values);
		void setUniform1f(const char* uniformName, float value);
		void setUniform1i(const char* uniformname, int newValue);
		void setUniformMatrix4fv(const char* uniform, int numberMatrices, GLuint normalize, const float* data);

	private:
		bool failed;
		bool active;
		GLuint linkedProgram;
	private:
		ShaderInit initData;
	private:
		bool shaderCompileSuccess(GLuint shaderID);
		bool programLinkSuccess(GLuint programID);
	};
}
