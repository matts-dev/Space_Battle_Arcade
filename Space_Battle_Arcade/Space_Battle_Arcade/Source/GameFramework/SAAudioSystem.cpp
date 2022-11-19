#include "SAAudioSystem.h"
#include "SAGameBase.h"
#include "SAGameEntity.h"
#include "Audio/OpenALUtilities.h"
#include "SALog.h"
#include "SAAssetSystem.h"
#include "SALevel.h"
#include "SALevel.h"
#include "SATimeManagementSystem.h"
#include "SALevelSystem.h"
#include "Tools/SAUtilities.h"
#include "SAPlayerSystem.h"
#include "../Rendering/Camera/SACameraBase.h"
#include "SAPlayerBase.h"
#include "Tools/PlatformUtils.h"
#include "TimeManagement/TickGroupManager.h"
#include "SADebugRenderSystem.h"
#include "SARandomNumberGenerationSystem.h"

namespace SA
{
#define IGNORE_AUDIO_COMPILE_TODOS 1


//defines whether extra logging should be compiled to debug resource management
#define VERBOSE_AUDIO_RESOURCE_LOGGING 0

#if VERBOSE_AUDIO_RESOURCE_LOGGING
#define CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE(msg, ...)\
logf_sa(__FUNCTION__, LogLevel::LOG, msg, __VA_ARGS__);
#else
#define CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE(msg, ...) 
#endif


//very verbose messages will print every frame in normal scenario
#define VERYVERBOSE_AUDIO_RESOURCE_LOGGING (VERBOSE_AUDIO_RESOURCE_LOGGING & 0)
#if VERYVERBOSE_AUDIO_RESOURCE_LOGGING
#define CONDITIONAL_VERYVERBOSE_RESOURCE_LOG_MESSAGE(msg, ...)\
logf_sa(__FUNCTION__, LogLevel::LOG, msg, __VA_ARGS__);
#else
#define CONDITIONAL_VERYVERBOSE_RESOURCE_LOG_MESSAGE(msg, ...) 
#endif
	





	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// audio emitter 3D
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//void AudioEmitter::activateSound(bool bNewActivation)
	//{
	//	AudioSystem& audioSystem = GameBase::get().getAudioSystem();
	//	audioSystem.activateEmitter(sp_this(), bNewActivation);
	//}

	void AudioEmitter::setSoundAssetPath(const std::string& path)
	{
		userData.sfxAssetPath = path;
		GameBase::get().getAudioSystem().trySetEmitterBuffer(*this, AudioSystem::EmitterPrivateKey{});
	}

	void AudioEmitter::setPosition(const glm::vec3& inPosition)
	{
		this->userData.position = inPosition;
		this->systemMetaData.dirtyFlags.bPosition = true;
	}
	
	void AudioEmitter::setVelocity(const glm::vec3& velocity)
	{
		this->userData.velocity = velocity;
		this->systemMetaData.dirtyFlags.bVelocity = true;
	}

	void AudioEmitter::setPriority(AudioEmitterPriority priority)
	{
		userData.priority = priority;
	}

	void AudioEmitter::setLooping(bool bLooping)
	{
		this->userData.bLooping = bLooping;

		if (this->systemMetaData.bPlaying)
		{
			//#TODO #audio perhaps we should create emitters with an initializer for some user data that should never change during the emitters life time.
			//but doing it this way does allow us to recycle emitters; its just that the user needs to take a lot of care to clean up state.
			logf_sa(__FUNCTION__, LogLevel::LOG_WARNING, "Changing looping user data on a sound that is already playing! this will likely cause bugs");
			STOP_DEBUGGER_HERE();
		}
	}

	void AudioEmitter::setMaxRadius(float soundRadius)
	{
		userData.maxRadius = soundRadius;

		//reference distance is the distance at which the sound will be half as loud
		//this is a bit ad-hoc, but we need to figure out what is the halfing rate so that the sound hits 0gain at the max radius
		/*
			half_n_times(1000, 14)    >>> half_n_times(10000, 17)    >>> half_n_times(100000, 20)   >>> half_n_times(1000000, 23)
			500.0					  5000.0					     50000.0					    500000.0
			250.0					  2500.0					     25000.0					    250000.0
			125.0					  1250.0					     12500.0					    125000.0
			62.5					  625.0						     6250.0						    62500.0
			31.25					  312.5						     3125.0						    31250.0
			15.625					  156.25					     1562.5						    15625.0
			7.8125					  78.125					     781.25						    7812.5
			3.90625					  39.0625					     390.625					    3906.25
			1.953125				  19.53125					     195.3125					    1953.125
			0.9765625				  9.765625					     97.65625					    976.5625
			0.48828125				  4.8828125					     48.828125					    488.28125
			0.244140625				  2.44140625				     24.4140625					    244.140625
			0.1220703125			  1.220703125				     12.20703125				    122.0703125
			0.06103515625			  0.6103515625				     6.103515625				    61.03515625
									  0.30517578125				     3.0517578125				    30.517578125
									  0.152587890625			     1.52587890625				    15.2587890625
									  0.0762939453125			     0.762939453125				    7.62939453125
																	 0.3814697265625			    3.814697265625
																	 0.19073486328125			    1.9073486328125
																	 0.095367431640625			    0.95367431640625
																								    0.476837158203125
																								    0.2384185791015625
																									0.11920928955078125
			
		Using this adhoc mapping we can estiamte a halfing distance to be 1/(14+3*n) where n is powers of 10 in addition to 100
		As I said, very adhoc. I'd prefer to do this analytically rather than numerically, but not going to invest time in that atm.

		based on above, seems need to increase by factor of 3 or 4 for every power of 10
		*/
		int factorsOf10 = int(std::log(soundRadius) / std::log(10.f));

#ifdef DEBUG_BUILD
		if (factorsOf10 < 0 ) //|| Utils::anyValueNAN(factorsOf10)
		{
			STOP_DEBUGGER_HERE();
			systemMetaData.referenceDistance = userData.maxRadius / 2.f; //provide some value, even though it wn't be correct.
		}
#endif

		float n = glm::max(float(factorsOf10) * 3.f, 2.f);
		float distFactor = 1.f / n;

		//TODO Implement this correctly
		systemMetaData.referenceDistance = userData.maxRadius * distFactor;
		systemMetaData.dirtyFlags.bReferenceDistance = true;
	}

	void AudioEmitter::setOneShotPlayTimeout(std::optional<float> timeoutAfterXSeconds)
	{
		userData.tryPlayWindowSeconds = timeoutAfterXSeconds;
	}

	void AudioEmitter::setPitchVariationRange(float pitchVariation)
	{
		userData.pitchVariationRange = pitchVariation;
	}

	void AudioEmitter::setPitch(float newPitch)
	{
		userData.pitch = newPitch;
		systemMetaData.dirtyFlags.bPitch = true;
	}

	void AudioEmitter::setGain(float gain)
	{
		userData.volume = gain;
		systemMetaData.dirtyFlags.bGain = true;
	}

	void AudioEmitter::setVolume(float gain)
	{
		setGain(gain);
	}

	void AudioEmitter::stop()
	{
		AudioSystem& audioSystem = GameBase::get().getAudioSystem();
		audioSystem.activateEmitter(sp_this(), false);
		audioSystem.stopEmitter(*this, AudioSystem::EmitterPrivateKey{});
	}

	void AudioEmitter::play()
	{
		//this will add sound to be activated; if it's priority is high enough and resources are available it will start playing
		GameBase::get().getAudioSystem().activateEmitter(sp_this(), true);

		//for non-looping sounds, the start time can be used to auto-deactivate sounds that never had the priority to play
		systemMetaData.playStartTimeStamp.reset();
		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			const sp<TimeManager>& worldTM = currentLevel->getWorldTimeManager();
			systemMetaData.playStartTimeStamp = worldTM->getTimestampSecs();
		}

#if USE_OPENAL_API
#endif //USE_OPENAL_API
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// audio system
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	sp<SA::AudioEmitter> AudioSystem::createEmitter()
	{
		sp<AudioEmitter> newEmitter = new_sp<AudioEmitter>(AudioEmitter::AudioSystemKey{});

		CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("Created emitter %p", newEmitter.get());
		allEmitters.push_back(newEmitter);

		return newEmitter;
	}


	void AudioSystem::activateEmitter(const sp<AudioEmitter>& emitter, bool bNewActivation)
	{
		if (emitter->systemMetaData.bActive != bNewActivation)
		{
			emitter->systemMetaData.bActive = bNewActivation;
			if (emitter->systemMetaData.bActive)
			{
				emitter->systemMetaData.bActive = true;
				set_pendingUserActivation.insert(emitter); //don't add directly to list because naive user code may call this multiple times
			}
			else
			{
				//if this is in the activation list, it will be removed
				emitter->systemMetaData.bActive = false;
			}

		}
	}

	void AudioSystem::trySetEmitterBuffer(AudioEmitter& emitter, const EmitterPrivateKey&)
	{
#if USE_OPENAL_API
		const std::string& path = emitter.userData.sfxAssetPath;

		ALBufferWrapper bufferData;

		auto findResult = audioBuffers.find(path);
		if (findResult == audioBuffers.end())
		{
			//sync load
			bufferData = GameBase::get().getAssetSystem().loadOpenAlBuffer(path);
			audioBuffers.insert({ path, bufferData });
		}
		else
		{
			bufferData = findResult->second;
		}

		emitter.hardwareData.bufferIdx = bufferData.buffer;
		emitter.systemMetaData.audioDurationSec = bufferData.durationSec;

		if (emitter.hardwareData.sourceIdx.has_value() && emitter.hardwareData.bufferIdx.has_value())
		{
#define DIAGNOSTIC_SOURCE_BUFFER_SETTING 1
#if DIAGNOSTIC_SOURCE_BUFFER_SETTING & VERBOSE_AUDIO_RESOURCE_LOGGING 
			ALuint source = *emitter.hardwareData.sourceIdx;
			ALint sourceState = 0;
			alec(alGetSourcei(source, AL_SOURCE_STATE, &sourceState));
			if (sourceState == AL_PLAYING)
			{
				CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("Switching buffer on a source still playing! source %d emitter %p", source, &emitter);
			}
			else if (sourceState == AL_PAUSED)
			{
				CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("Switching buffer on a paused source! source %d emitter %p", source, &emitter);
			}
#endif //DIAGNOSTIC_SOURCE_BUFFER_SETTING

			// we only need update listener if we have a listener and a valid buffer
			// this will probably cause an abrupt change in sound; to fix that we should fade out and fade in new sound.
			// not sure if changing sound on an emitter will be an active use case though as it will require probably tweaking all emitter properties
			// so for now just setting source's value directly
			alec(alSourcei(*emitter.hardwareData.sourceIdx, AL_BUFFER, *emitter.hardwareData.bufferIdx)); //#audiothreads update to listener should happen in dedicated audio thread processing
		}

#endif //USE_OPENAL_API
	}

	void AudioSystem::stopEmitter(AudioEmitter& emitter, const EmitterPrivateKey&)
	{
#if USE_OPENAL_API
		if (emitter.hardwareData.sourceIdx.has_value() && context && !GameBase::isEngineShutdown())
		{
			ALuint source = *emitter.hardwareData.sourceIdx;
			CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("stoping source %d", source);

			//if user requested stop, then stop immediately; don't let system fade it out
			alec(alSourceStop(source));

			//if we stopped this source, then clear the resource so it will have to be given a new source to play again
			sourcePool.releaseInstance(source);
			CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("releaseing source %d to free pool from emitter %p", source, &emitter);
			emitter.hardwareData.sourceIdx = std::nullopt;
		}
#endif //USE_OPENAL_API
	}

	void AudioSystem::setSystemVolumeMultiplier(float multiplier)
	{
		systemWideVolume = glm::clamp(multiplier, 0.f, getSystemAudioMaxMultiplier());
	}

	void AudioSystem::logDebugInformation()
	{
#if USE_OPENAL_API
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// log the existing hardware source data
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		size_t numSourcesPlaying = 0;
		for (const ALSourceData& srcData : generatedSources)
		{
			AudioEmitter* assignedEmitter = nullptr;
			std::string path = "null emitter or code compiled out";
#if AUDIO_TRACK_SOURCES
			if (!srcData.assignedEmitter.expired())
			{
				assignedEmitter = srcData.assignedEmitter.lock().get();
				if (assignedEmitter)
				{
					path = assignedEmitter->userData.sfxAssetPath;
				}
			}
#endif
			ALint sourceState;
			alec(alGetSourcei(srcData.source, AL_SOURCE_STATE, &sourceState));
			numSourcesPlaying += (sourceState == AL_PLAYING);

			logf_sa(__FUNCTION__, LogLevel::LOG, "HARDWARE SOURCES: source %d is assigned to emitter %p is playing %d with sound %s", srcData.source, assignedEmitter, (sourceState == AL_PLAYING), path.c_str());
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// log the emitters that are in user activated list
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		for (const AudioEmitterHandle& emitter : list_userActivatedSounds)
		{
			AudioEmitter* emitterRaw = emitter ? emitter.get() : nullptr;
			ALuint sourceIdx = 0;
			ALint sourceState = 0;
			std::string path = "NA";
			if (emitter)
			{
				sourceIdx = emitter->hardwareData.sourceIdx.value_or(0);
				if (sourceIdx)
				{
					alec(alGetSourcei(sourceIdx, AL_SOURCE_STATE, &sourceState));
				}
				path = emitter->userData.sfxAssetPath;
			}
			logf_sa(__FUNCTION__, LogLevel::LOG, "USER LIST: emitter %p with source %d is playing %d with sound %s", emitterRaw, sourceIdx, (sourceState == AL_PLAYING), path.c_str());
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Stats
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		logf_sa(__FUNCTION__, LogLevel::LOG, "STATS: UserActiveList[%d] TotalEmitters[%d] TotalSources[%d], TotalPlayingSound[%d]", list_userActivatedSounds.size(), allEmitters.size(), generatedSources.size(), numSourcesPlaying);
#else
		AUDIO_API_NEEDS_DEBUG_IMPLEMENTATION;
#endif 
	}

	void AudioSystem::tickAudioPipeline(float dt_sec)
	{
		//trying out a pipelined approach to writing this function; primary goal is to communicate the highlevel via code not comments
		audioTick_beginPipeline();
		audioTick_updateListenerStates();
		audioTick_updateActiveUserEmitterStates(dt_sec);
		audioTick_sortActivatedEmitters();				
		audioTick_cullEmitters();							
		audioTick_releaseHardwareResources();						
		audioTick_assignHardwareResources();						
		audioTick_updateEmittersWithHardwareResources();
		audioTick_emitterGarbageCollection();
		audioTick_endPipeline();
	}

	bool AudioSystem::hasValidOpenALDevice()
	{
		return device != nullptr;
	}

	float AudioSystem::calculatePitch(float rawPitch)
	{
		return rawPitch * cachedTimeDilation;
	}

	float AudioSystem::calculateGain(float rawGain, bool bIsMusic)
	{
		return rawGain * systemWideVolume * (bIsMusic ? musicVolume : 1.f);
	}

#if USE_OPENAL_API
	void AudioSystem::updateSourceProperties(AudioEmitter& emitterSource)
	{
		if (emitterSource.hardwareData.sourceIdx.has_value())
		{
			//alias data so it is more readable
			ALuint src = *emitterSource.hardwareData.sourceIdx;
			EmitterUserData& ud = emitterSource.userData;
			EmitterAudioSystemMetaData& md = emitterSource.systemMetaData;

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//remap position and velocity if they are not close to player0 listener
			//basically, use sources relative positioning/velocity to player2 and set that up for player0 in regards to API source.
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (md.closestListenerIdx != 0 && Utils::isValidIndex(listenerData, md.closestListenerIdx))
			{
				ListenerData& closestListener = listenerData[md.closestListenerIdx];

				md.position_listenerCorrected = (ud.position * closestListener.inverseRotation) * listenerData[0].rotation;
				md.velocity_listenerCorrected = (ud.velocity * closestListener.inverseRotation) * listenerData[0].rotation;

				md.bPreviousFrameListenerMapped = true;

				//always dirty position/velocity if we're doing a remapping
				md.dirtyFlags.bPosition = md.dirtyFlags.bVelocity = true;
			}
			else
			{
				//make dirty if we swapped from being mapped to no being mapped, also if we're already dirty then we need to refresh
				md.dirtyFlags.bPosition |= md.bPreviousFrameListenerMapped;
				md.dirtyFlags.bVelocity |= md.bPreviousFrameListenerMapped;

				//pass through the positioning data
				md.position_listenerCorrected = ud.position;
				md.velocity_listenerCorrected = ud.velocity;
				md.bPreviousFrameListenerMapped = false; //must be after dirty check!
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Update Audio API
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (md.dirtyFlags.bPosition)
			{
				glm::vec3& pos = md.position_listenerCorrected;
				alec(alSource3f(src, AL_POSITION, pos.x, pos.y, pos.z)); //#TODO #scenenodes
				md.dirtyFlags.bPosition = false;
			}
			if (md.dirtyFlags.bVelocity)
			{ 
				glm::vec3& vel = md.velocity_listenerCorrected;
				alec(alSource3f(src, AL_VELOCITY, vel.x, vel.y, vel.z));
				md.dirtyFlags.bVelocity = false;
			}
			if (md.dirtyFlags.bPitch || bDirtyAllPitch) 
			{ 
				float pitch = calculatePitch(ud.pitch + md.calculatedPitchVariation);
				alec(alSourcef(src, AL_PITCH, pitch));
				md.dirtyFlags.bPosition = false;
			}
			if (md.dirtyFlags.bGain) 
			{ 
				float gain = glm::clamp(calculateGain(ud.volume, ud.bIsMusic) /** md.soundFadeModifier*/, 0.f, 1.f);
#if !IGNORE_AUDIO_COMPILE_TODOS
				todo_renable_fade;
#endif

				alec(alSourcef(src, AL_GAIN, gain));
				md.dirtyFlags.bGain = false; 
			}
			if (md.dirtyFlags.bLooping) 
			{ 
				alec(alSourcei(src, AL_LOOPING, ALint(ud.bLooping)));
				md.dirtyFlags.bLooping = false; 
			}
			if (md.dirtyFlags.bReferenceDistance)
			{
				alec(alSourcei(src, AL_REFERENCE_DISTANCE, ALuint(md.referenceDistance)));
				md.dirtyFlags.bReferenceDistance = false;
			}

#if COMPILE_AUDIO_DEBUG_RENDERING_CODE
			if (bRenderSoundLocations)
			{
				static DebugRenderSystem& debugRenderSystem = GameBase::get().getDebugRenderSystem();
				//debugRenderSystem.renderSphere(ud.position, glm::vec3(0.1f), glm::vec3(0.5, 0.5, 1.f));
				debugRenderSystem.renderSphere(md.position_listenerCorrected, glm::vec3(0.1f), glm::vec3(0.5, 0.5, 1.f));
			}
#endif //COMPILE_AUDIO_DEBUG_RENDERING_CODE
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "updating AL source but no source index available!");
			log(__FUNCTION__, LogLevel::LOG_ERROR, emitterSource.userData.sfxAssetPath.size() > 0 ? emitterSource.userData.sfxAssetPath.c_str() : "no asset path");
		}
	}

	void AudioSystem::teardownALSource(ALuint source)
	{
		ALSourceData sourceData;
		sourceData.source = source;
		generatedSources.erase(sourceData);

		alec(alSourceStop(source));
		alec(alSourcei(source, AL_BUFFER, 0));
		alec(alDeleteSources(1, &source));
	}

#endif

	void AudioSystem::initSystem()
	{
		//have audio system register its tick to be after the camera
		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		levelSystem.onPreLevelChange.addWeakObj(sp_this(), &AudioSystem::handlePreLevelChange);
		if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
		{
			handlePreLevelChange(nullptr, currentLevel);
		}

		pitchVariabilityRNG = GameBase::get().getRNGSystem().getTimeInfluencedRNG();

		//note: if user is consistently making more than 10 emitters a frame, removal of stale emitters will fall behind creation.
		amortizeGarbageCollectionCheck.chunkSize = 10; 

#if !IGNORE_AUDIO_COMPILE_TODOS
		TODO_need_to_change_emitter_to_be_handle_like_situation_so_we_can_tell_if_theres_no_users_and_we_can_gc;
#endif


#if COMPILE_AUDIO
#if USE_OPENAL_API
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// find the default audio device
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		const ALCchar* defaultDeviceString = alcGetString(/*device*/nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
		device = alcOpenDevice(defaultDeviceString);
		if (!device)
		{
			log("AudioSystem - OpenAL", LogLevel::LOG_ERROR, "FATAL: failed to get the default device for OpenAL");
			return; //consider fatal
		}

		std::string deviceLogString = std::string("OpenAL Device: ") + alcGetString(device, ALC_DEVICE_SPECIFIER);
		log("AudioSystem - OpenAL", LogLevel::LOG, deviceLogString.c_str());

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Create an OpenAL audio context from the device
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		context = alcCreateContext(device, /*attrlist*/ nullptr);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Activate this context so that OpenAL state modifications are applied to the context
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (!context || !alcMakeContextCurrent(context))
		{
			log("AudioSystem - OpenAL", LogLevel::LOG_ERROR, "FATAL: failed to make the OpenAL context the current context");
			return; //consider fatal
		}
		OpenAL_ErrorCheck("Make context current"); //NOTE: we shouldn't error check until we have a nonnull context

		//query device data
		ALCint numAttributes;
		alec(alcGetIntegerv(device, ALC_ATTRIBUTES_SIZE, 1, &numAttributes));

		std::vector<ALCint> contextAttributes(numAttributes);
		alec(alcGetIntegerv(device, ALC_ALL_ATTRIBUTES, numAttributes, &contextAttributes[0]));

		//attribute appear to be packed in pairs, hence this loop jumps 2 spaces at a time
		for (size_t attributeIdx = 0; attributeIdx < contextAttributes.size(); attributeIdx += 2)
		{
			ALCint attribute = contextAttributes[attributeIdx];
			size_t valueIdx = attributeIdx + 1;

			if (attribute == ALC_MONO_SOURCES)
			{
				if (Utils::isValidIndex(contextAttributes, valueIdx))
				{
					api_MaxMonoSources = size_t(contextAttributes[valueIdx]);
					logf_sa(__FUNCTION__, LogLevel::LOG, "OpenAL max audio mono sources: %d", api_MaxMonoSources);
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "cannot read max mono sources attribute value as value index is invalid"); STOP_DEBUGGER_HERE();
				}
			}
			else if (attribute == ALC_STEREO_SOURCES)
			{
				if (Utils::isValidIndex(contextAttributes, valueIdx))
				{
					api_MaxStereoSources = size_t(contextAttributes[valueIdx]);
					logf_sa(__FUNCTION__, LogLevel::LOG, "OpenAL max audio stereo sources: %d", api_MaxStereoSources);
				}
				else
				{
					log(__FUNCTION__, LogLevel::LOG_ERROR, "cannot read max stereo sources attribute value as value index is invalid"); STOP_DEBUGGER_HERE();
				}
			}
		}
#endif //USE_OPENAL_API
#endif //ENABLE_AUDIO

		//!!! must come after we've queried how many sound sources we can use !!!
		sourcePool.reserve(api_MaxMonoSources);
		listenerData.reserve(MAX_LOCAL_PLAYERS);
		list_hardwarePermitted.reserve(api_MaxMonoSources);
		list_pendingAssignHardwareSource.reserve(api_MaxMonoSources);
		list_pendingRemoveHardwareSource.reserve(api_MaxMonoSources);
		gcIndices.reserve(amortizeGarbageCollectionCheck.chunkSize);
	}

	void AudioSystem::shutdown()
	{
#if COMPILE_AUDIO
#if USE_OPENAL_API

		logf_sa(__FUNCTION__, LogLevel::LOG, "begin cleaning up openal sources");
		for (sp<AudioEmitter>& emitter : allEmitters)
		{
			if (emitter && emitter->hardwareData.sourceIdx.has_value())
			{
				teardownALSource(*emitter->hardwareData.sourceIdx);
			}
		}

		//drain the pool of sources that can be claimed
		while (std::optional<ALuint> optionalSource= sourcePool.getInstance())
		{
			teardownALSource(*optionalSource);
		}
		logf_sa(__FUNCTION__, LogLevel::LOG, "end cleaning up openal sources");
		logf_sa(__FUNCTION__, LogLevel::LOG, "begin cleanup al buffers");

		GameBase::get().getAssetSystem().unloadAllOpenALBuffers();

		logf_sa(__FUNCTION__, LogLevel::LOG, "end cleanup al buffers");

		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		alcCloseDevice(device);

		context = nullptr;
		device = nullptr;
#endif //USE_OPENAL_API
#endif //ENABLE_AUDIO
	}

	void AudioSystem::audioTick_beginPipeline()
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//update time dilation
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			if (const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager())
			{
				float timeDilationFactor = worldTimeManager->getTimeDilationFactor();
				if (timeDilationFactor != cachedTimeDilation)
				{
					cachedTimeDilation = timeDilationFactor;
					bDirtyAllPitch = true;
				}

				currentTimeStampSec = worldTimeManager->getTimestampSecs();
			}
		}
	}

	void AudioSystem::audioTick_updateListenerStates()
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//update player positioning
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		const std::vector<sp<PlayerBase>>& allPlayers = GameBase::get().getPlayerSystem().getAllPlayers();
		listenerData.clear();

		for (const sp<PlayerBase>& player : allPlayers)
		{
			if (sp<CameraBase> camera = player->getCamera())
			{
				ListenerData listener;
				listener.position = camera->getPosition();
				//listener.right_n = camera->getRight();
				listener.front_n = camera->getFront();
				listener.up_n = camera->getUp();
				listener.velocity_v = camera->getVelocity();
				listener.rotation = camera->getQuat();
				listener.inverseRotation = glm::inverse(listener.rotation);
				listenerData.push_back(listener);
			}
		}

		if (listenerData.size() > 0)
		{
#if USE_OPENAL_API
			const ListenerData& listener = listenerData[0];

			alec(alListener3f(AL_POSITION, listener.position.x, listener.position.y, listener.position.z));
			alec(alListener3f(AL_VELOCITY, listener.velocity_v.x, listener.velocity_v.y, listener.velocity_v.z));
			ALfloat forwardAndUpVectors[] = {
				/*forward = */ listener.front_n.x, listener.front_n.y, listener.front_n.z,
				/* up = */ listener.up_n.x, listener.up_n.y, listener.up_n.z
			};
			alec(alListenerfv(AL_ORIENTATION, forwardAndUpVectors));
#endif //USE_OPENAL_API
		}
	}

	void AudioSystem::audioTick_updateActiveUserEmitterStates(float dt_sec)
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//consume the pending activations before prioritization happens
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		for (const sp<AudioEmitter>& sound : set_pendingUserActivation)
		{
			//user code may have deactivated emitter between its addition to this set and this tick so use a set.
			//additionally, the removal from active list may not happen before a recycled emitter is readded; so monitor it is in the active list
			if (sound->systemMetaData.bActive && !sound->systemMetaData.bInActiveUserList)
			{
				addToUserActiveList(sound);
			}
		}
		set_pendingUserActivation.clear();

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//walk the list backwards so we can do swap-and-pop on deactivated sounds
		//starting from the back means that we can count on the last elements already having their state recalculated
		//unsigned wrap around
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		constexpr size_t limit = std::numeric_limits<size_t>::max();
		for (size_t idx = list_userActivatedSounds.size() - 1; idx < list_userActivatedSounds.size() && idx != limit; --idx)
		{
			const AudioEmitterHandle& emitter = list_userActivatedSounds[idx];
			if (emitter)
			{
				if (emitter->systemMetaData.bActive)
				{
					EmitterAudioSystemMetaData& soundMetaData = emitter->systemMetaData;
					const EmitterUserData& soundUserData = emitter->userData;

					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// tick fade changes from previous frame prioritization
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					soundMetaData.soundFadeModifier -= dt_sec * soundMetaData.fadeDirection * soundMetaData.fadeRateSecs;
					soundMetaData.soundFadeModifier = glm::clamp(soundMetaData.soundFadeModifier, 0.f, 1.f);

					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// stop oneshot sounds when they're over
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if USE_OPENAL_API
					if (emitter->hardwareData.sourceIdx.has_value())
					{
						ALuint source = *emitter->hardwareData.sourceIdx;
						ALint sourceState = 0;
						alec(alGetSourcei(source, AL_SOURCE_STATE, &sourceState));

						//this is assuming that when something is given a hardware source, then it will be played immediately
						if (sourceState != AL_PLAYING)
						{
							CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("detected emitter is no longer playing, flagging for deactivation %p %s", emitter.get(), emitter->userData.sfxAssetPath.c_str());
							emitter->systemMetaData.bActive = false;
						}
					}
					else  //if it doesn't have hardware resource yet, has it been too long to give it one?
					{
						if (emitter->isOneShotSample() || emitter->userData.tryPlayWindowSeconds.has_value())
						{
							bool bHasOneshotPlayLimit = emitter->userData.tryPlayWindowSeconds || emitter->systemMetaData.audioDurationSec.has_value();
							if (emitter->systemMetaData.playStartTimeStamp.has_value() && bHasOneshotPlayLimit)
							{
								float startTime = *emitter->systemMetaData.playStartTimeStamp;
								float giveupTimeSec = emitter->userData.tryPlayWindowSeconds.has_value() 
									? (*emitter->userData.tryPlayWindowSeconds) 
									: (*emitter->systemMetaData.audioDurationSec);
								if (currentTimeStampSec > (startTime + giveupTimeSec))
								{
									//this will force the sound to be culled in the cull pass.
									emitter->systemMetaData.bActive = false;
									CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("deactivating emitter than never got resources %p %s", emitter.get(), emitter->userData.sfxAssetPath.c_str());
								}
							}
						}
					}
#else
					TODO_audio_api_needs_implementation_otherwise_oneshot_sounds_wont_autocull;
#endif

					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// Prioritize this sound so that it can be culled if resources are required for higher priority sounds
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////

					//calculate new priority; start a 0 and add to priority
					soundMetaData.calculatedPriority = 0;

					ListenerData* closestListener = nullptr;
					float closestDist2 = std::numeric_limits<float>::infinity();
					size_t closestListenerIdx = 0;
					for (size_t listernIdx = 0; listernIdx < listenerData.size(); ++listernIdx)
					{
						ListenerData& listenerDatum = listenerData[listernIdx];
						float length2 = glm::length2(listenerDatum.position - soundUserData.position);
						if (length2 < closestDist2)
						{
							closestListener = &listenerDatum;
							closestDist2 = length2;
							closestListenerIdx = listernIdx;
						}
					}

					//0.X000 <-takes this position on priority assignment. 
					float proximityValue = 0.f;
					bool bOutOfRange = true;

					//did we find any close listener? (basically always true unless we don't have a player)
					if (closestListener)
					{
						const float radiusSquared = Utils::square(soundUserData.maxRadius);
						soundMetaData.closestListenerIdx = closestListenerIdx;
						if (closestDist2 < radiusSquared)
						{
							bOutOfRange = false;
							float distance = glm::length(soundUserData.position - closestListener->position);
							float distanceAlpha = 1.0f - glm::clamp(distance / soundUserData.maxRadius, 0.f, 1.f); 

							//may need to rethink this a bit, perhaps multiplying things by 10,100,1000 based on priority level.
							soundMetaData.calculatedPriority += distanceAlpha; //[0,1] range already is in the tenths position
							soundMetaData.bOutOfRange = false;
						}
						else
						{
							//change the distance max distance when trying to drop sounds so that sounds on border of radius do not flicker in/out
							soundMetaData.bOutOfRange = closestDist2 * 1.01 < radiusSquared;
							soundMetaData.calculatedPriority += 1.f; //max distance alpha
						}
					}
					
					soundMetaData.calculatedPriority += float(soundUserData.priority);
				}
				else
				{
					removeFromActiveList(idx); //this is a backwards walk, no need to correct idx
				}
			}
			else
			{
				//emitter may be nullptr if it has been removed from GC
				removeFromActiveList(idx); //this is a backwards walk, no need to correct idx
			}
		}

	}

	void AudioSystem::audioTick_sortActivatedEmitters()
	{
		std::sort(
			list_userActivatedSounds.begin(),
			list_userActivatedSounds.end(),
			[](const AudioEmitterHandle& first, const AudioEmitterHandle& second)
			{
				//shouldn't hit nullptrs, but adding to reduce code fragility
				if(!second)
				{
					return true; //put second nullptr after
				}
				if (!first) 
				{
					return false;//put second before first (second is not nullptr)
				}

				//see if first is less than last.
				return first->systemMetaData.calculatedPriority < second->systemMetaData.calculatedPriority;
			}
		);
	}

	void AudioSystem::audioTick_cullEmitters()
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Changes to state should be done before this step in pipeline, emitters should be just moved to appropriate lists
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		size_t availableSources = api_MaxMonoSources;

		list_hardwarePermitted.clear();
		list_pendingRemoveHardwareSource.clear();
		list_pendingAssignHardwareSource.clear(); 

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Walk over user list. 
		// NOTE: this is not applied to all emitters, it is just the user activated list.
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		for (size_t idx = 0; idx < list_userActivatedSounds.size(); ++idx)
		{
			const AudioEmitterHandle& sound = list_userActivatedSounds[idx];

			if (sound)
			{
				//always reset fade state to fade up; we will fade out if needed. But this restores things that may haev filpped from a fade out to a fadein
				sound->systemMetaData.fadeDirection = 1.f;

				//use ordering of priority to determine if a sound should be given/keep hardware resources
				bool bSoundShouldPlay = //ie don't cull
					idx < availableSources 
					&& !sound->systemMetaData.bOutOfRange 
					&& sound->systemMetaData.bActive;
				if (bSoundShouldPlay)
				{
					// if this doesn't have a hardware source, then give it one
					if (!sound->hardwareData.sourceIdx.has_value())
					{
						list_pendingAssignHardwareSource.push_back(sound.get());
						CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("sound requesting hardware resource %p", sound.get());
					}
					else
					{
						//since this has hardware resources, add it to the list that will have their sources updated
						list_hardwarePermitted.push_back(sound.get());
						CONDITIONAL_VERYVERBOSE_RESOURCE_LOG_MESSAGE("sound with resource adding to hardware list %p", sound.get());
					}
				}
				else // cull this sound
				{
					//if this sound has a hardware source, then we need to take it away and give it to a higher priority sound
					if (sound->hardwareData.sourceIdx.has_value())
					{
						list_pendingRemoveHardwareSource.push_back(sound.get());
						CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("culling sound with hardware resource %p", sound.get());
					}
				}
			}
		}
	}

	void AudioSystem::audioTick_releaseHardwareResources()
{
		//these will have their fade percentage drained over time
		//NOTE: currently deactivating a sound will cause it to be evicted immediately and bypass fade logic
		for (AudioEmitter* emitter : list_pendingRemoveHardwareSource)
		{
			if (emitter && emitter->systemMetaData.soundFadeModifier <= 0.f)
			{
#if USE_OPENAL_API
				if (!emitter->hardwareData.sourceIdx.has_value())
				{
					STOP_DEBUGGER_HERE(); //why is there a emitter fading out that doesn't have a source?
				}
				else
#endif //USE_OPENAL_API
				{
					removeHardwareResources(*emitter);
					CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("fade out complete %p", emitter);
				}
			}
		}
	}

	void AudioSystem::audioTick_assignHardwareResources()
	{
		size_t freeSources = api_MaxMonoSources - generatedSources.size() + sourcePool.size();
		if (bool bThereAreAvailableHardwareSources = freeSources > 0)
		{
			for (AudioEmitter* emitter : list_pendingAssignHardwareSource)
			{
				freeSources = (api_MaxMonoSources - generatedSources.size()) + sourcePool.size(); //update as this number changes with loop
				if (freeSources > 0)
				{
#if USE_OPENAL_API
					std::optional<ALuint> opt_source = sourcePool.getInstance();
					if (!opt_source.has_value())
					{
						ALuint newSource = 0;
						alec(alGenSources(1, &newSource));
						if (newSource)
						{
							opt_source = newSource;

							ALSourceData sourceData;
							sourceData.source = newSource;
							generatedSources.insert(sourceData);
						}
						CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("fade in generated new source %d for emitter %p", newSource, emitter);
					}

					if (opt_source.has_value() && emitter)
					{
						emitter->hardwareData.sourceIdx = opt_source;
						ALuint source = *opt_source;
						CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("fade in assigned hardware source %d to %p", source, emitter);

						//for looping sounds, start them quiet and fade in
						emitter->systemMetaData.soundFadeModifier = emitter->userData.bLooping ? 0.f : 1.f;

						if (emitter->userData.pitchVariationRange != 0.f)
						{
							float halfRange = emitter->userData.pitchVariationRange / 2.f;
							emitter->systemMetaData.calculatedPitchVariation = pitchVariabilityRNG->getFloat(-halfRange, halfRange);
							emitter->systemMetaData.dirtyFlags.bPitch = true;
						}

#if AUDIO_TRACK_SOURCES
						ALSourceData sourceData;
						sourceData.source = source;
						sourceData.assignedEmitter = emitter->requestTypedReference_Nonsafe<std::remove_pointer_t<decltype(emitter)>>();
						generatedSources.erase(sourceData); //it will not update insertion with new data if it is already source is already there
						generatedSources.insert(sourceData);
#endif

						trySetEmitterBuffer(*emitter, EmitterPrivateKey{});

						//set all flags to dirty by writing 0xff to every byte
						std::memset(reinterpret_cast<uint8_t*>(&emitter->systemMetaData.dirtyFlags), 0xFF, sizeof(decltype(emitter->systemMetaData.dirtyFlags)));

						alec(alSourcePlay(source));

						//this now has a hardware resource, add it to list of sources we will update at the end of the pipeline
						list_hardwarePermitted.push_back(emitter);

					}
#else
					Needs_audio_api_implementation;
#endif
				}
				else
				{
					CONDITIONAL_VERYVERBOSE_RESOURCE_LOG_MESSAGE("fade in did not have enough sources available for all hardware requests. requesting %d sources", list_pendingAssignHardwareSource.size());
					break; //we used up all the sources that were available
				}
			}
		}
	}

	void AudioSystem::audioTick_updateEmittersWithHardwareResources()
	{
		//walk over the hardware permitted sounds list and update the sound buffers (platform and api specific)
		for (AudioEmitter* emitterSource : list_hardwarePermitted)
		{
			if (emitterSource)
			{
#if USE_OPENAL_API
				updateSourceProperties(*emitterSource);
			}
#else
				TODO_other_audio_api_implementation;
#endif
		}
	}

	void AudioSystem::audioTick_emitterGarbageCollection()
	{
		//amortized walk over allEmitters to find use_count of 1, this means no one has a reference to the sound anymore and we should clean it up and delete it.
		//O(4) = O(1)

		gcIndices.clear();

		for (size_t idx = amortizeGarbageCollectionCheck.updateStart(allEmitters); 
			idx < amortizeGarbageCollectionCheck.getStopIdxSafe(allEmitters);
			++idx)
		{
			sp<AudioEmitter>& emitter = allEmitters[idx];
			if (!emitter)
			{
				logf_sa(__FUNCTION__, LogLevel::LOG_ERROR, "AllEmitters list contains a nullptr!"); STOP_DEBUGGER_HERE();
				gcIndices.push_back(idx);
			}
			else //emitter is valid
			{
				//do a systemic check to make sure system hasn't broken; we shouldn't have emitters not in user list that also have hardware resources!
				if (emitter->hardwareData.sourceIdx.has_value() && !emitter->systemMetaData.bInActiveUserList)
				{ 
					logf_sa(__FUNCTION__, LogLevel::LOG_ERROR, "audio system found sound with hardware resources that is not in user playing list!");  STOP_DEBUGGER_HERE();
				}

				if (allEmitters[idx].use_count() <= 1)
				{
					gcIndices.push_back(idx);

					if (emitter->hardwareData.sourceIdx.has_value())
					{
						ALuint source = *emitter->hardwareData.sourceIdx;
						teardownALSource(source);
					}
					emitter->hardwareData.sourceIdx.reset();
					emitter->hardwareData.bufferIdx.reset();
				}
			}
		}

		//must process removal indices in reverse order so that we don't invalidate other indices
		std::sort(gcIndices.begin(), gcIndices.end(), std::greater<>());
		for (size_t idx : gcIndices)
		{
			const sp<AudioEmitter>& emitter = allEmitters[idx];
			if ((emitter && emitter.use_count() == 1) || !emitter)
			{
				CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("Removing emitter with no users %p", emitter.get());
				Utils::swapAndPopback(allEmitters, idx);

				//emitter reference is now garbage
			}
		}

	}

	void AudioSystem::audioTick_endPipeline()
	{
		//if we were dirtying all pitch (because time dilation changed) then do not do that on next frame
		bDirtyAllPitch = false;
	}

	void AudioSystem::addToUserActiveList(const sp<AudioEmitter>& emitter)
	{
		if (!emitter->systemMetaData.bInActiveUserList)
		{
			emitter->systemMetaData.bInActiveUserList = true;
			list_userActivatedSounds.push_back(emitter);
		}
		else
		{
			CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("Emitter already in active list being added to active list!");
			STOP_DEBUGGER_HERE();
		}
	}

	void AudioSystem::removeFromActiveList(size_t idx)
	{
		//this method forces us to track which emitters are in the list so we can use a raw vector for holding emitters

		if (Utils::isValidIndex(list_userActivatedSounds, idx))
		{
			AudioEmitterHandle emitter = list_userActivatedSounds[idx];

			//since emitter list is a list of handles, it is possible they could be null; though it is likely a broken state
			if (emitter)
			{
				emitter->systemMetaData.bInActiveUserList = false;

				if (emitter->hardwareData.sourceIdx.has_value())
				{
					removeHardwareResources(*emitter);
				}
			}
			
			Utils::swapAndPopback(list_userActivatedSounds, idx);
		}
		else
		{
			CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("invalid remove idx!");
			STOP_DEBUGGER_HERE();
		}
	}

	void AudioSystem::removeHardwareResources(AudioEmitter& emitter)
	{
#if USE_OPENAL_API
		if (emitter.hardwareData.sourceIdx.has_value())
		{
			ALuint source = *emitter.hardwareData.sourceIdx;
			emitter.hardwareData.sourceIdx.reset();

			CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("releaseing source %d to free pool from emitter %p", source, &emitter);
			sourcePool.releaseInstance(source);

			//make sure the source is no longer player, when this is pulled from the pool it will be played if necessary
			alec(alSourceStop(source));

#if AUDIO_TRACK_SOURCES
			ALSourceData sourceData;
			sourceData.source = source;
			generatedSources.erase(sourceData); //clear previous entry (just inserting doesn't erase previous data)
			generatedSources.insert(sourceData); //insert new empty entry
#endif

			CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("fade out complete %p", &emitter);
		}
		else { STOP_DEBUGGER_HERE(); } //why is there a emitter fading out that doesn't have a source?
#endif //#if USE_OPENAL_API
	}

	void AudioSystem::handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel)
	{
		if (currentLevel)
		{
			currentLevel->getWorldTimeManager()->getEvent(TickGroups::get().AUDIO).removeWeak(sp_this(), &AudioSystem::tickAudioPipeline);
		}

		for(const sp<AudioEmitter>& emitter : allEmitters)
		{
			//stop all emitters for level transition
			emitter->stop();
		}

		if (newLevel)
		{
			newLevel->getWorldTimeManager()->getEvent(TickGroups::get().AUDIO).addWeakObj(sp_this(), &AudioSystem::tickAudioPipeline);
		}
	}

}

