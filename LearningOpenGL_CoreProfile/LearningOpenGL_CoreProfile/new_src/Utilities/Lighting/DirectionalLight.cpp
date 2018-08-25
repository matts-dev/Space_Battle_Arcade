#include "DirectionalLight.h"



DirectionalLight::DirectionalLight(
	const glm::vec3& ambientIntensity,
	const glm::vec3& diffuseIntensity,
	const glm::vec3& specularIntensity, 
	const glm::vec3& direction,
	std::string uniformName, bool InArray /*= false*/, unsigned int arrayIndex/*= 0*/)
	: LightBase(ambientIntensity, diffuseIntensity, specularIntensity, uniformName, InArray, arrayIndex),
	direction(direction)
{
	glslDirectionStr = glslAccessString + "direction";
}


DirectionalLight::~DirectionalLight()
{
	
}

void DirectionalLight::applyUniforms(Shader& shader)
{
	LightBase::applyUniforms(shader);
	shader.setUniform3f(glslDirectionStr.c_str(), direction);
}
