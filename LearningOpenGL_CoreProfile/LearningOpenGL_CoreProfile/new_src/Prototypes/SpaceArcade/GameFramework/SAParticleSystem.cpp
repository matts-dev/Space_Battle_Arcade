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
#include <stack>
#include "SAAssetSystem.h"

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

	float ParticleConfig::getDurationSecs()
	{
		if (totalTime.has_value())
		{
			return *totalTime;
		}

		float largestTimeSoFar = 0;
		for (sp <Particle::Effect>& effect : effects)
		{
			effect->updateEffectDuration();
			float effectTime = *effect->effectDuration;

			largestTimeSoFar = largestTimeSoFar < effectTime ? effectTime : largestTimeSoFar;
		}

		totalTime = largestTimeSoFar;
		return *totalTime;
	}

	void ParticleConfig::handleDirtyValues()
	{
		totalTime.reset();
		getDurationSecs();
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
					//#future #remove_particles: check free list (stack) of shader idxs, if availe so use that and don't emplace back
					size_t nextId = instancedEffectsData.size();
					instancedEffectsData.emplace_back();
					effect->assignedShaderIndex = nextId;

					shaderToInstanceDataIndex[effect->shader] = nextId;
					findIter = shaderToInstanceDataIndex.find(effect->shader);

					//set up the configured instance data to contain correct attributes and uniforms
					// #suggested perhaps set this init in an "init/ctor" function on EffectInstanceData class
					EffectInstanceData& EIData = instancedEffectsData.back();
					EIData.effectData = effect;

					//below are += to add to built-in passed data (such as model matrix and effect time alive)
					EIData.numMat4PerInstance += effect->numCustomMat4sPerInstance;
					EIData.numVec4PerInstance += effect->numCustomVec4sPerInstance;

					//#TODO #optimize Need system for reserving more data if needed; perhaps a "buffer" class  that wraps vector that can auto-resize based on configuration
					EIData.mat4Data.reserve(EIData.numMat4PerInstance * effect->estimateMaxSimultaneousEffects);
					EIData.vec4Data.reserve(EIData.numVec4PerInstance * effect->estimateMaxSimultaneousEffects);

					EIData.numInstancesThisFrame = 0;

					//this is the first time we're using this effect, precalculate its value dependent state. (tweaking at runtime will require updating again)
					effect->updateEffectDuration();
				}

				//find iter is now definitely pointing to data
				int dataIdx = findIter->second; //#TODO might no longer need this
			}

			//Configure Active Particle
			sp<ActiveParticleGroup> newParticlePtr = new_sp<ActiveParticleGroup>(); //#optimize perhaps pool or write memory manager using large pool of memory for these?
			activeParticles.insert({ newParticlePtr.get(), newParticlePtr });
			ActiveParticleGroup& newParticle = *newParticlePtr;
			newParticle.particle = params.particle;
			newParticle.parentTransform = params.parentTransform;
			newParticle.velocity = params.velocity;
			newParticle.xform = params.xform;
			params.particle->generateMutableEffectData(newParticle.mutableEffectData);
			assert(newParticle.mutableEffectData.size() == params.particle->effects.size());

			//update particle and populate add to effect instance data
			updateActiveParticleGroup(newParticle, 0);

			uint64_t particleId = reinterpret_cast<uint64_t>(newParticlePtr.get());//#TODO may not be great since we're having to do a reinterpret cast, casting back will probably not be safe
			//#TODO return particleID and use that as a method for removal; cast it back for removal. This means the API doesn't allow user to have pointer to particle (wihtout coercion of int); needed for looping particles
		}
	}

	void ParticleSystem::postConstruct()
	{

	}

	void ParticleSystem::tick(float deltaSec)
	{
		using KeyFrameChain = Particle::KeyFrameChain;

		static PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
		const sp<PlayerBase>& player = playerSystem.getPlayer(0);
		const sp<CameraBase> camera = player ? player->getCamera() : sp<CameraBase>(nullptr); //#TODO perhaps just listen to camera changing


		static std::vector<sp<ActiveParticleGroup>> removeParticleContainer; 
		static int oneTimeReserve = [](std::vector<sp<ActiveParticleGroup>> toRemoveContainer) { toRemoveContainer.reserve(100); return 0; }(removeParticleContainer);

		if (currentLevel && camera)
		{
			float dt_sec_world = currentLevel->getWorldTimeManager()->getDeltaTimeSecs();
			
			for (auto& mapIter: activeParticles)
			{
				const sp<ActiveParticleGroup>& activeParticle = mapIter.second;
				if (updateActiveParticleGroup(*activeParticle, dt_sec_world))
				{
					removeParticleContainer.push_back(activeParticle);
				}
			}
		}

		for (sp<ActiveParticleGroup>& particle : removeParticleContainer)
		{
			activeParticles.erase(particle.get());
		}
		removeParticleContainer.clear();
	}

	bool ParticleSystem::updateActiveParticleGroup(ActiveParticleGroup& activeParticle, float dt_sec_world)
	{
		if (activeParticle.velocity)
		{
			activeParticle.xform.position += *activeParticle.velocity * dt_sec_world;
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

			//package vec4 data that is assumed to be available for every effect 
			glm::vec4 builtinvec4;
			builtinvec4.x = activeParticle.timeAlive;
			builtinvec4.y = *effect->effectDuration; 
			//builtinvec4.z = ?;
			//builtinvec4.a = ?;
			effectData.vec4Data.push_back(builtinvec4);

			//#TODO package custom data?
		}

		bool bShouldRemove = false;

		if (bAllEffectsDone)
		{
			sp<ParticleConfig>& particleCFG = activeParticle.particle;
			if (particleCFG->bLoop)
			{
				activeParticle.timeAlive = 0;
				if (particleCFG->numLoops.has_value())
				{
					++activeParticle.bLoopCount;
					if (activeParticle.bLoopCount >= *particleCFG->numLoops)
					{
						bShouldRemove = true;
					}
				}
			}
			else
			{
				bShouldRemove = true;
			}
		}

		return bShouldRemove;
	}

	void ParticleSystem::handlePostRender()
	{
		static PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
		const sp<PlayerBase>& player = playerSystem.getPlayer(0);
		const sp<CameraBase> camera = player ? player->getCamera() : sp<CameraBase>(nullptr); //#TODO perhaps just listen to camera changing

		if (instanceMat4VBO_opt.has_value() && instanceVec4VBO_opt.has_value() && camera)
		{
			const glm::mat4 projection_view = camera->getPerspective() * camera->getView(); //calculate early 
 			const glm::vec3 camPos = camera->getPosition();

			GLuint instanceMat4VBO = *instanceMat4VBO_opt;
			GLuint instanceVec4VBO = *instanceVec4VBO_opt;

			for(EffectInstanceData& eid : instancedEffectsData)
			{
				if (eid.numInstancesThisFrame > 0)
				{
					//at least 16 attributes to use for vertices. (see glGet documentation). query GL_MAX_VERTEX_ATTRIBS
					//assumed vertex attributes:
					// attribute 0 = pos
					// attribute 1 = norm
					// attribute 2 = uv
					// attribute 3 = tangent
					// attribute 4 = bitangent //can potentially remove by using bitangent = normal cross tangent
					// attribute 5-7 = reserved
					// attribute 7 = built-in vec4 where x=effectTimeAlive, y=reserved, z=reserved, w=reserved
					// attribute 8-11 = built-in model matrix
					// attribute 12-15 //last remaining attributes
				
					GLuint effectVAO = eid.effectData->mesh->getVAO();
					ec(glBindVertexArray(effectVAO));

					{ //set up mat4 buffer

						//#TODO check sizes of EID data before trying to bind optional buffers
						//#TODO investigate using glBufferSubData and glMapBuffer and GL_DYNAMIC_DRAW here; may be more efficient if we're not reallocating each time? It appears glBufferData will re-allocate
						ec(glBindBuffer(GL_ARRAY_BUFFER, instanceMat4VBO));
						ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * eid.mat4Data.size(), &eid.mat4Data[0], GL_STATIC_DRAW));
						GLsizei numVec4AttribsInBuffer = 4 * eid.numMat4PerInstance;
						size_t packagedVec4Idx_matbuffer = 0;

						//pass built-in data into instanced array vertex attribute
						{
							//mat4 (these take 4 separate vec4s)
							{
								//model matrix
								ec(glEnableVertexAttribArray(8));
								ec(glEnableVertexAttribArray(9));
								ec(glEnableVertexAttribArray(10));
								ec(glEnableVertexAttribArray(11));

								ec(glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
								ec(glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
								ec(glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
								ec(glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));

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
					}

					{ //set up vec4 buffer
						//#TODO check sizes of EID data before trying to bind optional buffers
						//#TODO investigate using glBufferSubData and glMapBuffer and GL_DYNAMIC_DRAW here; may be more efficient if we're not reallocating each time? It appears glBufferData will re-allocate
						ec(glBindBuffer(GL_ARRAY_BUFFER, instanceVec4VBO));
						ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * eid.vec4Data.size(), &eid.vec4Data[0], GL_STATIC_DRAW));

						//#TODO set num vec4s in stride based on custom data
						GLsizei numVec4AttribsInBuffer = eid.numVec4PerInstance; 

						size_t packagedVec4Idx_v4buffer = 0;
						{
							//package built-in vec4s
							ec(glEnableVertexAttribArray(7));
							ec(glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_v4buffer++ * sizeof(glm::vec4))));
							ec(glVertexAttribDivisor(7, 1));
						}

						//#TODO max instance variables check
						//pass custom data into instanced array
						{

						}
					}

					//activate shader
					sp<Shader>& shader = eid.effectData->shader;
					shader->use();

					//supply uniforms (important that we use uniforms for shared data and not attributes as we do not want run out of attribute space)
					shader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(projection_view));
					shader->setUniform3f("camPos", camPos.x, camPos.y, camPos.z);

					uint32_t mat_glTextureSlot = GL_TEXTURE0;
					for (Particle::Material& mat : eid.effectData->materials)
					{
						ec(glActiveTexture(mat_glTextureSlot));
						glBindTexture(GL_TEXTURE_2D, mat.textureId);
						shader->setUniform1i(mat.sampler2D_name.c_str(), (mat_glTextureSlot - GL_TEXTURE0));

						//#future set material optionals here

						++mat_glTextureSlot;
					}
					
					//instanced render
					eid.effectData->mesh->instanceRender(eid.numInstancesThisFrame);
					eid.clearFrameData();
				}
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

			ec(glDeleteBuffers(1, &(*instanceVec4VBO_opt)));
			instanceVec4VBO_opt.reset();
		}
	}

	void ParticleSystem::handleAcquiredOpenglContext(const sp<Window>& window)
	{
		if (!instanceMat4VBO_opt.has_value())
		{
			unsigned int mat4InstanceVBO;
			ec(glGenBuffers(1, &mat4InstanceVBO));
			ec(glBindBuffer(GL_ARRAY_BUFFER, mat4InstanceVBO));
			instanceMat4VBO_opt = mat4InstanceVBO;

			unsigned int vec4InstanceVBO;
			ec(glGenBuffers(1, &vec4InstanceVBO));
			ec(glBindBuffer(GL_ARRAY_BUFFER, vec4InstanceVBO));
			instanceVec4VBO_opt = vec4InstanceVBO;

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
				layout (location = 7) in vec3 effectData1; //x=timeAlive
				layout (location = 8) in mat4 model; //consumes attribute locations 8,9,10,11
					//layout (location = 12) in vec3 reserved;
					//layout (location = 13) in vec3 reserved;
					//layout (location = 14) in vec3 reserved;
					//layout (location = 15) in vec3 reserved;

				uniform mat4 projection_view;
				uniform vec3 camPos;

				out vec2 uvCoords;
				out float timeAlive;
				out float fractionComplete;

				void main(){
					gl_Position = projection_view * model * vec4(position, 1.0f);

					timeAlive = effectData1.x;
					float effectEndTime = effectData1.y;
					fractionComplete = timeAlive / effectEndTime;
					
					uvCoords = uv;
				}
			)";

	static char const* const SimpleExplosionFS_src = R"(
				#version 330 core
				out vec4 fragColor;

				in vec2 uvCoords;
				in float timeAlive;
				in float fractionComplete;

				uniform vec3 color = vec3(1.f, 0.f, 1.f);
				uniform vec3 camPos;

				uniform sampler2D tessellateTex;

				void main(){
					//----------------------------------------------
					//render as a base color
					//fragColor = vec4(color, 1.f);

					//----------------------------------------------
					//debug render time variables
					//fragColor = vec4(vec3(1.f - fractionComplete), 1.f);
					//fragColor = vec4(vec3(fractionComplete), 1.f);

					//----------------------------------------------
					vec2 baseEffectUV = uvCoords * vec2(5,5);

					//at this point, rgb should all be matching values
					vec4 whiteBaseTexture = texture(tessellateTex, baseEffectUV);

					if(whiteBaseTexture.r < fractionComplete)
					{
						discard;
					}	

					fragColor = whiteBaseTexture;
				}
			)";


	sp<ParticleConfig> ParticleFactory::getSimpleExplosionEffect()
	{
		//#TODO define a shader for each effect
		//#TODO define number of mat4s expected

		static sp<SphereMeshTextured> sphereMesh = new_sp<SphereMeshTextured>();
		static sp<ParticleConfig> sphereEffect = []() -> sp<ParticleConfig> 
		{
			using glm::vec3; using glm::vec4; using glm::mat4;

			//set up textures
			GLuint tessellatedExplosionTextureId = 0;
			if (!GameBase::get().getAssetSystem().loadTexture("GameData/engine_assets/TessellatedShapeRadials.png", tessellatedExplosionTextureId))
			{
				log("ParticleFactory", LogLevel::LOG_ERROR, "Failed to initialize particle texture!");
			}

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
					{
						sphereEffect->materials.emplace_back(
							tessellatedExplosionTextureId,
							"tessellateTex"
						);
						//set optional parameters
						Particle::Material& material = sphereEffect->materials.back();
					}

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

	void Particle::Effect::updateEffectDuration()
	{
		float effectTime = 0;
		for (Particle::KeyFrameChain& KFC : keyFrameChains)
		{
			float largestChainTime = 0;

			//float chains
			float typedChainTime = 0;
			for (Particle::KeyFrame<float> floatFrame : KFC.floatKeyFrames) { typedChainTime += floatFrame.durationSec; }
			largestChainTime = largestChainTime < typedChainTime ? typedChainTime : largestChainTime;

			//vec3 chains
			typedChainTime = 0;
			for (Particle::KeyFrame<glm::vec3> vecFrame : KFC.vec3KeyFrames) { typedChainTime += vecFrame.durationSec; }
			largestChainTime = largestChainTime < typedChainTime ? typedChainTime : largestChainTime;

			//vec4 chains
			typedChainTime = 0;
			for (Particle::KeyFrame<glm::vec4> vecFrame : KFC.vec4KeyFrames) { typedChainTime += vecFrame.durationSec; }
			largestChainTime = largestChainTime < typedChainTime ? typedChainTime : largestChainTime;

			//mat4 chains
			typedChainTime = 0;
			for (Particle::KeyFrame<glm::mat4> matFrame : KFC.mat4KeyFrames) { typedChainTime += matFrame.durationSec; }
			largestChainTime = largestChainTime < typedChainTime ? typedChainTime : largestChainTime;

			//update effect time
			effectTime = effectTime < largestChainTime ? largestChainTime : effectTime;
		}

		effectDuration = effectTime;
	}

}

