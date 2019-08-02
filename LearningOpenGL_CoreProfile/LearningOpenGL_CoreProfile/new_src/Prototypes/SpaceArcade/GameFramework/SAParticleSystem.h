#pragma once
#include "SASystemBase.h"

#include<optional>
#include<cstdint>
#include <stack>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "SAGameEntity.h"
#include "../Tools/DataStructures/SATransform.h"
#include "../Game/AssetConfigs/SAConfigBase.h"

#define DISABLE_PARTICLE_SYSTEM 0

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
		size_t numMat4PerInstance = 1; //#TODO increment in custom steps
		std::vector<glm::mat4> mat4Data;

		size_t numVec4PerInstance = 1; //#TODO increment in custom steps
		std::vector<glm::vec4> vec4Data;

		//#TODO #BEFORE_SUBMIT make sure these vectors get size reserved somewhere; before tick? after tick to see actual size? init?
		std::vector<float> timeAlive;
		size_t numInstancesThisFrame = 0;
 
		void clearFrameData()
		{
			numInstancesThisFrame = 0;
			timeAlive.clear();
			mat4Data.clear();
			vec4Data.clear();
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
		// Partical material; For the particle system this essentially that represents a texture
		// with configured state. If the particle effect require multiple textures then it will
		// use multiple materials.
		/////////////////////////////////////////////////////////////////////////////////////
		struct Material
		{
			//force user construct with required arguments for a valid material
			Material(uint32_t inTextureId, std::string inSamplerName) 
				: textureId(inTextureId), sampler2D_name(inSamplerName) 
			{}
			uint32_t textureId;
			std::string sampler2D_name;
			//#future fill with optionals for parameters that may need to be set
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
			std::vector<Material> materials;

			/** concurrent animation keyframes */
			std::vector<KeyFrameChain> keyFrameChains;

			//#TODO implement non-uniform custom vertex attributes; don't have a use case yet but I imagine there will be one.
			size_t numCustomMat4sPerInstance = 0;
			size_t numCustomVec4sPerInstance = 0;

		public: //perf helper fields
			size_t estimateMaxSimultaneousEffects = 250;

		public:
			void updateEffectDuration();
			std::optional<float> effectDuration;

		private: //particle system managed data for efficient rendering
			//#concerns this whole object probably needs copying disabled to prevent it from being corrupted. 
			//#concerns even with copying disabled the user may recycle shaders and corrupt the particle system.
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
		float getDurationSecs();

		void handleDirtyValues();

	protected:
		virtual void onSerialize(json& outData) override;
		virtual void onDeserialize(const json& inData) override;

	private:
		std::vector<sp<Particle::Effect>> effects;
		bool bLoop = false;
		std::optional<int> numLoops;
		std::optional<float> totalTime;
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
		int bLoopCount = 0;
	};

	///////////////////////////////////////////////////////////////////////
	// Particle System
	//		responsible for efficiently managing and rendering particles.
	//
	// Usage:
	//		Particles are spawned from this system. 
	//		Particles are defined via ParticleConfigs
	//		Particles are made up of particle effects.
	//		Different particle effects MUST NOT share a single shader instance.
	//			effect shaders are mapped by address in order to achieve instancing
	//		Instanced particles only require 1 draw call and are highly efficient in that regard.
	//			More specifically, particle effects are instanced.
	//			this basically means all the different effect instances are batched together and rendered at once.
	//			This requires that all effects share the same shader. In this context uniforms are global to all
	//			instances of the effect; vertex attributes must be used to specify instance-specific data.
	//		See the provided explosion effect in the particle factory for engineering an effect.
	///////////////////////////////////////////////////////////////////////
	class ParticleSystem : public SystemBase
	{
	public:
		struct SpawnParams
		{
			sp<ParticleConfig> particle{nullptr};
			glm::mat4 parentTransform{1.f};
			std::optional<glm::vec3> velocity{};
			Transform xform{};
		};

		void spawnParticle(const SpawnParams& params);

	private:
		virtual void postConstruct() override;
		virtual void initSystem() override;
		virtual void tick(float deltaSec) override;
		inline bool updateActiveParticleGroup(ActiveParticleGroup& particleGroup, float dt_sec_world);
		virtual void handlePostRender() override;

	private: //utility functions
	
	private:
		void handlePostLevelChange(const sp<LevelBase>& /*previousLevel*/, const sp<LevelBase>& /*newCurrentLevel*/);
		sp<LevelBase> currentLevel = nullptr;

		void handleLosingOpenglContext(const sp<Window>& window);
		void handleAcquiredOpenglContext(const sp<Window>& window);

		//currently the active particle and its spawn params are identical; so just renaming the type
		//#TODO #BEFORE_SUBMIT !!!!!!!!!!!!!!!!! change this data structure to be efficient
		std::map<ActiveParticleGroup*, sp<ActiveParticleGroup>> activeParticles; //#TODO perhaps hashmap will be better for inserts/removals, but suffers iteration

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
		std::optional<unsigned int> instanceMat4VBO_opt;
		std::optional<unsigned int> instanceVec4VBO_opt;
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