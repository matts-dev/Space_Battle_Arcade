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

	void StarField::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		if (hasAcquiredResources())
		{
			ec(glClear(GL_DEPTH_BUFFER_BIT));

			starShader->use();

			sj.tick(dt_sec);

			mat4 customView = view;

			static PlayerSystem& playerSys = GameBase::get().getPlayerSystem();
			const sp<CameraBase>& camera = playerSys.getPlayer(0)->getCamera();
			if (camera)
			{
				vec3 camDir_n = camera->getFront();
				if (bForceCentered)
				{
					vec3 origin(0.f);
					customView = glm::lookAt(origin, origin + camDir_n, camera->getUp());
				}

				bool bStarJumpInProgress = sj.bStarJump || (!sj.bStarJump && sj.starJumpPerc != 0.f);
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

			starMesh->instanceRender(stars.xforms.size());

			ec(glClear(GL_DEPTH_BUFFER_BIT));
		}
	}

	void StarField::postConstruct()
	{
		starShader = new_sp<Shader>(starFieldShader_vs, starFieldShader_fs, false);

		timerDelegate = new_sp<MultiDelegate<>>();
		timerDelegate->addWeakObj(sp_this(), &StarField::handleAcquireInstanceVBOOnNextTick);

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
	}

}

