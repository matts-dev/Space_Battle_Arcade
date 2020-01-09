#include "SALightBase.h"
#include <string>
#include "../SAShader.h"

namespace SA
{
	//LightBase::LightBase(const LightBase::Config& inConfig) : 
	//	config(inConfig)
	//{
	//	config.uniformName = config.uniformName;
	//	if (config.InArray)
	//	{
	//		config.uniformName += "[";
	//		config.uniformName += std::to_string(config.arrayIndex);
	//		config.uniformName += "]";
	//	}

	//	lightIntensityUniformName = config.uniformName + ".";
	//	lightIntensityUniformName += "intensity";
	//}

	//void LightBase::applyUniforms(Shader& shader)
	//{
	//	shader.setUniform3f(lightIntensityUniformName.c_str(), config.lightIntensity);
	//}

}
