#include "ConeLight.h"

ConeLight::ConeLight(
	const glm::vec3& ambientIntensity,
	const glm::vec3& diffuseIntensity,
	const glm::vec3& specularIntensity,
	const float cosineCutOffAngle,
	const float cosineOuterCutOffAngle,
	const float attenuationConstant,
	const float attenuationLinear,
	const float attenuationQuadratic,
	const glm::vec3& position,
	const glm::vec3& direction,
	std::string uniformName, bool InArray/* = false*/, unsigned int arrayIndex /*= 0*/)
	: LightBase (ambientIntensity, diffuseIntensity, specularIntensity, uniformName, InArray, arrayIndex),
	cosineCutOffAngle(cosineCutOffAngle), cosineOuterCutOffAngle(cosineOuterCutOffAngle),
	attenuationConstant(attenuationConstant),
	attenuationLinear(attenuationLinear),
	attenuationQuadratic(attenuationQuadratic),
	direction(direction)
{
	setPosition(position);

	glslPositionStr = glslAccessString + "position";
	glslDirectionStr = glslAccessString + "direction";
	glslCutOffStr = glslAccessString + "cos_cutoff";
	glslOuterCutOffStr = glslAccessString + "cos_outercutoff";
	glslAttenuationConstantStr = glslAccessString + "constant";
	glslAttenuationLinearStr = glslAccessString + "linear";
	glslAttenuationQuadraticString = glslAccessString + "quadratic";
}


ConeLight::~ConeLight()
{
}

void ConeLight::applyUniforms(Shader& shader)
{
	LightBase::applyUniforms(shader);

	shader.setUniform3f(glslPositionStr.c_str(), getPosition());
	shader.setUniform3f(glslDirectionStr.c_str(), direction);
	shader.setUniform1f(glslCutOffStr.c_str(), cosineCutOffAngle);
	shader.setUniform1f(glslOuterCutOffStr.c_str(), cosineOuterCutOffAngle);
	shader.setUniform1f(glslAttenuationConstantStr.c_str(), attenuationConstant);
	shader.setUniform1f(glslAttenuationLinearStr.c_str(), attenuationLinear);
	shader.setUniform1f(glslAttenuationQuadraticString.c_str(), attenuationQuadratic);
}
