#pragma once
#include "LightBase.h"
class ConeLight : public LightBase
{
protected:
	float cosineOuterCutOffAngle;
	float cosineCutOffAngle;
	float attenuationConstant;
	float attenuationLinear;
	float attenuationQuadratic;
	glm::vec3 direction;

	std::string glslPositionStr;
	std::string glslDirectionStr;
	std::string glslCutOffStr;
	std::string glslOuterCutOffStr;
	std::string glslAttenuationConstantStr;
	std::string glslAttenuationLinearStr;
	std::string glslAttenuationQuadraticString;

public:
	/**
	*	@param cosineCutOffAngle cosine(radians(inner circle cutoff angle))
	*	@param cosineInnerCutOffAngle cosine(radians(outter circle cutoff angle))
	*/
	ConeLight(
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
		std::string uniformName, bool InArray = false, unsigned int arrayIndex = 0);
	virtual ~ConeLight();

	virtual void applyUniforms(Shader& shader) override;

	void setDirection(const glm::vec3& newDirection) { direction = newDirection; }
};

