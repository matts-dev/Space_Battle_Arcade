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
#include "../Tools/SAUtilities.h"
#include "SAPlayerSystem.h"
#include "../Rendering/Camera/SACameraBase.h"
#include "SAPlayerBase.h"
#include "../Tools/PlatformUtils.h"
#include "TimeManagement/TickGroupManager.h"

namespace SA
{
#define IGNORE_AUDIO_COMPILE_TODOS 1


//defines whether extra logging should be compiled to debug resource management
#define VERBOSE_AUDIO_RESOURCE_LOGGING 1

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
	void AudioEmitter::activateSound(bool bNewActivation)
	{
		AudioSystem& audioSystem = GameBase::get().getAudioSystem();
		audioSystem.activateEmitter(sp_this(), bNewActivation);
	}

	void AudioEmitter::setSoundAssetPath(const std::string& path)
	{
		userData.sfxAssetPath = path;
		GameBase::get().getAudioSystem().trySetEmitterBuffer(*this, AudioSystem::EmitterPrivateKey{});
	}

	void AudioEmitter::setPosition(const glm::vec3& inPosition)
	{
		this->userData.position = inPosition;
		this->systemMetaData.dirty_bPosition = true;
	}
	
	void AudioEmitter::setVelocity(const glm::vec3& velocity)
	{
		this->userData.velocity = velocity;
		this->systemMetaData.dirty_bVelocity = true;
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
		systemMetaData.dirty_bReferenceDistance = true;
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
				list_pendingUserActivation.insert(emitter); //don't add directly to list because naive user code may call this multiple times
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
		ALuint bufferIdx = 0;

		auto findResult = audioBuffers.find(path);
		if (findResult == audioBuffers.end())
		{
			bufferIdx = GameBase::get().getAssetSystem().loadOpenAlBuffer(path);
			audioBuffers.insert({ path, bufferIdx });
		}
		else
		{
			bufferIdx = findResult->second;
		}

		emitter.hardwareData.bufferIdx = bufferIdx;

		if (emitter.hardwareData.sourceIdx.has_value() && emitter.hardwareData.bufferIdx.has_value())
		{
			//we only need update listener if we have a listener and a valid buffer
			//this will probably cause an abrupt change in sound; to fix that we should fade out and fade in new sound.
			//not sure if changing sound on an emitter will be an active use case though as it will require probably tweaking all emitter properties
			//so for now just setting source's value directly
			alec(alSourcei(*emitter.hardwareData.sourceIdx, AL_BUFFER, *emitter.hardwareData.bufferIdx)); //#audiothreads update to listener should happen in dedicated audio thread processing
		}

#endif //USE_OPENAL_API
	}

	void AudioSystem::tickAudioPipeline(float dt_sec)
	{
		//trying out a pipelined approach to writing this function; primary goal is to communicate the highlevel via code not comments
		audioTick_beginPipeline();
		audioTick_updateListenerStates();
		audioTick_updateEmitterStates(dt_sec);
		audioTick_sortActivatedEmitters();				
		audioTick_cullEmitters();							
		audioTick_updateFadeOut();						
		audioTick_updateFadeIn();						
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
				md.dirty_bPosition = md.dirty_bVelocity = true;
			}
			else
			{
				//make dirty if we swapped from being mapped to no being mapped, also if we're already dirty then we need to refresh
				md.dirty_bPosition |= md.bPreviousFrameListenerMapped;
				md.dirty_bVelocity |= md.bPreviousFrameListenerMapped;

				//pass through the positioning data
				md.position_listenerCorrected = ud.position;
				md.velocity_listenerCorrected = ud.velocity;
				md.bPreviousFrameListenerMapped = false; //must be after dirty check!
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Update Audio API
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (md.dirty_bPosition)
			{
				glm::vec3& pos = md.position_listenerCorrected;
				alec(alSource3f(src, AL_POSITION, pos.x, pos.y, pos.z)); //#TODO #scenenodes
				md.dirty_bPosition = false;
			}
			if (md.dirty_bVelocity)
			{ 
				glm::vec3& vel = md.velocity_listenerCorrected;
				alec(alSource3f(src, AL_VELOCITY, vel.x, vel.y, vel.z));
				md.dirty_bVelocity = false;
			}
			if (md.dirty_bPitch || bDirtyAllPitch) 
			{ 
				float pitch = calculatePitch(ud.pitch);
				alec(alSourcef(src, AL_PITCH, pitch));
				md.dirty_bPosition = false;
			}
			if (md.dirty_bGain) 
			{ 
				float gain = glm::clamp(calculateGain(ud.volume, ud.bIsMusic) /** md.soundFadeModifier*/, 0.f, 1.f);
#if !IGNORE_AUDIO_COMPILE_TODOS
				todo_renable_fade;
#endif

				alec(alSourcef(src, AL_GAIN, gain));
				md.dirty_bGain = false; 
			}
			if (md.dirty_bLooping) 
			{ 
				alec(alSourcei(src, AL_LOOPING, ALint(ud.bLooping)));
				md.dirty_bLooping = false; 
			}
			if (md.dirty_bReferenceDistance)
			{
				alec(alSourcei(src, AL_REFERENCE_DISTANCE, ALuint(md.referenceDistance)));
				md.dirty_bReferenceDistance = false;
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "updating AL source but no source index available!");
			log(__FUNCTION__, LogLevel::LOG_ERROR, emitterSource.userData.sfxAssetPath.size() > 0 ? emitterSource.userData.sfxAssetPath.c_str() : "no asset path");
		}
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
		list_HardwarePermitted.reserve(api_MaxMonoSources);
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
				ALuint source = *emitter->hardwareData.sourceIdx;
				alec(alDeleteSources(1, &source));
			}
		}

		//drain the pool of sources that can be claimed
		while (std::optional<ALuint> optionalSource= sourcePool.getInstance())
		{
			ALuint source = *optionalSource;
			alec(alDeleteSources(1, &source));
		}
		logf_sa(__FUNCTION__, LogLevel::LOG, "end cleaning up openal sources");

		logf_sa(__FUNCTION__, LogLevel::LOG, "begin cleanup al buffers");
		GameBase::get().getAssetSystem().unloadAllOpenALBuffers();
		logf_sa(__FUNCTION__, LogLevel::LOG, "end cleanup al buffers");

		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		alcCloseDevice(device);
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
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//update player positioning
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		const std::vector<sp<PlayerBase>>& allPlayers = GameBase::get().getPlayerSystem().getAllPlayers();
		listenerData.clear();

		for (const sp<PlayerBase>& player: allPlayers)
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
	}

	void AudioSystem::audioTick_updateListenerStates()
	{
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

	void AudioSystem::audioTick_updateEmitterStates(float dt_sec)
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//consume the pending activations before prioritization happens
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		for (const sp<AudioEmitter>& sound : list_pendingUserActivation)
		{
			//user code may have deactivated emitter between its addition to this set and this tick.
			if (sound->systemMetaData.bActive)
			{
				list_userActivatedSounds.push_back(sound);
			}
		}
		list_pendingUserActivation.clear();

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//walk the list backwards so we can do swap-and-pop on deactivated sounds
		//starting from the back means that we can count on the last elements already having their state recalculated
		//unsigned wrap around
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		constexpr size_t limit = std::numeric_limits<size_t>::max();
		for (size_t idx = list_userActivatedSounds.size() - 1; idx < list_userActivatedSounds.size() && idx != limit; --idx)
		{
			const sp<AudioEmitter> emitter = list_userActivatedSounds[idx];
			if (emitter)
			{
				if (list_userActivatedSounds[idx]->systemMetaData.bActive)
				{
					EmitterAudioSystemMetaData& soundMetaData = list_userActivatedSounds[idx]->systemMetaData;
					const EmitterUserData& soundUserData = list_userActivatedSounds[idx]->userData;

					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// tick fade changes from previous frame prioritization
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					soundMetaData.soundFadeModifier -= dt_sec * soundMetaData.fadeDirection * soundMetaData.fadeRateSecs;
					soundMetaData.soundFadeModifier = glm::clamp(soundMetaData.soundFadeModifier, 0.f, 1.f);

					////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					// Prioritize this sound so that it can be culled if resources are required for higher priority sounds
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////

					//calculate new priority; start a 0 and add to priority
					soundMetaData.calculatedPriority = 0;

					ListenerData* closestListener = nullptr;
					float closestDist2 = std::numeric_limits<float>::infinity();
					size_t closestListenerIdx = 0;
					for (size_t idx = 0; idx < listenerData.size(); ++idx)
					{
						ListenerData& listenerDatum = listenerData[idx];
						float length2 = glm::length2(listenerDatum.position - soundUserData.position);
						if (length2 < closestDist2)
						{
							closestListener = &listenerDatum;
							closestDist2 = length2;
							closestListenerIdx = idx;
						}
					}

					//0.X000 <-takes this position on priority assignment. 
					float proximityValue = 0.f;
					bool bOutOfRange = true;

					//did we find any close listener?
					if (closestListener)
					{
						const float radiusSquared = Utils::square(soundUserData.maxRadius);
						soundMetaData.closestListenerIdx = closestListenerIdx;
						if (closestDist2 < radiusSquared)
						{
							bOutOfRange = false;
							float distance = glm::length(soundUserData.position - closestListener->position);
							float distanceAlpha = 1.0f - glm::clamp(distance / soundUserData.maxRadius, 0.f, 1.f); 

							//soundMetaData.bPreviousFrameListenerMapped = DO not set this here, we will detect we need remapping via index and use this to determine whether we need to remap; 

							//may need to rethink this a bit, perhaps multiplying things by 10,100,1000 based on priority level.
							soundMetaData.calculatedPriority += distanceAlpha; //[0,1] range already is in the tenths position
						}
						else
						{
							//change the distance max distance when trying to drop sounds so that sounds on border of radius do not flicker in/out
							soundMetaData.bOutOfRange = closestDist2 * 1.01 < radiusSquared;
						}
					}
					
					soundMetaData.calculatedPriority += soundUserData.priority;
				}
				else
				{
					Utils::swapAndPopback(list_userActivatedSounds, idx);
					--idx; //back up so that we process to item we just swapped into this position
				}
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_WARNING, "unexpected nullptr emitter in audio pipeline");
				Utils::swapAndPopback(list_userActivatedSounds, idx);
			}
		}

	}

	void AudioSystem::audioTick_sortActivatedEmitters()
	{
		std::sort(
			list_userActivatedSounds.begin(),
			list_userActivatedSounds.end(),
			[](const sp<AudioEmitter>& first, const sp<AudioEmitter>& second) 
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
		size_t availableSources = api_MaxMonoSources;

		list_HardwarePermitted.clear();
		list_pendingRemoveHardwareSource.clear();
		list_pendingAssignHardwareSource.clear(); 

		for (size_t idx = 0; idx < list_userActivatedSounds.size(); ++idx)
		{
			const sp<AudioEmitter>& sound = list_userActivatedSounds[idx];

			//always reset fade state to fade up; we will fade out if needed. But this restores things that may haev filpped from a fade out to a fadein
			sound->systemMetaData.fadeDirection = 1.f;

			bool bSoundSafeFromCull = idx < availableSources && !sound->systemMetaData.bOutOfRange || !sound->systemMetaData.bActive;
			if (bSoundSafeFromCull)
			{
				//if this doesn't have a hardware source, then give it one
				if (!sound->hardwareData.sourceIdx.has_value())
				{
					list_pendingAssignHardwareSource.push_back(sound.get());
					CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("sound requesting hardware resource %p", sound.get());
				}
				else
				{
					//since this has hardware resources, add it to the list that will have their sources updated
					list_HardwarePermitted.push_back(sound.get());
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

	void AudioSystem::audioTick_updateFadeOut()
{
		//these will have their fade percentage drained over time
		for (AudioEmitter* emitter : list_pendingRemoveHardwareSource)
		{
			if (emitter && emitter->systemMetaData.soundFadeModifier <= 0.f)
			{
				if (emitter->hardwareData.sourceIdx.has_value())
				{
					ALuint source = *emitter->hardwareData.sourceIdx;
					emitter->hardwareData.sourceIdx.reset();

					sourcePool.releaseInstance(source);

					//make sure the source is no longer player, when this is pulled from the pool it will be played if necessary
					alec(alSourceStop(source));

					CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("fade out complete %p", emitter);
				}
				else{STOP_DEBUGGER_HERE();} //why is there a emitter fading out that doesn't have a source?
			}
		}
	}

	void AudioSystem::audioTick_updateFadeIn()
	{
		if (bool bThereAreAvailableHardwareSources = list_HardwarePermitted.size() < api_MaxMonoSources)
		{
			for (AudioEmitter* emitter : list_pendingAssignHardwareSource)
			{
				size_t freeSources = api_MaxMonoSources - list_HardwarePermitted.size();

				if (freeSources > 0)
				{
#if USE_OPENAL_API
					std::optional<ALuint> instance = sourcePool.getInstance();
					if (!instance.has_value())
					{
						ALuint newSource = 0;
						alec(alGenSources(1, &newSource));
						if (newSource)
						{
							instance = newSource;
						}
					}

					if (instance.has_value() && emitter)
					{
						emitter->hardwareData.sourceIdx = instance;
						ALuint source = *instance;

						//for looping sounds, start them quiet and fade in
						emitter->systemMetaData.soundFadeModifier = emitter->userData.bLooping ? 0.f : 1.f;

						trySetEmitterBuffer(*emitter, EmitterPrivateKey{});

						alec(alSourcePlay(source));

						//this now has a hardware resource, add it to list of sources we will update at the end of the pipeline
						list_HardwarePermitted.push_back(emitter);

						CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("fade in assigned hardware source %d to %p", source, emitter);
					}
#endif
				}
				else
				{
					CONDITIONAL_VERYVERBOSE_RESOURCE_LOG_MESSAGE("fade in has no sources available. need %d", list_pendingAssignHardwareSource.size());
					break; //we used up all the sources that were available
				}
			}
		}
	}

	void AudioSystem::audioTick_updateEmittersWithHardwareResources()
	{
		//walk over the hardware permitted sounds list and update the sound buffers (platform and api specific)
		for (AudioEmitter* emitterSource : list_HardwarePermitted)
		{
			if (emitterSource)
			{
				updateSourceProperties(*emitterSource);
			}
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
			if (allEmitters[idx].use_count() <= 1)
			{
				gcIndices.push_back(idx);
			}
		}

		for (size_t idx : gcIndices)
		{
			CONDITIONAL_VERBOSE_RESOURCE_LOG_MESSAGE("Removing emitter with no users %p", allEmitters[idx].get());
			Utils::swapAndPopback(allEmitters, idx);
		}

	}

	void AudioSystem::audioTick_endPipeline()
	{
		//if we were dirtying all pitch (because time dilation changed) then do not do that on next frame
		bDirtyAllPitch = false;
	}

	void AudioSystem::handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel)
	{
		if (currentLevel)
		{
			currentLevel->getWorldTimeManager()->getEvent(TickGroups::get().AUDIO).removeWeak(sp_this(), &AudioSystem::tickAudioPipeline);
		}

		if (newLevel)
		{
			newLevel->getWorldTimeManager()->getEvent(TickGroups::get().AUDIO).addWeakObj(sp_this(), &AudioSystem::tickAudioPipeline);
		}
	}

}

