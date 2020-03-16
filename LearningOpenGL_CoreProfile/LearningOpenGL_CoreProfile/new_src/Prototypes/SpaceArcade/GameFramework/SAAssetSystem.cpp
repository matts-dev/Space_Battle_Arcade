#include "SAAssetSystem.h"

#include <iostream>
#include "../../../../Libraries/stb_image.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../Rendering/OpenGLHelpers.h"

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
		stbi_image_free(textureData);

		loadedTextureIds.insert({ relative_filepath, textureID });

		outTexId = textureID;
		return true;
	}
}