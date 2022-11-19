#include <assert.h>
#include <stack>
#include "SAAssetSystem.h"
#include "SALevelSystem.h"
#include "SALog.h"
#include "SALevel.h"
#include "SAGameBase.h"
#include "SAParticleSystem.h"
#include "SAPlayerSystem.h"
#include "SAPlayerBase.h"
#include "SAWindowSystem.h"
#include "../Tools/Geometry/SimpleShapes.h"
#include "Tools/SAUtilities.h"
#include "../Rendering/Camera/SACameraBase.h"
#include "Rendering/OpenGLHelpers.h"
#include "../Rendering/DeferredRendering/DeferredRendererStateMachine.h"
#include "SARenderSystem.h"

namespace SA
{
	/////////////////////////////////////////////////////////////////////////////
	// Particle Config Base
	/////////////////////////////////////////////////////////////////////////////
	std::string ParticleConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("Assets/Particles/") + fileName + std::string(".json");
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

	wp<ActiveParticleGroup> ParticleSystem::spawnParticle(const SpawnParams& params)
	{
		wp<ActiveParticleGroup> spawnResult;
#if DISABLE_PARTICLE_SYSTEM
		return spawnResult;
#endif //DISABLE_PARTICLE_SYSTEM

		if (params.particle)
		{
			//validate particle has all necessary data; early out if not
			for (sp<Particle::Effect>& effect : params.particle->effects)
			{
				if (!effect->forwardShader)
				{
					log("Particle System", LogLevel::LOG_ERROR, "Particle spawned with no shader");
					return spawnResult;
				}
			}

			for (sp<Particle::Effect>& effect : params.particle->effects)
			{
				auto findIter = shaderToInstanceDataIndex.find(effect->forwardShader);

				/// generate effectShader-to-dataEntry if one doesn't exist 
				if (findIter == shaderToInstanceDataIndex.end())
				{
					//#future #remove_particles: check free list (stack) of shader idxs, if availe so use that and don't emplace back
					size_t nextId = instancedEffectsData.size();
					instancedEffectsData.emplace_back();
					effect->assignedShaderIndex = nextId;

					shaderToInstanceDataIndex[effect->forwardShader] = nextId;
					findIter = shaderToInstanceDataIndex.find(effect->forwardShader);

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
			newParticle.velocity = params.velocity;
			newParticle.xform = params.xform;
			newParticle.durationDilation = params.durationDilation;
			newParticle.parentXform_m = params.parentXform;
			params.particle->generateMutableEffectData(newParticle.mutableEffectData);
			assert(newParticle.mutableEffectData.size() == params.particle->effects.size());

			//update particle and populate add to effect instance data
			updateActiveParticleGroup(newParticle, 0);

			//#concern perhaps particle alias? user can corrupt data with bad memory access. But user needs to modify transform directly and stop loops.
			spawnResult = newParticlePtr;
			return spawnResult;
		}
		return spawnResult;
	}

	void ParticleSystem::postConstruct()
	{

	}

	void ParticleSystem::tick(float deltaSec)
	{
	}

	bool ParticleSystem::updateActiveParticleGroup(ActiveParticleGroup& activeParticle, float dt_sec_world)
	{
		if (activeParticle.velocity)
		{
			activeParticle.xform.position += *activeParticle.velocity * dt_sec_world;
		}

		glm::mat4 particleGroupModelMat = activeParticle.xform.getModelMatrix();

		if (activeParticle.parentXform_m.has_value())
		{
			particleGroupModelMat = activeParticle.parentXform_m.value() * particleGroupModelMat;
		}

		bool bAllEffectsDone = true;

		for (size_t effectIdx = 0; effectIdx < activeParticle.particle->effects.size(); ++effectIdx)
		{
			sp<Particle::Effect>& effect = activeParticle.particle->effects[effectIdx];
			MutableEffectData& meData = activeParticle.mutableEffectData[effectIdx];

			///Let effects update particle data
			activeParticle.timeAlive += (dt_sec_world * activeParticle.durationDilation);

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
			if (!activeParticle.bAlive)
			{ //user killed particle
				bShouldRemove = true;
			}

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

	void ParticleSystem::handleRenderDispatch(float delta_sec)
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

			for (EffectInstanceData& eid : instancedEffectsData)
			{
				if (eid.numInstancesThisFrame > 0 && eid.effectData->mesh->getVAOs().size() > 0)
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

					// ---- BUFFER data ---- before binding it to all VAOs (model's may have multiple meshes, each with their own VAO)
					//#TODO check sizes of EID data before trying to bind optional buffers
					//#TODO investigate using glBufferSubData and glMapBuffer and GL_DYNAMIC_DRAW here; may be more efficient if we're not reallocating each time? It appears glBufferData will re-allocate
					ec(glBindBuffer(GL_ARRAY_BUFFER, instanceMat4VBO));
					ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * eid.mat4Data.size(), &eid.mat4Data[0], GL_STATIC_DRAW)); //#TODO this needs to go before binding of VAOs

					ec(glBindBuffer(GL_ARRAY_BUFFER, instanceVec4VBO));
					ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * eid.vec4Data.size(), &eid.vec4Data[0], GL_STATIC_DRAW));

					for (GLuint effectVAO : eid.effectData->mesh->getVAOs())
					{
						/*GLuint effectVAO = eid.effectData->mesh->getVAOs();*/
						ec(glBindVertexArray(effectVAO));

						{ //set up mat4 buffer

							ec(glBindBuffer(GL_ARRAY_BUFFER, instanceMat4VBO));
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
							ec(glBindBuffer(GL_ARRAY_BUFFER, instanceVec4VBO));

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
					}

					//activate shader
					sp<Shader>& shader = eid.effectData->forwardShader;
					shader->use();

					//supply uniforms (important that we use uniforms for shared data and not attributes as we do not want run out of attribute space)
					shader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(projection_view));
					shader->setUniform3f("camPos", camPos.x, camPos.y, camPos.z);

					//load material textures and parameters
					uint32_t mat_glTextureSlot = GL_TEXTURE0;
					for (Particle::Material& mat : eid.effectData->materials)
					{
						ec(glActiveTexture(mat_glTextureSlot));
						ec(glBindTexture(GL_TEXTURE_2D, mat.textureId));
						shader->setUniform1i(mat.sampler2D_name.c_str(), (mat_glTextureSlot - GL_TEXTURE0));

						//#future set material optionals here

						++mat_glTextureSlot;
					}

					//load custom uniforms
					for (Particle::UniformData<float>& uniformData : eid.effectData->floatUniforms) { shader->setUniform1f(uniformData.uniformName.c_str(), uniformData.data); }
					for (Particle::UniformData<glm::vec3>& uniformData : eid.effectData->vec3Uniforms) { shader->setUniform3f(uniformData.uniformName.c_str(), uniformData.data); }
					for (Particle::UniformData<glm::vec4>& uniformData : eid.effectData->vec4Uniforms) { shader->setUniform4f(uniformData.uniformName.c_str(), uniformData.data); }
					for (Particle::UniformData<glm::mat4>& uniformData : eid.effectData->mat4Uniforms) { shader->setUniformMatrix4fv(uniformData.uniformName.c_str(), 1, GL_FALSE, glm::value_ptr(uniformData.data)); }

					//instanced render
					eid.effectData->mesh->instanceRender(eid.numInstancesThisFrame);
					eid.clearFrameData();
				}
			}
		}
		ec(glBindVertexArray(0));//unbind VAO's
	}

	void ParticleSystem::handlePostGameloopTick(float deltaSec)
	{
		using KeyFrameChain = Particle::KeyFrameChain;

		static PlayerSystem& playerSystem = GameBase::get().getPlayerSystem();
		const sp<PlayerBase>& player = playerSystem.getPlayer(0);
		const sp<CameraBase> camera = player ? player->getCamera() : sp<CameraBase>(nullptr); //#TODO perhaps just listen to camera changing


		static std::vector<sp<ActiveParticleGroup>> removeParticleContainer;
		static int oneTimeReserve = [](std::vector<sp<ActiveParticleGroup>> toRemoveContainer) { toRemoveContainer.reserve(100); return 0; }(removeParticleContainer);

		if (currentLevel && camera)
		{
			const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager();
			float dt_sec_world = worldTimeManager->isTimeFrozen() ? 0 : worldTimeManager->getDeltaTimeSecs();

			for (auto& mapIter : activeParticles)
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

	void ParticleSystem::initSystem()
	{
		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		//use pre level change as start of level may spawn particles in world, we don't want to clean up those particles.
		levelSystem.onPreLevelChange.addWeakObj(sp_this(), &ParticleSystem::handlePreLevelChange);

		WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		windowSystem.onWindowAcquiredOpenglContext.addWeakObj(sp_this(), &ParticleSystem::handleAcquiredOpenglContext);
		windowSystem.onWindowLosingOpenglContext.addWeakObj(sp_this(), &ParticleSystem::handleLosingOpenglContext);
		if (const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow())
		{
			handleAcquiredOpenglContext(primaryWindow);
		}

		GameBase& game = GameBase::get();
		game.onPostGameloopTick.addStrongObj(sp_this(), &ParticleSystem::handlePostGameloopTick); 
		game.onRenderDispatch.addStrongObj(sp_this(), &ParticleSystem::handleRenderDispatch);
		//game.subscribePostRender(sp_this());
	}

	void ParticleSystem::shutdown()
	{
		GameBase& game = GameBase::get();
		game.onPostGameloopTick.removeStrong(sp_this(), &ParticleSystem::handlePostGameloopTick);
		game.onRenderDispatch.removeStrong(sp_this(), &ParticleSystem::handleRenderDispatch);
	}

	void ParticleSystem::handlePreLevelChange(const sp<LevelBase>& /*previousLevel*/, const sp<LevelBase>& newCurrentLevel)
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
	static char const* const simpleExplosionVS_src = R"(
		#version 330 core
		layout (location = 0) in vec3 position;				
		layout (location = 1) in vec3 normal;
		layout (location = 2) in vec2 uv;
        //layout (location = 3) in vec3 tangent;
        //layout (location = 4) in vec3 bitangent; //may can get 1 more attribute back by calculating this cross(normal, tangent);
			//layout (location = 5) in vec3 reserved;
			//layout (location = 6) in vec3 reserved;
		layout (location = 7) in vec4 effectData1; //x=timeAlive,y=fractionComplete
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

	static char const* const simpleExplosionFS_src = R"(
		#version 330 core
		out vec4 fragColor;

		in vec2 uvCoords;
		in float timeAlive;
		in float fractionComplete;

		uniform vec3 debug_color = vec3(1.f, 0.f, 1.f);
		uniform vec3 camPos;

		uniform vec3 darkColor = vec3(1.f, 0.f, 1.f);
		uniform vec3 medColor = vec3(1.f, 0.f, 1.f);
		uniform vec3 brightColor = vec3(1.f, 0.f, 1.f);
		uniform float hdrFactor = 1.f;

		uniform sampler2D tessellateTex;

		void main(){
			//----------------------------------------------
			//render as a base color
			//fragColor = vec4(debug_color, 1.f);

			//----------------------------------------------
			//debug render time variables
			//fragColor = vec4(vec3(1.f - fractionComplete), 1.f);
			//fragColor = vec4(vec3(fractionComplete), 1.f);

			//----------------------------------------------
			vec2 preventAlignmentOffset = vec2(0.1, 0.2);
			float medMove = 0.5f *fractionComplete;
			float smallMove = 0.5f *fractionComplete;

			vec2 baseEffectUV = uvCoords * vec2(5,5);
			vec2 mediumTessUV_UR = uvCoords * vec2(10,10) + vec2(medMove, medMove) + preventAlignmentOffset;
			vec2 mediumTessUV_UL = uvCoords * vec2(10,10) + vec2(medMove, -medMove);

			vec2 smallTessUV_UR = uvCoords * vec2(20,20) + vec2(smallMove, smallMove) + preventAlignmentOffset;
			vec2 smallTessUV_UL = uvCoords * vec2(20,20) + vec2(smallMove, -smallMove);

			///////////////////////////////////////////////////////////////
			//at this point, rgb should all be matching values (white color)
			///////////////////////////////////////////////////////////////
			vec4 largeTesselations = texture(tessellateTex, baseEffectUV);
			vec4 mediumTesselations = 0.5f * texture(tessellateTex, mediumTessUV_UR)
										+ 0.5f * texture(tessellateTex, mediumTessUV_UL);
			vec4 smallTesselations = 0.5f * texture(tessellateTex, smallTessUV_UR)
									+ 0.5f * texture(tessellateTex, smallTessUV_UL);
			smallTesselations = (smallTesselations * -1) + vec4(vec3(1), 0);			//flip pattern

			///////////////////////////////////////////////////////////////
			// discard fragment logic
			///////////////////////////////////////////////////////////////			

			//easily disable in debugging these by setting to false
			float startDisappearing = 0.5f;										//point at which fading should start in the particles lifetime [0, 1]
			float fadeRegion = 1.f - startDisappearing;
			float fadeThreshold = ((fractionComplete - startDisappearing) / fadeRegion);	//[0, 1]
	
			bool bPassColorThreshold = largeTesselations.r > fadeThreshold;
			if(!bPassColorThreshold)
			{
				discard;
			}

			///////////////////////////////////////////////////////////////
			//colorize white textures
			///////////////////////////////////////////////////////////////
						
			float colorGrowth = clamp(fractionComplete*4, 0, 1);

			fragColor = vec4(0,0,0,1);
			fragColor += (largeTesselations * 1.0) * vec4(darkColor, 0.f);
			fragColor += (mediumTesselations * 1.0f) * vec4(medColor, 0.f);
			fragColor = fragColor *= max(0.1f, colorGrowth);	
		
			fragColor += (smallTesselations * 0.3f) * vec4(brightColor, 0.f);

			fragColor.rgb = fragColor.rgb * hdrFactor;
		}
	)";


	static char const* const simpleExplosion_deferred_vs = R"(
		#version 330 core
		layout (location = 0) in vec3 position;				
		layout (location = 1) in vec3 normal;
		layout (location = 2) in vec2 uv;

        //layout (location = 3) in vec3 tangent;
        //layout (location = 4) in vec3 bitangent; //may can get 1 more attribute back by calculating this cross(normal, tangent);
			//layout (location = 5) in vec3 reserved;
			//layout (location = 6) in vec3 reserved;
		layout (location = 7) in vec4 effectData1; //x=timeAlive,y=fractionComplete
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
		out vec3 fragPosition;

		void main(){
			gl_Position = projection_view * model * vec4(position, 1.0f);

			timeAlive = effectData1.x;
			float effectEndTime = effectData1.y;
			fractionComplete = timeAlive / effectEndTime;
					
			uvCoords = uv;

			fragPosition = vec3(gl_Position);
		}
	)";


	static char const* const simpleExplosion_deferred_fs = R"(
		#version 330 core

		//framebuffer locations 
		layout (location = 0) out vec3 position;
		layout (location = 1) out vec3 normal;
		layout (location = 2) out vec4 albedo_spec;

		in vec2 uvCoords;
		in float timeAlive;
		in float fractionComplete;
		in vec3 fragPosition;

		uniform vec3 debug_color = vec3(1.f, 0.f, 1.f);
		uniform vec3 camPos;

		uniform vec3 darkColor = vec3(1.f, 0.f, 1.f);
		uniform vec3 medColor = vec3(1.f, 0.f, 1.f);
		uniform vec3 brightColor = vec3(1.f, 0.f, 1.f);

		uniform sampler2D tessellateTex;

		void main(){
			//----------------------------------------------
			//render as a base color
			//fragColor = vec4(debug_color, 1.f);

			//----------------------------------------------
			//debug render time variables
			//fragColor = vec4(vec3(1.f - fractionComplete), 1.f);
			//fragColor = vec4(vec3(fractionComplete), 1.f);

			//----------------------------------------------
			vec2 preventAlignmentOffset = vec2(0.1, 0.2);
			float medMove = 0.5f *fractionComplete;
			float smallMove = 0.5f *fractionComplete;

			vec2 baseEffectUV = uvCoords * vec2(5,5);
			vec2 mediumTessUV_UR = uvCoords * vec2(10,10) + vec2(medMove, medMove) + preventAlignmentOffset;
			vec2 mediumTessUV_UL = uvCoords * vec2(10,10) + vec2(medMove, -medMove);

			vec2 smallTessUV_UR = uvCoords * vec2(20,20) + vec2(smallMove, smallMove) + preventAlignmentOffset;
			vec2 smallTessUV_UL = uvCoords * vec2(20,20) + vec2(smallMove, -smallMove);

			///////////////////////////////////////////////////////////////
			//at this point, rgb should all be matching values (white color)
			///////////////////////////////////////////////////////////////
			vec4 largeTesselations = texture(tessellateTex, baseEffectUV);
			vec4 mediumTesselations = 0.5f * texture(tessellateTex, mediumTessUV_UR)
										+ 0.5f * texture(tessellateTex, mediumTessUV_UL);
			vec4 smallTesselations = 0.5f * texture(tessellateTex, smallTessUV_UR)
									+ 0.5f * texture(tessellateTex, smallTessUV_UL);
			smallTesselations = (smallTesselations * -1) + vec4(vec3(1), 0);			//flip pattern

			///////////////////////////////////////////////////////////////
			// discard fragment logic
			///////////////////////////////////////////////////////////////			

			//easily disable in debugging these by setting to false
			float startDisappearing = 0.5f;										//point at which fading should start in the particles lifetime [0, 1]
			float fadeRegion = 1.f - startDisappearing;
			float fadeThreshold = ((fractionComplete - startDisappearing) / fadeRegion);	//[0, 1]
	
			bool bPassColorThreshold = largeTesselations.r > fadeThreshold;
			if(!bPassColorThreshold)
			{
				discard;
			}

			///////////////////////////////////////////////////////////////
			//colorize white textures
			///////////////////////////////////////////////////////////////
						
			float colorGrowth = clamp(fractionComplete*4, 0, 1);

			vec4 fragColor = vec4(0,0,0,1);
			fragColor += (largeTesselations * 1.0) * vec4(darkColor, 0.f);
			fragColor += (mediumTesselations * 1.0f) * vec4(medColor, 0.f);
			fragColor = fragColor *= max(0.1f, colorGrowth);	
			fragColor += (smallTesselations * 0.3f) * vec4(brightColor, 0.f);

			albedo_spec.rgb = fragColor.rgb;
			albedo_spec.a = 0.f;
			normal.rgb = vec3(0.f); //perhaps zero normal will make this immue to lighting contributions?
			position = fragPosition;
		}
	)";


	sp<ParticleConfig> ParticleFactory::getSimpleExplosionEffect()
	{
		//#TODO define a shader for each effect
		//#TODO define number of mat4s expected
#if !IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
		TODO_set_up_deferred_particle_rendering_shaders;
		todo_sheild_particle_to_have_deferred_rendering_shader;
#endif //IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE

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
					sphereEffect->forwardShader = new_sp<Shader>(simpleExplosionVS_src, simpleExplosionFS_src, false);

#if !IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
					sphereEffect->deferredShader = new_sp<Shader>(simpleExplosion_deferred_vs, simpleExplosion_deferred_fs, false);
#endif //IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE

					Particle::KeyFrameChain& scaleEffectChain = sphereEffect->keyFrameChains.back();
					{
						//new effect key frame -- a step in the series
						{
							scaleEffectChain.vec3KeyFrames.emplace_back();
							Particle::KeyFrame<vec3>& scaleFrame = scaleEffectChain.vec3KeyFrames.back();
							scaleFrame.startValue = vec3(0.25f);  
							scaleFrame.endValue = vec3(3); 
							//curve		//-x^(1/4)		//#TODO LUT (look up table) for this curve as exponents would probably be more expensive than LUT cache hit?
							scaleFrame.durationSec = 1.5f;
							scaleFrame.dataIdx = MutableEffectData::SCALE_VEC3_IDX;
						}
						//{
						//	scaleEffectChain.vec3KeyFrames.emplace_back();
						//	Particle::KeyFrame<vec3>& scaleFrame = scaleEffectChain.vec3KeyFrames.back();
						//	scaleFrame.startValue = vec3(1);  
						//	scaleFrame.endValue = vec3(3); 
						//	scaleFrame.durationSec = 2.f;
						//	scaleFrame.dataIdx = MutableEffectData::SCALE_VEC3_IDX;
						//}
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

					sphereEffect->vec3Uniforms.emplace_back("darkColor",	glm::vec3(0xBB / 255.f, 0x5C / 255.f, 0x2B / 255.f));
					sphereEffect->vec3Uniforms.emplace_back("medColor",		glm::vec3(0xEC / 255.f, 0x93 / 255.f, 0x4A / 255.f));
					sphereEffect->vec3Uniforms.emplace_back("brightColor",	glm::vec3(0xF6 / 255.f, 0xAB / 255.f, 0x65 / 255.f));
					sphereEffect->floatUniforms.emplace_back("hdrFactor", GameBase::get().getRenderSystem().isUsingHDR() ? 3.f : 1.f);//@hdr_tweak
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

