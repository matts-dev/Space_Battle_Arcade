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
#include "Audio/ALBufferWrapper.h"

#define COMPILE_AUDIO 1
#define COMPILE_AUDIO_DEBUG_RENDERING_CODE 1

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
		AudioEmitterPriority priority = AudioEmitterPriority::GAMEPLAY_AMBIENT;
		glm::vec3 position; 
		glm::vec3 velocity = glm::vec3(0.f);
		float pitch = 1.f;
		float volume = 1.f; 
		float maxRadius = 10.f;
		std::optional<float> tryPlayWindowSeconds = std::nullopt; //if audio can't play because of hardware contention, this is amount of time before it gives up because sound wouldn't make sense to play
		bool bLooping = false;
		bool bIsMusic = false;
	};

	/** These are really API resources, but can be limited by hardware. So code refers to them as hardware resources
	    as this makes it more intuitive to reason about what code is doing (managing a limited resource) */
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
		std::optional<float> playStartTimeStamp = std::nullopt;
		std::optional<float> audioDurationSec = std::nullopt; //optional because have to load to find this information
		bool bActive = false;
		bool bInActiveUserList = false;
		bool bPreviousFrameListenerMapped = false;
		bool bOutOfRange = false;
		bool bPlaying = false;
		struct DirtyFlags
		{
			bool bPosition = true;
			bool bVelocity = true;
			bool bPitch = true;
			bool bGain = true;
			bool bLooping = true;
			bool bReferenceDistance = true;
		}dirtyFlags;
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
		void stop();
		void play();
		//void activateSound(bool bNewActivation); //removing because adding play()/stop() api as we need start_times for playing
		void setSoundAssetPath(const std::string& path);
		void setPosition(const glm::vec3& inPosition);
		void setVelocity(const glm::vec3& velocity);
		void setPriority(AudioEmitterPriority priority);
		void setLooping(bool bLooping);
		void setMaxRadius(float soundRadius);
		void setOneShotPlayTimeout(std::optional<float> timeoutAfterXSeconds); //if audio can't play because of hardware contention, this is amount of time before it gives up because sound wouldn't make sense to play
		bool isOneShotSample() const { return !userData.bLooping; }
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

	
	/** A handle to reference an emitter internal to the audio system. This allows the audio system to move emitters
	   around without increasing the reference count to the emitter. This lets the audio system determine that an
	   emitter has only 1 reference count (the AllEmitters array) and do controlled cleanup of the emitter. This design
	   could be avoided by just holding weak references to emitters. The main advantage is that we always know emitters
	   will be cleaned up at the end of the audio pipeline and that we always have a reference to all emitters in the game,
	   so we can force release hardware resources if needed. */
	struct AudioEmitterHandle
	{
		AudioEmitterHandle(const sp<AudioEmitter>& emitter) 
			: emitter_wp(emitter), emitter_Raw(emitter.get())
		{}
	private:
		AudioEmitter* emitter_Raw;
		wp<AudioEmitter> emitter_wp;
	public:
		operator bool() const { return !emitter_wp.expired(); }
		inline AudioEmitter* operator->() const noexcept { return emitter_Raw; }
		inline AudioEmitter& operator*() const noexcept { return *emitter_Raw; }
		AudioEmitter* get() const { return emitter_Raw; }
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
		void teardownALSource(ALuint source);
#endif //USE_OPENAL_API
	public:
		struct EmitterPrivateKey 
		{
		private:
			friend class AudioEmitter; friend class AudioSystem;
			EmitterPrivateKey() {};
		};
		void trySetEmitterBuffer(AudioEmitter& emitter, const EmitterPrivateKey&);
		void stopEmitter(AudioEmitter& emitter, const EmitterPrivateKey&);
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
		void addToUserActiveList(const sp<AudioEmitter>& emitter);
		void removeFromActiveList(size_t idx);
	private:
		void handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel);
	private:
		float calculatePitch(float rawPitch);
		float calculateGain(float rawGain, bool bIsMusic);
	public:
#if COMPILE_AUDIO_DEBUG_RENDERING_CODE
		bool bRenderSoundLocations = false;
#endif //COMPILE_AUDIO_DEBUG_RENDERING_CODE
	private:
		//note: lists of raw pointers are cleared and regenerated each frame
		//note: do not create additional sp handles to emitters, otherwise cleanup will not detect that only reference is the allEmitters container. Follow precedent of other lists (eg handle or raw pointer if it set just within the pipeline)
		std::vector<AudioEmitterHandle> list_userActivatedSounds;	//entire list of sounds the game wants playing, likely larger than what hardware can handle
		std::set<sp<AudioEmitter>> set_pendingUserActivation;	//set of sounds that user just flagged to be activated
		std::vector<AudioEmitter*> list_hardwarePermitted;		//sounds that can be played given hardware restrictions (in addition to of FadeIn and FadeOut lists)
		std::vector<AudioEmitter*> list_pendingAssignHardwareSource;
		std::vector<AudioEmitter*> list_pendingRemoveHardwareSource;
		std::vector<sp<AudioEmitter>> allEmitters;					
		std::vector<ListenerData> listenerData;
		AmortizeLoopTool amortizeGarbageCollectionCheck;
		std::vector<size_t> gcIndices;
#if USE_OPENAL_API
		ALCdevice* device = nullptr;
		ALCcontext* context = nullptr; //for now, split screen will share context, and manual source fixup required; perahps multiple contexts
		std::unordered_map</*filePath*/std::string, ALBufferWrapper> audioBuffers; 
		std::set<ALuint> generatedSources;
		PrimitivePool<ALuint> sourcePool;
#endif //USE_OPENAL_API
		float cachedTimeDilation = 1.f;
		float systemWideVolume = 1.f;
		float musicVolume = 1.f;
		bool bDirtyAllPitch = false; //useful for when time dilation changes
		float currentTimeStampSec = 0.f;
	};

}