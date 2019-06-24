#pragma once
#include "SASubsystemBase.h"
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <set>
#include <map>
#include <string>

namespace SA
{
	class Model3D;

	class AssetSubsystem : public SubsystemBase
	{
	public:
		sp<Model3D> loadModel(const char* relative_filepath);
		sp<Model3D> getModel(const std::string& key) const;

		bool loadTexture(const char* relative_filepath, GLuint& outTexId, int texture_unit = -1, bool useGammaCorrection = false);
	private:
		virtual void shutdown() override;
		virtual void tick(float deltaSec) {};
	private:
		std::map<std::string, sp<Model3D>> loadedModel3Ds;
		std::set<GLuint> loadedTextureIds;
	};
}