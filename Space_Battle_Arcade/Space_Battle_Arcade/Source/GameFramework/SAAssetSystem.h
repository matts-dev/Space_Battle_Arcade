#pragma once

#include "BuildConfiguration/SAPreprocessorDefines.h"
#ifdef USE_OPENAL_API
#include<AL/al.h>
#include "Audio/ALBufferWrapper.h"
#endif

#include "SASystemBase.h"
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <set>
#include <map>
#include <string>
#include "Tools/DataStructures/SATransform.h" //glm
#include "AssetManagement/AssetHandle.h"

namespace SA
{
	class Model3D;
	class Texture_2D;
	struct SoundRawData;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// System for managing load/unload of game assets such as models, textures, and sounds.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class AssetSystem : public SystemBase
	{
	public:
		sp<Model3D> loadModel(const char* relative_filepath);
		sp<Model3D> loadModel(const std::string& relative_filepath);
		sp<Model3D> getModel(const std::string& key) const;
		AssetHandle<SoundRawData> loadSound(const std::string& relative_filepath);
		AssetHandle<SoundRawData> getSound(const std::string& relative_filepath);
		sp<Texture_2D> getNullBlackTexture() const;

		bool loadTexture(const char* relative_filepath, GLuint& outTexId, int texture_unit = -1, bool useGammaCorrection = false);
		bool loadTexture(glm::vec3 solidColor, GLuint& outTexId, int texture_unit = -1, bool useGammaCorrection = false);

#ifdef USE_OPENAL_API
		ALBufferWrapper loadOpenAlBuffer(const std::string& relative_filepath);
		bool unloadOpenALBuffer(const std::string& relative_filepath);
		void unloadAllOpenALBuffers();
#endif
	private:
		bool loadTexture_internal(unsigned char* textureDataBytes, int img_width, int img_height, int img_nrChannels, const char* relative_filepath, GLuint& outTexId, int texture_unit = -1, bool useGammaCorrection = false);
	private:
		virtual void shutdown() override;
		virtual void tick(float deltaSec) {};
	private:
		std::map<std::string, sp<Model3D>> loadedModel3Ds;
		std::map<std::string, GLuint> loadedTextureIds; //open question as to whether asset system should be managing API memory
		std::map<std::string, sp<SoundRawData>> loadedSoundPcmData;
#ifdef USE_OPENAL_API
		std::map<std::string, ALBufferWrapper> assetPathToloadedAlBuffers;
#endif
	};
}