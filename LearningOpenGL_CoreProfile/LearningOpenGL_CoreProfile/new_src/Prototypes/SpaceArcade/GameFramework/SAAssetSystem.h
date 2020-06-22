#pragma once
#include "SASystemBase.h"
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <set>
#include <map>
#include <string>
#include "../Tools/DataStructures/SATransform.h" //glm

namespace SA
{
	class Model3D;
	class Texture_2D;

	class AssetSystem : public SystemBase
	{
	public:
		sp<Model3D> loadModel(const char* relative_filepath);
		sp<Model3D> loadModel(const std::string& relative_filepath);
		sp<Model3D> getModel(const std::string& key) const;
		sp<Texture_2D> getNullBlackTexture() const;

		bool loadTexture(const char* relative_filepath, GLuint& outTexId, int texture_unit = -1, bool useGammaCorrection = false);
		bool loadTexture(glm::vec3 solidColor, GLuint& outTexId, int texture_unit = -1, bool useGammaCorrection = false);
	private:
		bool loadTexture_internal(unsigned char* textureDataBytes, int img_width, int img_height, int img_nrChannels, const char* relative_filepath, GLuint& outTexId, int texture_unit = -1, bool useGammaCorrection = false);
	private:
		virtual void shutdown() override;
		virtual void tick(float deltaSec) {};
	private:
		std::map<std::string, sp<Model3D>> loadedModel3Ds;
		std::map<std::string, GLuint> loadedTextureIds;
	};
}