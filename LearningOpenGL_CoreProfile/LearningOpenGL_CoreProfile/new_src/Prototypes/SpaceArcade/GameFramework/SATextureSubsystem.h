#pragma once
#include "SASubsystemBase.h"
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <set>

namespace SA
{
	class TextureSubsystem : public SubsystemBase
	{
	public:
		bool loadTexture(const char* relative_filepath, GLuint& outTexId, int texture_unit = -1, bool useGammaCorrection = false);
	private:
		virtual void shutdown() override;
		virtual void tick(float deltaSec) {};
	private:
		std::set<GLuint> loadedTextureIds;
	};
}