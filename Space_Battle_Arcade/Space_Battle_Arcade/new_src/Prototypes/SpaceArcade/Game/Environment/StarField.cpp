#include "StarField.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "../../Tools/Geometry/SimpleShapes.h"
#include "../../Rendering/OpenGLHelpers.h"
#include "../../Rendering/SAShader.h"
#include "../../Rendering/Camera/SACameraBase.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/SARenderSystem.h"
#include "../../Rendering/Camera/Texture_2D.h"
#include <detail/func_common.hpp>
#include "../../Tools/color_utils.h"
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SAAudioSystem.h"
#include "../AssetConfigs/SoundEffectSubConfig.h"
#include "../../GameFramework/SAWorldEntity.h"
#include "../../GameFramework/Interfaces/SAIControllable.h"

namespace SA
{
	using namespace glm;


	static char const* const starFieldShader_vs = R"(
		#version 330 core
		layout (location = 0) in vec3 position;				
		layout (location = 1) in vec3 normal;
		layout (location = 2) in vec2 uv;

		layout (location = 7) in vec3 starColor; 
		layout (location = 8) in mat4 model; //consumes attribute locations 8,9,10,11

		uniform mat4 projection_view;

		uniform mat4 starJump_Displacement = mat4(1.f); //start out as identity matrix
		uniform mat4 starJump_Stretch = mat4(1.f);

		out vec3 color;

		void main(){
			gl_Position = projection_view * starJump_Displacement * model * starJump_Stretch * vec4(position, 1.0f);
			color = starColor;
		}
	)";

	static char const* const starFieldShader_fs = R"(
		#version 330 core
		out vec4 fragColor;

		uniform bool bStarJump = false;
		uniform float starJumpPerc = 0.f; //[0,1]

		in vec3 color;

		void main(){

			vec3 stretchColorMultiplier = vec3(1.f);
			if(bStarJump)
			{
				float maxColorIntensity = 5.f; //#hdr_tweak
				stretchColorMultiplier = vec3(1.f + starJumpPerc*maxColorIntensity);
			}

			fragColor = vec4(stretchColorMultiplier * color, 1.0f);

		}
	)";


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// sprite texture for star jumping final stage for us with a textured quad
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	char const* const spiralShader_vs = R"(
		#version 330 core
		layout (location = 0) in vec3 position;				
		layout (location = 1) in vec2 inTexCoord;		
				
		out vec2 texCoord;

		uniform mat4 model;
		uniform mat4 projection;

		void main(){
			texCoord = inTexCoord;
			gl_Position = projection * model * vec4(position, 1.f);
		}
	)";
	char const* const spiralShader_fs = R"(
		#version 330 core

		out vec4 fragmentColor;

		in vec2 texCoord;

		uniform sampler2D textureData;   
		uniform bool bgMode = false;
		uniform vec3 uColor = vec3(1.f);
		uniform float percComplete = 1.f; //expects [0,1] range

		void main()
		{
			vec3 color = uColor;
			float alphaInfluence = 1.f;

			if(bgMode)
			{
				color *= 2.0f; //#hdr_tweak BG to spiral in star jump
			}
			else
			{
				color *= 3.0f; //#hdr_tweak spiral pattern during end of star jump

				vec2 centered = texCoord - 0.5f;	//convert from [0,1] to [-.5,.5]
				centered = abs(centered);			//[0,.5]
				centered *= 2.f;					//convert [0,1]
				//centered = vec2(1.f) - centered;	//[0,1] flipped
								
				float centerScaleFactor = max(centered.x, centered.y);
				//vec2 centerScaleFactor = centered;
				centerScaleFactor *= 2.f; //change influence of center by some amount
				
				float baseScaleUp = 100.f;
				//vec2 uv = texCoord;
				vec2 uv = centered;
				uv *= baseScaleUp;
				uv *= centerScaleFactor;

				vec4 tex = texture(textureData, uv);
				color *= tex.rgb;

				float discardCenterValue = 0.2f;
				if(tex.r < 0.75
				 //|| centered.x < discardCenterValue || centered.y < discardCenterValue
				)
				{
					discard;
				}

				alphaInfluence = 0.5f; //have texture be alpha blended
			}

			fragmentColor = vec4(color, min(percComplete, alphaInfluence)); 
			//fragmentColor = vec4(color, 1.f);  //debug
		}
	)";


	StarField::StarField(const StarField::InitData& init /*= InitData()*/)
	{
		if (init.colorScheme.has_value())
		{
			colorScheme = *init.colorScheme;
		}
	}

	void StarField::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		if (hasAcquiredResources())
		{
			ec(glClear(GL_DEPTH_BUFFER_BIT));

			starShader->use();

			sj.tick(dt_sec);

			mat4 customView = view;

			bool bStarJumpInProgress = sj.bStarJump || (!sj.bStarJump && sj.starJumpPerc != 0.f);

			static PlayerSystem& playerSys = GameBase::get().getPlayerSystem();
			const sp<CameraBase>& camera = playerSys.getPlayer(0)->getCamera();
			vec3 camDir_n = glm::vec3(0, 0, -1);
			if (camera)
			{
				camDir_n = camera->getFront();
				if (bForceCentered)
				{
					vec3 origin(0.f);
					customView = glm::lookAt(origin, origin + camDir_n, camera->getUp());
				}

				glm::mat4 starJump_Displacement{ 1.f }; //start out as identity matrix
				glm::mat4 starJump_Stretch{ 1.f };
				if (bStarJumpInProgress)
				{
					//camera is always considered at 0,0,0 for star field, so we just need to shift and stretch stars in camera direction
					const float maxStretchFactor = 100.f;
					const float stretchThisFrame = maxStretchFactor * sj.starJumpPerc;
					vec3 stretchVec = stretchThisFrame * -camDir_n;//stretch stars opposite of camera direction (they're coming at you), put star in center of scale up
					vec3 adjustedPosition = 0.5f * stretchVec; 

					//stretch is applied before model
 					starJump_Stretch = starJump_Stretch * glm::toMat4(camera->getQuat()); //rotate
					float stretchScaleUp = 1.f;
					starJump_Stretch = glm::scale(starJump_Stretch, vec3(1.f, 1.f, glm::max(stretchThisFrame * stretchScaleUp, 1.f)));

					//displacement is applied after model
					starJump_Displacement = glm::translate(starJump_Displacement, adjustedPosition);

				}
				starShader->setUniformMatrix4fv("starJump_Displacement", 1, GL_FALSE, glm::value_ptr(starJump_Displacement));
				starShader->setUniformMatrix4fv("starJump_Stretch", 1, GL_FALSE, glm::value_ptr(starJump_Stretch));
				starShader->setUniform1i("bStarJump", int(bStarJumpInProgress)); //if we have disabled star jump, but are still ticking down, then consider this still enabled as we want the stretch to go away. 
				starShader->setUniform1f("starJumpPerc", sj.starJumpPerc);
			}

			mat4 projection_view = projection * customView;


			starShader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(projection_view));

			if (!bStarJumpInProgress)
			{
				starMesh->instanceRender(stars.xforms.size());
			}
			else
			{
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// render star jump vfx
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				//ec(glEnable(GL_BLEND)); //blending is not working as expected, compromising as this already looks okay.
				//ec(glBlendFunc(GL_ONE, GL_ONE)); //set source and destination factors, recall blending is color = source*sourceFactor + destination*destinationFactor;

				//only show stars when not at end of star jump
				float threshold_finalStartStartPerc = 0.99f;
				float finalStagePerc = sj.starJumpPerc - threshold_finalStartStartPerc;
				if (finalStagePerc >= 0.f)
				{
					starMesh->instanceRender(stars.xforms.size());

					float max = 1.f - threshold_finalStartStartPerc;
					finalStagePerc /= max;
					finalStagePerc = glm::clamp(finalStagePerc, 0.f, 1.f);
					//logf_sa(__FUNCTION__, LogLevel::LOG, "perc %f", finalStagePerc);

					float currentTime = sj.totalTime;
					Transform spiralXform;

					vec3 simulatedCamDir_n = vec3(0, 0, -1.f);
					spiralXform.position = 2.f*simulatedCamDir_n;
					//spiralXform.rotQuat = camQuat;
					spiralXform.scale = vec3(10.f);

					spiralShader->use();
					spiralShader->setUniform1i("textureData", 0);
					spiralShader->setUniform1f("percComplete", finalStagePerc);

					spiralShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(spiralXform.getModelMatrix()));
					spiralShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(projection));
					starJumpTexture->bindTexture(GL_TEXTURE0);

					spiralShader->use();
					spiralShader->setUniform1i("bgMode", 1);
					spiralShader->setUniform3f("uColor", vec3(1.f));
					texturedQuad->render();

					spiralShader->use();
					spiralShader->setUniform1i("bgMode", 0);
					spiralXform.position -= 0.1f * simulatedCamDir_n; //make this closer to screen so we can see it
					float rotationRate_radSec = 1.f;
					static float first_rotation_rad = 0.f;
					first_rotation_rad += rotationRate_radSec* dt_sec;
					first_rotation_rad = first_rotation_rad > radians(360.f) ? first_rotation_rad - radians(360.f) : first_rotation_rad;
					spiralXform.rotQuat = glm::angleAxis(first_rotation_rad, simulatedCamDir_n);
					float secondaryColorStr = 0.6f;
					spiralShader->setUniform3f("uColor", vec3(secondaryColorStr, secondaryColorStr,1.f));
					spiralShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(spiralXform.getModelMatrix()));
					texturedQuad->render();

					spiralXform.position -= 0.1f * simulatedCamDir_n; //make this closer to screen so we can see it
					static float second_rotation_rad = 0.f;
					second_rotation_rad += -rotationRate_radSec * dt_sec * 1.1f;
					second_rotation_rad = second_rotation_rad > radians(360.f) ? second_rotation_rad - radians(360.f) : second_rotation_rad;
					spiralXform.rotQuat = glm::angleAxis(second_rotation_rad, simulatedCamDir_n);
					spiralShader->setUniform3f("uColor", vec3(1.f, secondaryColorStr, secondaryColorStr));
					spiralShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(spiralXform.getModelMatrix()));
					texturedQuad->render();

					//ec(glDisable(GL_BLEND));
				}
				else
				{
					starMesh->instanceRender(stars.xforms.size());
				}

				//update sfx for star jump (only when star jump is in progress)
			}
			if (sj.isStarJumpInProgress() || !bStarJumpSfxComplete)
			{
				updateStarJumpSFX();
			}

			ec(glClear(GL_DEPTH_BUFFER_BIT));
		}
	}

	void StarField::postConstruct()
	{
		starShader = new_sp<Shader>(starFieldShader_vs, starFieldShader_fs, false);
		spiralShader = new_sp<Shader>(spiralShader_vs, spiralShader_fs, false);

		texturedQuad = new_sp<TexturedQuad>();

		starJumpTexture = new_sp<Texture_2D>("GameData/engine_assets/TessellatedShapeRadials.png");

		timerDelegate = new_sp<MultiDelegate<>>();
		timerDelegate->addWeakObj(sp_this(), &StarField::handleAcquireInstanceVBOOnNextTick);


		AudioSystem& audioSystem = GameBase::get().getAudioSystem();

		AudioEmitterPriority sfxPriority = AudioEmitterPriority::GAMEPLAY_PLAYER;

		sfx_starJumpWindUp = audioSystem.createEmitter();
		sfx_starJumpWindUp->setPriority(sfxPriority);
		sfx_starJumpWindUp->setGain(3.f);
		SoundEffectSubConfig windUpSfxConfig;
		windUpSfxConfig.assetPath = "Assets/Sounds/engine_ramp_up_fx.wav";
		windUpSfxConfig.configureEmitter(sfx_starJumpWindUp);


		sfx_starJumpWindDown = audioSystem.createEmitter();
		sfx_starJumpWindDown->setPriority(sfxPriority);
		sfx_starJumpWindDown->setGain(3.f);
		SoundEffectSubConfig windDownSfxConfig;
		windDownSfxConfig.assetPath = "Assets/Sounds/engine_ramp_down_fx.wav";
		windDownSfxConfig.configureEmitter(sfx_starJumpWindDown);

		sfx_starJumpBoom = audioSystem.createEmitter();
		sfx_starJumpBoom->setPriority(sfxPriority);
		sfx_starJumpBoom->setGain(3.f);
		SoundEffectSubConfig jumpBoomSfxConfig;
		jumpBoomSfxConfig.assetPath = "Assets/Sounds/space_jump_explosion_fx.wav";
		jumpBoomSfxConfig.configureEmitter(sfx_starJumpBoom);

		//generate star field before GPU resources are acquired
		generateStarField();
		GPUResource::postConstruct();
	}

	void StarField::onReleaseGPUResources()
	{
		ec(glDeleteBuffers(1, &modelVBO));
		ec(glDeleteBuffers(1, &colorVBO));
		bool bDataBuffered = false;
	}

	void StarField::onAcquireGPUResources()
	{
		ec(glGenBuffers(1, &modelVBO));
		ec(glBindBuffer(GL_ARRAY_BUFFER, modelVBO));

		ec(glGenBuffers(1, &colorVBO));
		ec(glBindBuffer(GL_ARRAY_BUFFER, colorVBO));

		//buffer on next tick so there isn't a race condition on acquiring the sphere VAO.
		TimeManager& systemTM = GameBase::get().getSystemTimeManager();
		systemTM.createTimer(timerDelegate, 0.01f); //essentially a "next tick" since we don't have nextTicket yet.
	}

	void StarField::handleAcquireInstanceVBOOnNextTick()
	{
		bufferInstanceData();
	}

	void StarField::bufferInstanceData()
	{
		if (!bGenerated)
		{
			generateStarField();
		}

		assert(starMesh);

		//warning, becareful not to cause a race condition here, we must wait until next tick to acquire VAO if this is called from "handleAcquiredGPUResources" because there is no enforced order on the subscribers of that event.
		uint32_t sphereVAO = starMesh->getVAOs()[0];
		glBindVertexArray(sphereVAO);

		////////////////////////////////////////////////////////
		// MODEL vbo
		////////////////////////////////////////////////////////
		ec(glBindBuffer(GL_ARRAY_BUFFER, modelVBO));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * stars.xforms.size(), &stars.xforms[0], GL_STATIC_DRAW));

		ec(glEnableVertexAttribArray(8));
		ec(glEnableVertexAttribArray(9));
		ec(glEnableVertexAttribArray(10));
		ec(glEnableVertexAttribArray(11));

		//void (glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
		ec(glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), reinterpret_cast<void*>(0* sizeof(glm::vec4))));
		ec(glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), reinterpret_cast<void*>(1* sizeof(glm::vec4))));
		ec(glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), reinterpret_cast<void*>(2* sizeof(glm::vec4))));
		ec(glVertexAttribPointer(11, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), reinterpret_cast<void*>(3 * sizeof(glm::vec4))));

		ec(glVertexAttribDivisor(8, 1));
		ec(glVertexAttribDivisor(9, 1));
		ec(glVertexAttribDivisor(10, 1));
		ec(glVertexAttribDivisor(11, 1));

		/////////////////////////////////////////////////////////////////////////////////////
		// COLOR vbo
		/////////////////////////////////////////////////////////////////////////////////////

		ec(glBindBuffer(GL_ARRAY_BUFFER, colorVBO));
		ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * stars.colors.size(), &stars.colors[0], GL_STATIC_DRAW));

		ec(glEnableVertexAttribArray(7));
		ec(glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 1 * sizeof(glm::vec3), reinterpret_cast<void*>(0 * sizeof(glm::vec3))));
		ec(glVertexAttribDivisor(7, 1));

		bDataBuffered = true;
	}

	void StarField::generateStarField()
	{
		if (bGenerated)
		{
			return;
		}

		static GameBase& game = GameBase::get();
		static RNGSystem& rngSystem = game.getRNGSystem();

		starMesh = starMesh ? starMesh : new_sp<SphereMeshTextured>(0.05f); //tolerance is clamped 0.01 - 0.2

		//these coordinates are not the same as coordinates else where. This will be specially rendered first so that 
		//it doesn't influence depth. These positions are just to creat some sort of 3d field of stars.

		float range = 100; //NOTE: we really should get the camera and configure a custom near and far plane to match this value

		sp<RNG> rng = rngSystem.getSeededRNG(seed);
		for (uint32_t starIdx = 0; starIdx < numStars; ++starIdx)
		{
			vec3 pos{ rng->getFloat(-range, range),
				rng->getFloat(-range, range),
				rng->getFloat(-range, range)
			};

			//don't let any stars close to origin exist. 
			const float minDist = 10.f;
			abs(pos.x) < minDist ? minDist : pos.x;
			abs(pos.y) < minDist ? minDist : pos.y;
			abs(pos.z) < minDist ? minDist : pos.z;

			//use distance to origin to scale down the instanced sphere that represents a star
			float dist = length(pos);
			float scaleDownAtMin = 1 / 140.0f;
			float scale = scaleDownAtMin;
			scale *= (dist / minDist); // roughly make further objects the about same size as close objects

			mat4 model(1.f);
			model = glm::translate(model, pos);
			model = glm::scale(model, vec3(scale));


			size_t colorIdx = rng->getInt(0, 2);
			glm::vec3 color = colorScheme[colorIdx];

			color += vec3(0.60f); //whiten the stars a bit
			color = clamp(color, vec3(0.f), vec3(1.0f));

			//stars are too bright after whitening, dim them down while still having adjusted colors to be more star-like
			color *= 0.5f;

			if (bUseHDR &&  GameBase::get().getRenderSystem().isUsingHDR())
			{
				//scale up color to make them variable HDR colors
				color *= rng->getFloat(1.51f, 3.f);  //@hdr_tweak
			}

			//load instance data
			stars.xforms.push_back(model);
			stars.colors.push_back(color);
		}

		bGenerated = true;
	}

	void StarField::enableStarJump(bool bEnable, bool bSkipTransition /*= false*/)
	{
		sj.enableStarJump(bEnable, bSkipTransition);

		if (!bSkipTransition)
		{
			if (bEnable)
			{
				sfx_starJumpWindUp->play();
				bStarJumpBoomPlayed = false;
			}
			else
			{
				sfx_starJumpBoom->play();
				sfx_starJumpWindDown->play();
			}

			bStarJumpSfxComplete = false;
			updateStarJumpSFX();
		}

	}

	void StarField::updateStarJumpSFX()
	{
		if (const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0))
		{
			if (IControllable* controlTarget = player->getControlTarget())
			{
				if (WorldEntity* ct_worldEntity = controlTarget->asWorldEntity())
				{
					glm::vec3 worldPosition = ct_worldEntity->getWorldPosition();
					sfx_starJumpWindUp->setPosition(worldPosition);
					sfx_starJumpWindDown->setPosition(worldPosition);
					sfx_starJumpBoom->setPosition(worldPosition);

				}
			}
		}
		if (!bStarJumpBoomPlayed && sj.starJumpPerc == 1.f)
		{
			bStarJumpBoomPlayed = true;
			sfx_starJumpBoom->play();
		}

		if (!bStarJumpSfxComplete && 
				( (sj.starJumpPerc == 1.f && sj.bStarJump)
				|| (sj.starJumpPerc == 0.f && !sj.bStarJump)
				)
			)
		{
			bStarJumpSfxComplete = true;
			sfx_starJumpWindUp->stop();
			sfx_starJumpWindDown->stop();
		}
	}

	void StarField::regenerate()
	{
		bGenerated = false;

		bool bGpuResourceAvailable = hasAcquiredResources();

		if(bGpuResourceAvailable) onReleaseGPUResources();
		generateStarField();
		if (bGpuResourceAvailable) onAcquireGPUResources();
	}


}

