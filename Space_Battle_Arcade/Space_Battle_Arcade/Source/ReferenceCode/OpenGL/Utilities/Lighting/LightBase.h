#pragma once
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ReferenceCode/OpenGL/Utilities/Transformable.h"

class LightBase : public Transformable
{
protected:
	bool InArray = false;
	std::string uniformName;
	std::string glslAccessString;
	std::string glslAmbientStr;
	std::string glslDiffuseStr;
	std::string glslSpecularStr;

	//these are strength values, but may also contain color information
	glm::vec3 ambientIntensity;
	glm::vec3 diffuseIntensity;
	glm::vec3 specularIntensity;

public:
	LightBase(
		const glm::vec3& ambientIntensity,
		const glm::vec3& diffuseIntensity,
		const glm::vec3& specularIntensity, 
		std::string uniformName, bool InArray = false, unsigned int arrayIndex = 0);
	virtual ~LightBase(); 
	virtual void applyUniforms(Shader& shader);
};

