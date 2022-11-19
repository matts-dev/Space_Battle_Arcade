#pragma once

#include "SALightBase.h"
#include "Tools/DataStructures/SATransform.h"

namespace SA
{
	class Shader;

	struct DirectionLight
	{
		glm::vec3 lightIntensity;
		glm::vec3 direction_n;

		void applyToShader(Shader& shader, size_t index) const;
	};



	//class DirectionLight : public LightBase
	//{
	//public:
	//	struct Config
	//	{
	//		//defaults create a black light
	//		glm::vec3 direction = glm::vec3(1.0f, 0.f, 0.f);
	//	};
	//	DirectionLight(
	//		const DirectionLight::Config& dirConfig, 
	//		const LightBase::Config& lbConfig
	//	);
	//public:
	//	virtual void applyUniforms(Shader& shader) override;
	//public:
	//	void setDirection(glm::vec3 dir) { dirConfig.direction = dir; }
	//	glm::vec3 getDirection() { return dirConfig.direction; }
	//private:
	//	Config dirConfig;
	//	std::string directionUniformName;

	//};

}
