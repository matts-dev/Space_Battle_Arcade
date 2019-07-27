#include "SAParticleSystem.h"
#include "SALevelSystem.h"
#include "SAGameBase.h"
#include "SALog.h"
#include "SALevel.h"
#include "../Tools/Geometry/SimpleShapes.h"
#include <assert.h>
#include "../Tools/SAUtilities.h"
#include "SAPlayerSystem.h"
#include "../Rendering/Camera/SACameraBase.h"
#include "SAPlayerBase.h"
#include "SAWindowSystem.h"
#include "../Rendering/OpenGLHelpers.h"

namespace SA
{
	/////////////////////////////////////////////////////////////////////////////
	// Particle Config Base
	/////////////////////////////////////////////////////////////////////////////
	std::string ParticleConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("Assets/Particles/") + name + std::string(".json");
	}

	void ParticleConfig::generateMutableEffectData(std::vector<MutableEffectData>& outEffectData) const
	{
		outEffectData.clear();
		outEffectData.reserve(effects.size());

		for (const sp<Particle::Effect>& effect : effects)
		{
			outEffectData.emplace_back();
			MutableEffectData& med = outEffectData.back();

			//Reserve built-in locations
			med.vec3Array.resize(3);
			med.vec3Array[MutableEffectData::POS_VEC3_IDX] = { 0, 0, 0 }; //position
			med.vec3Array[MutableEffectData::ROT_VEC3_IDX] = { 0, 0, 0 }; //rotation
			med.vec3Array[MutableEffectData::SCALE_VEC3_IDX] = { 1, 1, 1 }; //scale

			//#TODO reserve custom data locations
		}
	}

	void ParticleConfig::onSerialize(json& outData)
	{
	}

	void ParticleConfig::onDeserialize(const json& inData)
	{
	}

	/////////////////////////////////////////////////////////////////////////////
	// Particle System 
	/////////////////////////////////////////////////////////////////////////////

	void ParticleSystem::spawnParticle(const SpawnParams& params)
	{
#if DISABLE_PARTICLE_SYSTEM
		return;
#endif //DISABLE_PARTICLE_SYSTEM

		if (params.particle)
		{
			//validate particle has all necessary data; early out if not
			for (sp<Particle::Effect>& effect : params.particle->effects)
			{
				if (!effect->shader)
				{
					log("Particle System", LogLevel::LOG_ERROR, "Particle spawned with no shader");
					return;
				}
			}

			for (sp<Particle::Effect>& effect : params.particle->effects)
			{
				auto findIter = shaderToInstanceDataIndex.find(effect->shader);

				/// generate effectShader-to-dataEntry if one doesn't exist 
				if (findIter == shaderToInstanceDataIndex.end())
				{
					//#TODO check free list of shader idxs, if so use that and don't emplace back
					size_t nextId = instancedEffectsData.size();
					instancedEffectsData.emplace_back();
					effect->assignedShaderIndex = nextId;

					shaderToInstanceDataIndex[effect->shader] = nextId;
					findIter = shaderToInstanceDataIndex.find(effect->shader);

					//set up the configured instance data to contain correct attributes and uniforms
					// #TODO perhaps set this up in an "init/ctor" function on EffectInstanceData class
					EffectInstanceData& EIData = instancedEffectsData.back();
					EIData.numMat4PerInstance = effect->numMat4sPerInstance;
					EIData.numVec4PerInstance = effect->numVec4sPerInstance;
					EIData.effectData = effect;

					//#TODO std::vector reserve data? need system for reserving more data if needed 
					EIData.mat4Data.reserve(EIData.numMat4PerInstance * effect->estimateMaxSimultaneousEffects);
					EIData.vec4Data.reserve(EIData.numVec4PerInstance * effect->estimateMaxSimultaneousEffects);

					EIData.numInstancesThisFrame = 0;
				}

				//find iter is now definitely pointing to data
				int dataIdx = findIter->second;

				// populate data into entry
				EffectInstanceData& EIData = instancedEffectsData[dataIdx];
				EIData.numInstancesThisFrame++;

			}

			//Configure Active Particle
			activeParticles.emplace_back();
			ActiveParticleGroup& newParticle = activeParticles.back();
			newParticle.particle = params.particle;
			newParticle.parentTransform = params.parentTransform;
			newParticle.velocity = params.velocity;
			newParticle.xform = params.xform;
			params.particle->generateMutableEffectData(newParticle.mutableEffectData);
			assert(newParticle.mutableEffectData.size() == params.particle->effects.size());
			updateActiveParticleGroup(newParticle, 0);
		}
	}

	void ParticleSystem::postConstruct()
	{
		//prevent buffer resizing; #suggested make the reserve size configurable
		activeParticles.reserve(particleReserveSize);
	}

	void ParticleSystem::tick(float deltaSec)
	{
		using KeyFrameChain = Particle::KeyFrameChain;

		static PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
		const sp<PlayerBase>& player = playerSystem.getPlayer(0);
		const sp<CameraBase> camera = player ? player->getCamera() : sp<CameraBase>(nullptr); //#TODO perhaps just listen to camera changing

		//#TODO move to post-tick so new effects can be generated

		//#suggested #todo macroify/encapsulate this so it can be done in multiple palces
		static bool reserveSizeWarning = false;
		if (!reserveSizeWarning &&  activeParticles.size() > particleReserveSize) 
		{
			reserveSizeWarning = true;
			log("Particle System", LogLevel::LOG_WARNING, "More particles than reserved space -- slow buffer resizing will occur");
		}

		if (currentLevel && camera)
		{
			float dt_sec_world = currentLevel->getWorldTimeManager()->getDeltaTimeSecs();

			for (ActiveParticleGroup& activeParticle : activeParticles)
			{
				//#TODO this needs to be an inline function so it can be called with 0dtsec from spawning projectile
				updateActiveParticleGroup(activeParticle, dt_sec_world);
			}
		}

		//#TODO remove dead particles
		//#TODO tear down all resources when opengl context is lost!
	}

	void ParticleSystem::updateActiveParticleGroup(ActiveParticleGroup& activeParticle, float dt_sec_world)
	{
		if (activeParticle.velocity)
		{
			//#TODO update state since optional is present

		}

		glm::mat4 particleGroupModelMat = activeParticle.xform.getModelMatrix();

		bool bAllEffectsDone = true;

		for (size_t effectIdx = 0; effectIdx < activeParticle.particle->effects.size(); ++effectIdx)
		{
			sp<Particle::Effect>& effect = activeParticle.particle->effects[effectIdx];
			MutableEffectData& meData = activeParticle.mutableEffectData[effectIdx];

			///Let effects update particle data
			activeParticle.timeAlive += dt_sec_world;

			for (Particle::KeyFrameChain& KFChain : effect->keyFrameChains)
			{
				bAllEffectsDone &= KFChain.update(meData, activeParticle.timeAlive);
			}

			///package active particle mutable data into instanced rendering format; 
			///#future this may need to be a 2-pass thing so we can sort distance for transparency effects
			EffectInstanceData& effectData = instancedEffectsData[*effect->assignedShaderIndex];
			int thisInstanceIdx = effectData.numInstancesThisFrame;

			effectData.numInstancesThisFrame += 1;
			effectData.timeAlive.push_back(activeParticle.timeAlive);

			Transform effectXform;
			effectXform.position = meData.vec3Array[MutableEffectData::POS_VEC3_IDX];
			effectXform.rotQuat = Utils::degreesVecToQuat(meData.vec3Array[MutableEffectData::ROT_VEC3_IDX]);
			effectXform.scale = meData.vec3Array[MutableEffectData::SCALE_VEC3_IDX];
			glm::mat4 effectWorldModelMat = particleGroupModelMat * effectXform.getModelMatrix();

			//package matrix data 
			effectData.mat4Data.push_back(effectWorldModelMat);	//first matrix is always model
																//second matrix can be defined by the user 

			//package vec3 data

			//#TODO package custom data?
		}

		if (bAllEffectsDone)
		{
			//#TODO flag removals / loops
		}

		//#TODO submit to an instanced render
	}

	void ParticleSystem::handlePostRender()
	{
		static PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
		const sp<PlayerBase>& player = playerSystem.getPlayer(0);
		const sp<CameraBase> camera = player ? player->getCamera() : sp<CameraBase>(nullptr); //#TODO perhaps just listen to camera changing


		if (instanceMat4VBO_opt.has_value() && camera)
		{
			const glm::mat4 projection_view = camera->getPerspective() * camera->getView(); //calculate early 
 			const glm::vec3 camPos = camera->getPosition();

			GLuint instanceMat4VBO = *instanceMat4VBO_opt;
			for(EffectInstanceData& eid : instancedEffectsData)
			{
				//at least 16 attributes to use for vertices. (see glGet documentation). query GL_MAX_VERTEX_ATTRIBS
				//assumed vertex attributes:
				// attribute 0 = pos
				// attribute 1 = norm
				// attribute 2 = uv
				// attribute 3 = tangent
				// attribute 4 = bitangent //can potentially remove by using bitangent = normal cross tangent
				// attribute 5-7 = reserved
				// attribute 8-11 = model matrix
				// attribute 12-15 //last remaining attributes
				
				//#TODO check sizes of EID data before trying to bind optional buffers
				ec(glBindBuffer(GL_ARRAY_BUFFER, instanceMat4VBO));

				//#TODO investigate using glBufferSubData and glMapBuffer and GL_DYNAMIC_DRAW here; may be more efficient if we're not reallocating each time? It appears glBufferData will re-allocate
				ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * eid.mat4Data.size(), &eid.mat4Data[0], GL_STATIC_DRAW));

				GLuint effectVAO = eid.effectData->mesh->getVAO();
				ec(glBindVertexArray(effectVAO));

				//#TODO set num vec4s in stride based on custom data
				GLsizei numVec4AttribsInEID = 4;

				//pass built-in data into instanced array vertex attribute
				{
					//mat4 (these take 4 separate vec4s)
					{
						//model matrix
						ec(glEnableVertexAttribArray(8));
						ec(glEnableVertexAttribArray(9));
						ec(glEnableVertexAttribArray(10));
						ec(glEnableVertexAttribArray(11));

						ec(glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInEID * sizeof(glm::vec4), reinterpret_cast<void*>(0)));
						ec(glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInEID * sizeof(glm::vec4), reinterpret_cast<void*>(sizeof(glm::vec4))));
						ec(glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInEID * sizeof(glm::vec4), reinterpret_cast<void*>(2*sizeof(glm::vec4))));
						ec(glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInEID * sizeof(glm::vec4), reinterpret_cast<void*>(3*sizeof(glm::vec4))));

						ec(glVertexAttribDivisor(8, 1));
						ec(glVertexAttribDivisor(9, 1));
						ec(glVertexAttribDivisor(10, 1));
						ec(glVertexAttribDivisor(11, 1));

					}
				}

				//#TODO max instance variables check
				//pass custom data into instanced array
				{

				}

				//activate shader
				sp<Shader>& shader = eid.effectData->shader;
				shader->use();

				//supply uniforms (important that we use uniforms for shared data and not attributes as we do not want run out of attribute space)
				shader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(projection_view));
				shader->setUniform3f("camPos", camPos.x, camPos.y, camPos.z);
					
				//instanced render
				eid.effectData->mesh->instanceRender(eid.numInstancesThisFrame);
				eid.clearFrameData();
			}
		}
		ec(glBindVertexArray(0));//unbindg VAO's
	}

	void ParticleSystem::initSystem()
	{
		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		levelSystem.onPostLevelChange.addWeakObj(sp_this(), &ParticleSystem::handlePostLevelChange);

		WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		windowSystem.onWindowAcquiredOpenglContext.addWeakObj(sp_this(), &ParticleSystem::handleAcquiredOpenglContext);
		windowSystem.onWindowLosingOpenglContext.addWeakObj(sp_this(), &ParticleSystem::handleLosingOpenglContext);
		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			handleAcquiredOpenglContext(primaryWindow);
		}
		
		GameBase::get().subscribePostRender(sp_this());
	}

	void ParticleSystem::handlePostLevelChange(const sp<LevelBase>& /*previousLevel*/, const sp<LevelBase>& newCurrentLevel)
	{
		log("ParticleSystem", LogLevel::LOG, "Detected level change, clearing particles");

		activeParticles.clear();
		currentLevel = newCurrentLevel;
	}

	void ParticleSystem::handleLosingOpenglContext(const sp<Window>& window)
	{
		if (instanceMat4VBO_opt.has_value())
		{
			ec(glDeleteBuffers(1, &(*instanceMat4VBO_opt)));
			instanceMat4VBO_opt.reset();
		}
	}

	void ParticleSystem::handleAcquiredOpenglContext(const sp<Window>& window)
	{
		if (!instanceMat4VBO_opt.has_value())
		{
			unsigned int instanceVBO;
			ec(glGenBuffers(1, &instanceVBO));
			ec(glBindBuffer(GL_ARRAY_BUFFER, instanceVBO));
			instanceMat4VBO_opt = instanceVBO;

			ec(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertAttributes));
			//log("ParticleSystem", LogLevel::LOG, "Max Vertex Attributes To Use");
		}
		else
		{
			log("ParticleSystem", LogLevel::LOG_ERROR, "Particle System acquired opengl context but already has instance VBO");
		}
	}

	///////////////////////////////////////////////////////////////////////
	// particle factory
	///////////////////////////////////////////////////////////////////////

	static char const* const SimpleExplosionVS_src = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec3 normal;
				layout (location = 2) in vec2 uv;
                //layout (location = 3) in vec3 tangent;
                //layout (location = 4) in vec3 bitangent; //may can get 1 more attribute back by calculating this cross(normal, tangent);
					//layout (location = 5) in vec3 reserved;
					//layout (location = 6) in vec3 reserved;
					//layout (location = 7) in vec3 reserved;
				layout (location = 8) in mat4 model; //consumes attribute locations 8,9,10,11
					//layout (location = 12) in vec3 reserved;
					//layout (location = 13) in vec3 reserved;
					//layout (location = 14) in vec3 reserved;
					//layout (location = 15) in vec3 reserved;

				uniform mat4 projection_view;
				uniform vec3 camPos;

				void main(){
					gl_Position = projection_view * model * vec4(position, 1.0f);
				}
			)";

	static char const* const SimpleExplosionFS_src = R"(
				#version 330 core
				out vec4 fragmentColor;
				uniform vec3 color = vec3(1.f, 0.f, 1.f);
				uniform vec3 camPos;
				void main(){
					fragmentColor = vec4(color, 1.f);
				}
			)";


	sp<ParticleConfig> ParticleFactory::getSimpleExplosionEffect()
	{
		//#TODO define a shader for each effect
		//#TODO figure out clean way to define num mat4s per instance
		//#TODO define number of mat4s expected

		static sp<SphereMeshTextured> sphereMesh = new_sp<SphereMeshTextured>();
		static sp<ParticleConfig> sphereEffect = []() -> sp<ParticleConfig> 
		{
			using glm::vec3; using glm::vec4; using glm::mat4;

			size_t numFloats = 0;
			size_t numVec3s = 0;
			size_t numVec4s = 0;
			size_t numMat4s = 0;

			++numVec3s; //position always idx 0
			++numVec3s; //rotation always idx 1
			++numVec3s; //scale always idx 2

			sp<ParticleConfig> particle = new_sp<ParticleConfig>();

			//new effect  
			particle->effects.push_back(new_sp<Particle::Effect>()); //cpp17 actually returns a reference here
			sp<Particle::Effect>& sphereEffect = particle->effects.back();
			{
				//effect render = sphere
				sphereEffect->mesh = sphereMesh;
				{
					//material : 
						//textured bg
						//texture to make hole-like patterns over time?

					//new keyframe chain -- a series of things to animate sequentially
					sphereEffect->keyFrameChains.emplace_back();
					sphereEffect->shader = new_sp<Shader>(SimpleExplosionVS_src, SimpleExplosionFS_src, false);
					Particle::KeyFrameChain& scaleEffectChain = sphereEffect->keyFrameChains.back();
					{
						//new effect key frame -- a step in the series
						scaleEffectChain.vec3KeyFrames.emplace_back();
						Particle::KeyFrame<vec3>& scaleFrame = scaleEffectChain.vec3KeyFrames.back();
						{
							scaleFrame.startValue = vec3(1, 1, 1);  //#TODO this should start at 0, but for debugging starting visible
							scaleFrame.endValue = vec3(3, 3, 3); //#TODO this will need tweaking, perhaps put at 1?
							//curve		//-x^(1/4)		//#TODO LUT for this curve as exponents would probably be more expensive than LUT cache hit?
							scaleFrame.durationSec = 3.f;
							scaleFrame.dataIdx = MutableEffectData::SCALE_VEC3_IDX;
						}
					}
					//new keyframe chain color
						//new keyframe
							//start		//color = blue
							//end		//color = yellow
							//duration	//0.25 sec
						//new keyframe
							//start		//color = yellow
							//end		//color = red
							//duration	//0.25 sec

				}

			}
				


			return particle;
		}(); //immediately invoke lambda for one-time initialization of local static


		//#TODO validate effect? have a check to make sure all data indices are present
		return sphereEffect;
	}

	

	bool Particle::KeyFrameChain::update(MutableEffectData& meData, float timeAliveSecs)
	{
		bool bFloatsDone = updateFrames<float>(meData.floatsArray, floatKeyFrames, timeAliveSecs);
		bool bVec3Done = updateFrames<glm::vec3>(meData.vec3Array, vec3KeyFrames, timeAliveSecs);
		bool bVec4Done = updateFrames<glm::vec4>(meData.vec4Array, vec4KeyFrames, timeAliveSecs);
		bool bMat4Done = updateFrames<glm::mat4>(meData.mat4Array, mat4KeyFrames, timeAliveSecs);

		return bFloatsDone && bVec3Done &&bVec4Done && bMat4Done;
	}
}

