#pragma once
#include "LightBase.h"
class PointLight : public LightBase
{
protected:
	float attenuationConstant;
	float attenuationLinear;
	float attenuationQuadratic;

	std::string glslPositionStr;
	std::string glslDirectionStr;
	std::string glslCutOffStr;
	std::string glslOuterCutOffStr;
	std::string glslAttenuationConstantStr;
	std::string glslAttenuationLinearStr;
	std::string glslAttenuationQuadraticString;

public:
	PointLight(
		const glm::vec3& ambientIntensity,
		const glm::vec3& diffuseIntensity,
		const glm::vec3& specularIntensity,
		const float attenuationConstant,
		const float attenuationLinear,
		const float attenuationQuadratic,
		const glm::vec3& position,
		std::string uniformName, bool InArray = false, unsigned int arrayIndex = 0);
	virtual ~PointLight();

	void applyUniforms(Shader& shader);
};

