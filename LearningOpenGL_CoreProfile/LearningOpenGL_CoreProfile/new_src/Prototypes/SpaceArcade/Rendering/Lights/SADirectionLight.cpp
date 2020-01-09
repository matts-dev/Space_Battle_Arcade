#include<cstdint>
#include<assert.h>

#include "SADirectionLight.h"
#include "../SAShader.h"
#include "../../GameFramework/SAGameBase.h"


namespace SA
{


	//DirectionLight::DirectionLight(const DirectionLight::Config& inDirConfig, const LightBase::Config& lbConfig)
	//	: LightBase(lbConfig),
	//	dirConfig(inDirConfig)
	//{
	//	directionUniformName = config.uniformName + ".dir";
	//}

	//void DirectionLight::applyUniforms(Shader& shader)
	//{
	//	LightBase::applyUniforms(shader);
	//	shader.setUniform3f(directionUniformName.c_str(), config.lightIntensity);
	//}

	void DirectionLight::applyToShader(Shader& shader, size_t index) const
	{
		using LUT = std::vector<std::string>;
		static LUT uniformNameLUT_Dir;
		static LUT uniformNameLUT_Intensity;
		static int oneTimeInit = [](LUT& dir, LUT& intensity)
		{
			std::vector<std::string> LUT;
			uint32_t numToGenerate = 16;

			assert(numToGenerate >= GameBase::getConstants().MAX_DIR_LIGHTS);
			assert(dir.size() == 0 && intensity.size() == 0);

			for (uint32_t idx = 0; idx < numToGenerate; ++idx)
			{
				std::string uniformName = "dirLights[";
				uniformName += std::to_string(idx);
				uniformName += "]";

				std::string intensityName = uniformName + ".";
				intensityName += "intensity";//dirLights[0].intensity;

				std::string dirName = uniformName + ".";
				dirName += "dir_n"; //dirLights[0].dir_n;

				dir.push_back(dirName);
				intensity.push_back(intensityName);
			}
			return 0;
		}(uniformNameLUT_Dir, uniformNameLUT_Intensity);

		shader.setUniform3f(uniformNameLUT_Dir[index].c_str(), direction_n);
		shader.setUniform3f(uniformNameLUT_Intensity[index].c_str(), lightIntensity);
	}

}

