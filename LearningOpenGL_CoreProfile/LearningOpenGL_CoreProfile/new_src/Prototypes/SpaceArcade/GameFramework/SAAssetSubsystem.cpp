#include "SAAssetSubsystem.h"

#include <iostream>
#include "..\..\..\..\Libraries\stb_image.h"
#include "..\Tools\ModelLoading\SAModel.h"

namespace SA
{
	void AssetSubsystem::shutdown()
	{
		for (GLuint textureId : loadedTextureIds)
		{
			glDeleteTextures(1, &textureId);
		}
		loadedTextureIds.clear();
	}

	sp<Model3D> AssetSubsystem::loadModel(const char* relative_filepath)
	{
		auto loadedModelIter = loadedModel3Ds.find(relative_filepath);
		if (loadedModelIter != loadedModel3Ds.end())
		{
			return loadedModelIter->second;
		}
		else
		{
			sp<Model3D> loadedModel = new_sp<Model3D>(relative_filepath);
			loadedModel3Ds[relative_filepath] = loadedModel;

			return loadedModel;
		}

		//returning by value so this is safe; be careful when returning nullptr directly when returning reference to a sp
		return nullptr;
	}

	SA::sp<SA::Model3D> AssetSubsystem::getModel(const std::string& key) const
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

	bool AssetSubsystem::loadTexture(const char* relative_filepath, GLuint& outTexId, int texture_unit /*= -1*/, bool useGammaCorrection /*= false*/)
	{
		//TODO upgrade 3d model class to use this; but care will need to be taken so that textures are deleted after models
		int img_width, img_height, img_nrChannels;
		unsigned char* textureData = stbi_load(relative_filepath, &img_width, &img_height, &img_nrChannels, 0);
		if (!textureData)
		{
			std::cerr << "failed to load texture" << relative_filepath << std::endl;
			return false;
		}

		GLuint textureID;
		glGenTextures(1, &textureID);

		if (texture_unit >= 0)
		{
			glActiveTexture(texture_unit);
		}
		glBindTexture(GL_TEXTURE_2D, textureID);

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
			glDeleteTextures(1, &textureID);
			return false;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, mode, img_width, img_height, 0, dataFormat, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); //causes issue with materials on models
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT); //causes issue with materials on models
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		stbi_image_free(textureData);

		loadedTextureIds.insert(textureID);

		outTexId = textureID;
		return true;
	}
}