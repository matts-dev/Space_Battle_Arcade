#pragma once
#include "../../GameFramework/SAGameEntity.h"
#include <string>
#include <cstdint>
#include "../../Tools/DataStructures/SATransform.h"
#include "../../Tools/RemoveSpecialMemberFunctionUtils.h"

namespace SA
{
	class Shader;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The base class for lights
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//class LightBase : public GameEntity, public RemoveCopies, public RemoveMoves
	//{
	//public:
	//	struct Config
	//	{
	//		std::string uniformName;
	//		glm::vec3 lightIntensity; 
	//		bool InArray = false;
	//		uint32_t arrayIndex = 0;
	//	};
	//public:
	//	LightBase(const LightBase::Config& inConfig);
	//	virtual ~LightBase();
	//public:
	//	virtual void applyUniforms(Shader& shader);
	//public:
	//	void setLightIntensity(const glm::vec3& newIntensity) { config.lightIntensity = newIntensity; }
	//	glm::vec3 getLightIntensity() { return config.lightIntensity; }
	//protected:
	//	Config config;
	//	std::string lightIntensityUniformName;
	//};
}
