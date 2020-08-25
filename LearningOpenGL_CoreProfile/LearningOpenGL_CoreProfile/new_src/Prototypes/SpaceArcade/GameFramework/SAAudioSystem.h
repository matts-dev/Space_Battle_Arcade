#pragma once

#include "BuildConfiguration/SAPreprocessorDefines.h"
#if USE_OPENAL_API
#include "AL/al.h"
#include <AL/alc.h>
#endif

#include<string>
#include <set>
#include <vector>

#include "SASystemBase.h"
#include "../Tools/DataStructures/SATransform.h"
#include <unordered_map>
#include <optional>
#include "../Tools/Algorithms/AmortizeLoopTool.h"
#include "../Tools/DataStructures/ObjectPools.h"

#define COMPILE_AUDIO 1

namespace SA
{
	class LevelBase;


	enum class AudioEmitterPriority : uint8_t
	{
		GAMEPLAY_CRITICAL, 
		MUSIC_OR_AMBIENCE,
		MENU,
		GAMEPLAY_PLAYER,
		GAMEPLAY_COMBAT,
		GAMEPLAY_AMBIENT,
	};

	struct EmitterUserData
	{
		std::string sfxAssetPath = "";
		float priority = float(AudioEmitterPriority::GAMEPLAY_AMBIENT);
		glm::vec3 position; 
		glm::vec3 velocity = glm::vec3(0.f);
		float pitch = 1.f;
		float volume = 1.f; 
		float maxRadius = 10.f;
		bool bLooping = false;
		bool bIsMusic = false;
	};

	struct EmitterHardwareData
	{
#if USE_OPENAL_API
		std::optional<ALuint> bufferIdx;
		std::optional<ALuint> sourceIdx;
#endif
	};

	struct EmitterAudioSystemMetaData
	{
		float calculatedPriority = 0.f;
		float soundFadeModifier = 1.f;
		float fadeDirection = 0.f; //-1=fade down; 0=nofade; 1=fade up
		float fadeRateSecs = 0.25f;
		float referenceDistance = 1.f;
		bool bActive = false;
		bool bPreviousFrameListenerMapped = false;
		bool bOutOfRange = false;
		bool bPlaying = false;
		bool dirty_bPosition = true;
		bool dirty_bVelocity = true;
		bool dirty_bPitch = true;
		bool dirty_bGain = true;
		bool dirty_bLooping = true;
		bool dirty_bReferenceDistance = true;
		size_t closestListenerIdx = 0;
		glm::vec3 position_listenerCorrected{ 0.f }; //remapped to primary listener
		glm::vec3 velocity_listenerCorrected{ 0.f };
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A user facing audio handle. This will be mapped to audio buffer behind the scenes. 
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
	class AudioEmitter : public GameEntity
	{
	private:
		struct AudioSystemKey{}; friend class AudioSystem;
	public:
		AudioEmitter(const AudioSystemKey& AudioSystemRestrictedConstruction) {}
		void activateSound(bool bNewActivation);
		void setSoundAssetPath(const std::string& path);
		void setPosition(const glm::vec3& inPosition);
		void setVelocity(const glm::vec3& velocity);
		void setLooping(bool bLooping);
		void setMaxRadius(float soundRadius);
	private:
		EmitterUserData userData = {};
		EmitterHardwareData hardwareData = {};
		EmitterAudioSystemMetaData systemMetaData = {};
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Listener Data (ie the individual that hears the audio
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct ListenerData
	{
		glm::vec3 position;
		glm::vec3 front_n;
		glm::vec3 up_n;
		//glm::vec3 right_n;
		glm::vec3 velocity_v;
		glm::quat rotation;
		glm::quat inverseRotation; //used to transform sources in splitscreen to player1 position/orientation
	};



	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// AudioSystem - Give access to wrapped game audio
	//
	// The audio system treats sound sources as a limited resource.
	// it prioritizes sounds and assigns a limited number of emitters actual hardware resources (ie a sound source).
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class AudioSystem : public SystemBase
	{
	private:
		size_t api_MaxMonoSources = 16;		//this value is updated by the audio api
		size_t api_MaxStereoSources = 16;	//this value is updated by the audio api
	public:
		sp<AudioEmitter> createEmitter();
		void activateEmitter(const sp<AudioEmitter>& emitter, bool bNewActivation);
		void tickAudioPipeline(float dt_sec);
#if USE_OPENAL_API
		bool hasValidOpenALDevice();
		void updateSourceProperties(AudioEmitter& emitterSource);
#endif //USE_OPENAL_API
	public:
		struct EmitterPrivateKey 
		{
		private:
			friend class AudioEmitter; friend class AudioSystem;
			EmitterPrivateKey() {};
		};
		void trySetEmitterBuffer(AudioEmitter& emitter, const EmitterPrivateKey&);
	private:
		virtual void initSystem() override;
		virtual void shutdown() override;
		void audioTick_beginPipeline();
		void audioTick_updateListenerStates();
		void audioTick_updateEmitterStates(float dt_sec);
		void audioTick_sortActivatedEmitters();
		void audioTick_cullEmitters();
		void audioTick_updateFadeOut();
		void audioTick_updateFadeIn();
		void audioTick_updateEmittersWithHardwareResources();
		void audioTick_emitterGarbageCollection();
		void audioTick_endPipeline();
	private:
		void handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel);
	private:
		float calculatePitch(float rawPitch);
		float calculateGain(float rawGain, bool bIsMusic);
	private:
		//note: lists of raw pointers are cleared and regenerated each frame
		std::vector<sp<AudioEmitter>> list_userActivatedSounds;	//entire list of sounds the game wants playing, likely larger than what hardware can handle
		std::set<sp<AudioEmitter>> list_pendingUserActivation;	//set of sounds that user just flagged to be activated
		std::vector<AudioEmitter*> list_HardwarePermitted;		//sounds that can be played given hardware restrictions (in addition to of FadeIn and FadeOut lists)
		std::vector<AudioEmitter*> list_pendingAssignHardwareSource;
		std::vector<AudioEmitter*> list_pendingRemoveHardwareSource;
		std::vector<sp<AudioEmitter>> allEmitters;					
		std::vector<ListenerData> listenerData;
		AmortizeLoopTool amortizeGarbageCollectionCheck;
		std::vector<size_t> gcIndices;
#if USE_OPENAL_API
		ALCdevice* device = nullptr;
		ALCcontext* context = nullptr; //for now, split screen will share context, and manual source fixup required; perahps multiple contexts
		std::unordered_map</*filePath*/std::string, ALuint> audioBuffers;
		Pool<ALuint> sourcePool;
#endif //USE_OPENAL_API
		float cachedTimeDilation = 1.f;
		float systemWideVolume = 1.f;
		float musicVolume = 1.f;
		bool bDirtyAllPitch = false; //useful for when time dilation changes
	};

}