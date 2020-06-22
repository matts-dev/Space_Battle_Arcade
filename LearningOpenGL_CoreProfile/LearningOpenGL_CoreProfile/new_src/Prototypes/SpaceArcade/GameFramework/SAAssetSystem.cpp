#include "SAAssetSystem.h"

#include <iostream>
#include <stdio.h>

#include "../../../../Libraries/stb_image.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../Rendering/OpenGLHelpers.h"
#include "../Rendering/Camera/Texture_2D.h"

namespace SA
{
	void AssetSystem::shutdown()
	{
		for (const auto& textureMapIter : loadedTextureIds)
		{
			GLuint textureId = textureMapIter.second;
			ec(glDeleteTextures(1, &textureId));
		}
		loadedTextureIds.clear();

		//Early clean up of resources before OpenGL context is destroyed; we cannot just destroy models here as someone may be holding shared ptr.
		for (const auto& modelMapIter : loadedModel3Ds)
		{
			modelMapIter.second->releaseGPUData();
		}
	}

	sp<Model3D> AssetSystem::loadModel(const char* relative_filepath)
	{
		auto loadedModelIter = loadedModel3Ds.find(relative_filepath);
		if (loadedModelIter != loadedModel3Ds.end())
		{
			return loadedModelIter->second;
		}
		else
		{
			try //eat failures to load models here and signal failure by returning nullptr
			{
				sp<Model3D> loadedModel = new_sp<Model3D>(relative_filepath); //may fail; #TODO handling failures at model level would be nice
				loadedModel3Ds[relative_filepath] = loadedModel;
				return loadedModel;
			}
			catch (std::runtime_error & )
			{
				return nullptr;
			}
		}

		//returning by value so this is safe; be careful when returning nullptr directly when returning reference to a sp
		return nullptr;
	}

	sp<SA::Model3D> AssetSystem::loadModel(const std::string& relative_filepath)
	{
		return loadModel(relative_filepath.c_str());
	}

	SA::sp<SA::Model3D> AssetSystem::getModel(const std::string& key) const
	{
		const auto& iter = loadedModel3Ds.find(key);
		if (iter != loadedModel3Ds.end())
		{
			return iter->second;
		}
		else
		{
			return nullptr;
		}
	}

	sp<SA::Texture_2D> AssetSystem::getNullBlackTexture() const
	{
		static sp<Texture_2D> nullTexture = new_sp<Texture_2D>(glm::vec3(0.f));
		return nullTexture;
	}

	bool AssetSystem::loadTexture(const char* relative_filepath, GLuint& outTexId, int texture_unit /*= -1*/, bool useGammaCorrection /*= false*/)
	{
		//# TODO upgrade 3d model class to use this; but care will need to be taken so that textures are deleted after models
		auto previousLoadTextureIter = loadedTextureIds.find(relative_filepath);
		if (previousLoadTextureIter != loadedTextureIds.end())
		{
			outTexId = previousLoadTextureIter->second;
			return true;
		}

		int img_width, img_height, img_nrChannels;
		unsigned char* textureData = stbi_load(relative_filepath, &img_width, &img_height, &img_nrChannels, 0);
		if (!textureData)
		{
			std::cerr << "failed to load texture" << relative_filepath << std::endl;
			return false;
		}

		bool bSuccess = loadTexture_internal(textureData, img_width, img_height, img_nrChannels, relative_filepath, outTexId, texture_unit, useGammaCorrection);

		stbi_image_free(textureData);
		loadedTextureIds.insert({ relative_filepath, outTexId });

		return bSuccess;
	}

	bool AssetSystem::loadTexture(glm::vec3 solidColor, GLuint& outTexId, int texture_unit /*= -1*/, bool useGammaCorrection /*= false*/)
	{
		//convert to integer based color
		using byte = unsigned char;

		byte rgb[] = { byte(solidColor.r * 255.f), byte(solidColor.g * 255.f), byte(solidColor.b * 255.f) };

		char textBuffer[1024];
		snprintf(textBuffer, sizeof(textBuffer), "solidColor[%d,%d,%d]", rgb[0], rgb[1], rgb[2]);

		auto previousLoadTextureIter = loadedTextureIds.find(std::string(textBuffer));
		if (previousLoadTextureIter != loadedTextureIds.end())
		{
			outTexId = previousLoadTextureIter->second;
			return true;
		}

		bool bSuccess = loadTexture_internal(rgb, 1, 1, 3, textBuffer, outTexId, texture_unit, useGammaCorrection);

		loadedTextureIds.insert({ std::string(textBuffer), outTexId });

		return bSuccess;
	}

	bool AssetSystem::loadTexture_internal(unsigned char* textureData, int img_width, int img_height, int img_nrChannels, const char* relative_filepath, GLuint& outTexId, int texture_unit /*= -1*/, bool useGammaCorrection /*= false*/)
	{
		GLuint textureID;
		ec(glGenTextures(1, &textureID));

		if (texture_unit >= 0)
		{
			ec(glActiveTexture(texture_unit));
		}
		ec(glBindTexture(GL_TEXTURE_2D, textureID));

		int mode = -1;
		int dataFormat = -1;
		if (img_nrChannels == 3)
		{
			mode = useGammaCorrection ? GL_SRGB : GL_RGB;
			dataFormat = GL_RGB;
		}
		else if (img_nrChannels == 4)
		{
			mode = useGammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
			dataFormat = GL_RGBA;
		}
		else if (img_nrChannels == 1)
		{
			mode = GL_RED;
			dataFormat = GL_RED;
		}
		else
		{
			std::cerr << "unsupported image format for texture at " << relative_filepath << " there are " << img_nrChannels << "channels" << std::endl;
			stbi_image_free(textureData);
			ec(glDeleteTextures(1, &textureID));
			return false;
		}

		ec(glTexImage2D(GL_TEXTURE_2D, 0, mode, img_width, img_height, 0, dataFormat, GL_UNSIGNED_BYTE, textureData));
		ec(glGenerateMipmap(GL_TEXTURE_2D));
		//ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT)); //causes issue with materials on models
		//ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT)); //causes issue with materials on models
		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		ec(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));

		outTexId = textureID;
		return true;
	}
}