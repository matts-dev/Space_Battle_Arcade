#include "SAAssetSystem.h"

#include <iostream>
#include <stdio.h>

#include "../../../../Libraries/stb_image.h"
#include "../Tools/ModelLoading/SAModel.h"
#include "../Rendering/OpenGLHelpers.h"
#include "../Rendering/Camera/Texture_2D.h"
#include <dr_lib/dr_wav.h>
#include "Audio/SoundRawData.h"
#include "Audio/OpenALUtilities.h"
#include "SAAudioSystem.h"
#include "SAGameBase.h"
#include "SALog.h"

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

	AssetHandle<SoundRawData> AssetSystem::loadSound(const std::string& relative_filepath)
	{
		SoundRawData loadedData;
		drwav_int16* pSampleData = drwav_open_file_and_read_pcm_frames_s16(relative_filepath.c_str(), &loadedData.channels, &loadedData.sampleRate, &loadedData.totalPCMFrameCount, nullptr);

		bool bSuccess = true;
		if (pSampleData == nullptr)
		{
			std::cerr << "failed to load audio file" << std::endl;
			bSuccess = false;
		}
		if (loadedData.getTotalSamples() > drwav_uint64(std::numeric_limits<size_t>::max()))
		{
			std::cerr << "too much data in file for 32bit addressed vector" << std::endl;
			bSuccess = false;
		}

		sp<SoundRawData> loadedDataPtr = nullptr;

		if (bSuccess)
		{
			loadedData.pcmData.resize(size_t(loadedData.getTotalSamples()));
			std::memcpy(loadedData.pcmData.data(), pSampleData, loadedData.pcmData.size() * /*twobytes_in_s16*/2);
			loadedDataPtr = new_sp<SoundRawData>(std::move(loadedData));
			//loadedData should now be considered empty!

			//sample rate is samples per sec (generally 44100 hz)
			//total frame count should be the same regardless if it is mono,stereo, etc.
			loadedDataPtr->durationSec = float(loadedDataPtr->sampleRate) * float(loadedDataPtr->totalPCMFrameCount);

			loadedSoundPcmData.insert({ relative_filepath, loadedDataPtr });
		}
		else
		{
			logf_sa(__FUNCTION__, LogLevel::LOG_WARNING, "Failed to load sound %s", relative_filepath.c_str());
			STOP_DEBUGGER_HERE();
		}

		drwav_free(pSampleData, /*allocation callbacks*/nullptr);

		return loadedDataPtr;
	}

	AssetHandle<SoundRawData> AssetSystem::getSound(const std::string& relative_filepath)
	{
		auto findIter = loadedSoundPcmData.find(relative_filepath);
		if (findIter != loadedSoundPcmData.end())
		{
			const sp<SoundRawData>& foundResult = findIter->second;
			return foundResult;
		}

		return nullptr;
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

#ifdef USE_OPENAL_API
	ALBufferWrapper AssetSystem::loadOpenAlBuffer(const std::string& relative_filepath)
	{
		auto previousLoad = assetPathToloadedAlBuffers.find(relative_filepath);
		if (previousLoad != assetPathToloadedAlBuffers.end())
		{
			return previousLoad->second;
		}
		else
		{
			AudioSystem& audioSystem = GameBase::get().getAudioSystem();
			if (audioSystem.hasValidOpenALDevice())
			{
				AssetHandle<SoundRawData> soundPcmAsset = loadSound(relative_filepath);
				if (const SA::SoundRawData* soundData = soundPcmAsset.getAsset())
				{
					ALBufferWrapper alWrapper = {};
					alWrapper.durationSec = soundData->durationSec;

					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					//clear previous errors so we can safely read if we had an error creating a buffer
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					constexpr size_t numAlErrorsBeforeGiveup = 100;
					size_t errorNumber = 0;
					ALenum error = alGetError(); //clear any error
					while (error != AL_NO_ERROR && errorNumber < numAlErrorsBeforeGiveup)
					{
						//log that we had a previous error before we attempted to fill a buffer
						logf_sa(__FUNCTION__, LogLevel::LOG_WARNING, "previous OpenAL errors before attempting to create buffer! %d", int(error));
						error = alGetError(); //clear any error
						++errorNumber;
					}

					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					//create buffer
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					//manual error checking so we know if we created a buffer
					alGenBuffers(1, &alWrapper.buffer);
					error = alGetError(); //see if creating buffer threw an error

					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// fill buffer
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					if (error == AL_NO_ERROR)
					{
						alBufferData(alWrapper.buffer,
							soundData->channels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
							soundData->pcmData.data(),
							soundData->pcmData.size() * 2 /*two bytes per sample*/,
							soundData->sampleRate
						);
						error = alGetError(); //see if we failed to fill the buffer
						if (error == AL_NO_ERROR)
						{
							assetPathToloadedAlBuffers.insert({ relative_filepath, alWrapper});
							return alWrapper;
						}
						else
						{
							alec(alDeleteBuffers(1, &alWrapper.buffer)); //clean up buffer since we created it but couldn't populate it
							logf_sa(__FUNCTION__, LogLevel::LOG_WARNING, "failed to populate AL buffer %d", int(error));
						}
					}
					else
					{
						logf_sa(__FUNCTION__, LogLevel::LOG_WARNING, "failed to create AL buffer %d", int(error));
					}
				}
			}
		}

		ALBufferWrapper nullBuffer;
		return nullBuffer;
	}

	bool AssetSystem::unloadOpenALBuffer(const std::string& relative_filepath)
	{
		auto previousLoad = assetPathToloadedAlBuffers.find(relative_filepath);
		if (previousLoad != assetPathToloadedAlBuffers.end())
		{
			ALuint alBuffer = previousLoad->second.buffer;
			alec(alDeleteBuffers(1, &alBuffer))

			assetPathToloadedAlBuffers.erase(relative_filepath);

			return true;
		}
		return false;
	}

	void AssetSystem::unloadAllOpenALBuffers()
	{
		log(__FUNCTION__, LogLevel::LOG, "Cleaning up audio buffers from asset system");
		for (auto& kv_pair : assetPathToloadedAlBuffers)
		{
			ALuint buffer = kv_pair.second.buffer;
			alec(alDeleteBuffers(1, &buffer));
		}
		assetPathToloadedAlBuffers.clear();
		log(__FUNCTION__, LogLevel::LOG, "complete");
	}

#endif //USE_OPENAL_API

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