#include "LightBase.h"



LightBase::LightBase(
	const glm::vec3& ambientIntensity, 
	const glm::vec3& diffuseIntensity, 
	const glm::vec3& specularIntensity, 
	std::string uniformName, bool InArray /*= false*/, unsigned int arrayIndex/*= 0*/) :
	ambientIntensity(ambientIntensity),
	diffuseIntensity(diffuseIntensity),
	specularIntensity(specularIntensity),
	uniformName(uniformName),
	InArray(InArray)
{
	glslAccessString = uniformName;
	if (InArray)
	{
		glslAccessString += "[";
		glslAccessString += std::to_string(arrayIndex);
		glslAccessString += "]";
	}
	glslAccessString += ".";
	glslAmbientStr  = glslAccessString + "ambientIntensity";
	glslDiffuseStr  = glslAccessString + "diffuseIntensity";
	glslSpecularStr = glslAccessString + "specularIntensity";
}


LightBase::~LightBase()
{

}

void LightBase::applyUniforms(Shader& shader)
{
	shader.setUniform3f(glslAmbientStr.c_str(), ambientIntensity);
	shader.setUniform3f(glslDiffuseStr.c_str(), diffuseIntensity);
	shader.setUniform3f(glslSpecularStr.c_str(), specularIntensity);
}
