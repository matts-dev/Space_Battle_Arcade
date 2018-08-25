#include "PointLight.h"


PointLight::PointLight(
	const glm::vec3& ambientIntensity,
	const glm::vec3& diffuseIntensity,
	const glm::vec3& specularIntensity,
	const float attenuationConstant,
	const float attenuationLinear,
	const float attenuationQuadratic,
	const glm::vec3& position,
	std::string uniformName, bool InArray/* = false*/, unsigned int arrayIndex /*= 0*/) 
	: LightBase(ambientIntensity, diffuseIntensity, specularIntensity, uniformName, InArray, arrayIndex),
	attenuationConstant(attenuationConstant),
	attenuationLinear(attenuationLinear),
	attenuationQuadratic(attenuationQuadratic)
{
	setPosition(position);

	//perhaps have static fields that allow glsl uniform names to be configurable; 
	//basically update a static field before construction and use static field below in place of "position"
	glslPositionStr = glslAccessString + "position";
	glslDirectionStr = glslAccessString + "direction";
	glslCutOffStr = glslAccessString + "cos_cutoff";
	glslOuterCutOffStr = glslAccessString + "cos_outercutoff";
	glslAttenuationConstantStr = glslAccessString + "constant";
	glslAttenuationLinearStr = glslAccessString + "linear";
	glslAttenuationQuadraticString = glslAccessString + "quadratic";
}


PointLight::~PointLight()
{
}

void PointLight::applyUniforms(Shader& shader)
{
	LightBase::applyUniforms(shader);

	shader.setUniform3f(glslPositionStr.c_str(), getPosition());
	shader.setUniform1f(glslAttenuationConstantStr.c_str(), attenuationConstant);
	shader.setUniform1f(glslAttenuationLinearStr.c_str(), attenuationLinear);
	shader.setUniform1f(glslAttenuationQuadraticString.c_str(), attenuationQuadratic);
}
