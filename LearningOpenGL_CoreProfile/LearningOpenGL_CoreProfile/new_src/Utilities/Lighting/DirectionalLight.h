#pragma once
#include "LightBase.h"

class DirectionalLight : public LightBase
{
	glm::vec3 direction;
	std::string glslDirectionStr;
public:
	DirectionalLight(
		const glm::vec3& ambientIntensity, 
		const glm::vec3& diffuseIntensity,
		const glm::vec3& specularIntensity,
		const glm::vec3& direction, std::string uniformName, bool InArray = false, unsigned int arrayIndex = 0);
	~DirectionalLight();

	virtual void applyUniforms(Shader& shader) override;
};

