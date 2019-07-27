#pragma once
#include "SASystemBase.h"
#include "../Game/AssetConfigs/SAConfigBase.h"
#include<optional>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>
#include "../Tools/DataStructures/SATransform.h"
#include "SAGameEntity.h"

#define DISABLE_PARTICLE_SYSTEM 1

namespace SA
{
	class LevelBase;
	class ShapeMesh;
	class Shader;
	class Window;

	struct ActiveParticleGroup;
	struct MutableEffectData;
	
	namespace Particle
	{
		struct Effect;
	}

	
	/////////////////////////////////////////////////////////////////////////////////////
	// Structure for storing arbitrary instance data.
	//
	// Instanced data allows rendering a large number of objects (models, meshes) with a single draw call.
	// 
	//		NOTE: for performance, vector.reserve() an estimate of the number of instances 
	/////////////////////////////////////////////////////////////////////////////////////
	struct EffectInstanceData
	{
		friend class ParticleSystem; //allow particle system to view this as a struct

		sp<Particle::Effect> effectData; 

		//order for which data is applied vertex attributes is the top-to-bottom order of this class.
		size_t numMat4PerInstance = 2; //#TODO increment in customer steps
		std::vector<glm::mat4> mat4Data;

		size_t numVec4PerInstance = 0; //#TODO increment in customer steps
		std::vector<glm::vec4> vec4Data;

		size_t numVec3PerInstance = 0; //#TODO increment in customer steps
		std::vector<glm::vec3> vec3Data;

		size_t numFloatPerInstance = 0; //#TODO increment in customer steps
		std::vector<float> floatData;

		//#TODO #BEFORE_SUBMIT make sure these vectors get size reserved somewhere; before tick? after tick to see actual size? init?
		std::vector<float> timeAlive;
		size_t numInstancesThisFrame = 0;

		void clearFrameData()
		{
			numInstancesThisFrame = 0;
			timeAlive.clear();
			mat4Data.clear();
			vec4Data.clear();
			vec3Data.clear();
			floatData.clear();
		}
	};

	namespace Particle
	{
		template<typename T>
		struct KeyFrame
		{
			T startValue;
			T endValue;
			float durationSec;
			//#TODO curve LUT reference? rather than just linear interpolation
			size_t dataIdx;

			inline void update(std::vector<T> elementArray, float effectTime)
			{

			}
		};

		//represents sequential key-frames
		struct KeyFrameChain
		{
			std::vector<KeyFrame<float>> floatKeyFrames;
			std::vector<KeyFrame<glm::vec3>> vec3KeyFrames;
			std::vector<KeyFrame<glm::vec4>> vec4KeyFrames;
			std::vector<KeyFrame<glm::mat4>> mat4KeyFrames;

			bool update(MutableEffectData& particle, float timeAliveSecs);

			template <typename T>
			inline bool updateFrames(std::vector<T>& elementArray, std::vector<KeyFrame<T>> frames, float timeAlive);
		};

		/////////////////////////////////////////////////////////////////////////////////////
		// Particle Effects are a component of a particle. 
		//		A particle may have many simultaneous effects going on. 
		//		For example, an explosion may have multiple growing fire spheres
		//      and smoke effects at the same time.
		/////////////////////////////////////////////////////////////////////////////////////
		struct Effect
		{
			sp<Shader> shader;
			sp<ShapeMesh> mesh;
			//sp<material> material; //TODO

			/** concurrent animation keyframes */
			std::vector<KeyFrameChain> keyFrameChains;

			//#TODO implement this
			size_t numMat4sPerInstance = 0;
			size_t numVec4sPerInstance = 0;

		public: //perf helper fields
			size_t estimateMaxSimultaneousEffects = 250;

		private: //particle system managed data for efficient rendering
			//#TODO this whole object probably needs copying to prevent it from being corrupted. :\ even still the user may recycle shaders and corrupt the particle system.
			friend class ParticleSystem;
			std::optional<size_t> assignedShaderIndex;
		};
	}

	///////////////////////////////////////////////////////////////////////
	// A particle asset that is used to render a specific particle effect
	//		Instances of this object are intended to be shared. It defines 
	//		the configuration of a particular effect.5
	///////////////////////////////////////////////////////////////////////
	class ParticleConfig : public ConfigBase
	{
		friend class ParticleFactory;
		friend class ParticleSystem;

		//#FUTURE serialization of these are not currently supported; this will require a lot of work to make an editor
	public:
		virtual std::string getRepresentativeFilePath() override;

		/* Generates data for the effect that can be manipulated over time */
		void generateMutableEffectData(std::vector<MutableEffectData>& outEffectData) const;

	protected:
		virtual void onSerialize(json& outData) override;
		virtual void onDeserialize(const json& inData) override;

	private:
		std::vector<sp<Particle::Effect>> effects;
	};

	struct MutableEffectData
	{
		////////////////////////////////////////////////////////
		// location constants
		////////////////////////////////////////////////////////
		const static size_t POS_VEC3_IDX = 0;
		const static size_t ROT_VEC3_IDX = 1;
		const static size_t SCALE_VEC3_IDX = 2;

		//#TODO make sure this gets initialized
		std::vector<float> floatsArray;
		std::vector<glm::vec3> vec3Array;
		std::vector<glm::vec4> vec4Array;
		std::vector<glm::mat4> mat4Array;
	};

	//#TODO document after system gets fleshed out
	struct ActiveParticleGroup
	{
		friend class ParticleSystem; //#TODO may not should be treated as a struct; will see when system fleshes out

		////////////////////////////////////////////////////////
		// data
		////////////////////////////////////////////////////////
		std::vector<MutableEffectData> mutableEffectData;

		sp<ParticleConfig> particle{ nullptr };
		glm::mat4 parentTransform{ 1.f };
		std::optional<glm::vec3> velocity;
		Transform xform{};
		float timeAlive = 0.f;
	};

	///////////////////////////////////////////////////////////////////////
	// Particle System
	//		responsible for efficiently managing and rendering particles.
	///////////////////////////////////////////////////////////////////////
	class ParticleSystem : public SystemBase
	{
	public:
		struct SpawnParams
		{
			sp<ParticleConfig> particle{nullptr};
			glm::mat4 parentTransform{1.f};
			std::optional<glm::vec3> velocity;
			Transform xform{};
		};

		void spawnParticle(const SpawnParams& params);

	private:
		virtual void postConstruct() override;
		virtual void initSystem() override;
		virtual void tick(float deltaSec) override;
		void updateActiveParticleGroup(ActiveParticleGroup& particleGroup, float dt_sec_world);
		virtual void handlePostRender() override;

	private: //utility functions
		int getNextInstancedShaderIndex();
	
	private:
		void handlePostLevelChange(const sp<LevelBase>& /*previousLevel*/, const sp<LevelBase>& /*newCurrentLevel*/);
		sp<LevelBase> currentLevel = nullptr;

		void handleLosingOpenglContext(const sp<Window>& window);
		void handleAcquiredOpenglContext(const sp<Window>& window);

		//currently the active particle and its spawn params are identical; so just renaming the type
		//#TODO #BEFORE_SUBMIT !!!!!!!!!!!!!!!!! change this data structure to be efficient
		std::vector<ActiveParticleGroup> activeParticles;
		std::size_t particleReserveSize = 10000;



		/////////////////////////////////////////////////////////////////////////////////////
		// Map from shader instance to index in array of EffectInstanceData
		// Each effect has its own shader that must be bound for rendering; thus everything 
		// about an effect is associated with the shader responsible for the effect. 
		/////////////////////////////////////////////////////////////////////////////////////
		std::map<sp<Shader>, size_t> shaderToInstanceDataIndex;

		/////////////////////////////////////////////////////////////////////////////////////
		// data for instanced rendering; the contained data is cleared and regenerated each 
		// frame and used for drawing a large number of particles. 
		/////////////////////////////////////////////////////////////////////////////////////
		std::vector<EffectInstanceData> instancedEffectsData;
		std::optional<unsigned int> instanceMat4VBO_opt; //#TODO unset when losing glContext/ set when gaining glContext
		int maxVertAttributes;
	};


	///////////////////////////////////////////////////////////////////////
	// Built-in particle factory
	///////////////////////////////////////////////////////////////////////
	class ParticleFactory
	{
	public:
		static sp<ParticleConfig> getSimpleExplosionEffect();
	};















































	template <typename T>
	inline bool Particle::KeyFrameChain::updateFrames(std::vector<T>& elementArray, std::vector<Particle::KeyFrame<T>> frames, float timeAlive)
	{
		bool frameTypeIsDone = true;
		float chainTime = 0;
		for (Particle::KeyFrame<T>& thisKF : frames)
		{
			//find chain for this time
			if (timeAlive <= thisKF.durationSec + chainTime)
			{
				float effectTime = timeAlive - chainTime;

				float a = effectTime / thisKF.durationSec;
				//#FUTURE curve LUT  ; a = LUT[linear_interp_to_LUT_idx] ;
				T newValue = a * (thisKF.endValue - thisKF.startValue) + thisKF.startValue;
				elementArray[thisKF.dataIdx] = newValue;
				frameTypeIsDone = false;
				break;
			}
			else
			{
				chainTime += thisKF.durationSec;
			}
		}

		return frameTypeIsDone;
	}

































}