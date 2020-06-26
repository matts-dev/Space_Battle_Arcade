#include "LaserUIPool.h"
#include <assert.h>
#include <algorithm>
#include "../../../GameFramework/SAGameBase.h"
#include "../../../GameFramework/SATimeManagementSystem.h"
#include "../../../GameFramework/SALevel.h"
#include "../../../GameFramework/SALevelSystem.h"
#include "../../OptionalCompilationMacros.h"
#include "../../../GameFramework/SARandomNumberGenerationSystem.h"
#include "../../../Rendering/Camera/SACameraBase.h"
#include "../../GameSystems/SAUISystem_Game.h"
#include "../../../Rendering/OpenGLHelpers.h"
#include "../../SpaceArcade.h"
#include "../../../Rendering/RenderData.h"
#include "../../../GameFramework/SAPlayerBase.h"
#include "../../../GameFramework/SAPlayerSystem.h"
#include "../../../Rendering/SAWindow.h"
#include "../../../GameFramework/SAWindowSystem.h"
#include "../../../Rendering/Camera/SAQuaternionCamera.h"
#include "../../../Tools/PlatformUtils.h"

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Laser UI Pool
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SA::LaserUIPool& LaserUIPool::get()
	{
		static sp<LaserUIPool> sharedPool = new_sp<LaserUIPool>();
		return *sharedPool;
	}

	/*static*/ SA::Curve_highp LaserUIPool::laserLerpCurve;
	/*static */glm::vec3 LaserUIPool::defaultColor = glm::vec3(1, 0, 0);

	LaserUIPool::~LaserUIPool()
	{
		log(__FUNCTION__, LogLevel::LOG, "releasing laser pool memory");
	}

	sp<SA::LaserUIObject> LaserUIPool::requestLaserObject()
	{
		sp<LaserUIObject> ret = nullptr;

		if (unclaimedPool.size() > 0)
		{
			ret = unclaimedPool.back();
			unclaimedPool.pop_back();
		}

		if (!ret)
		{
			//couldn't find one from the pool, so we need to make one
			ret = new_sp<LaserUIObject>();

			//place laser off screen so when it is used it will interpolate onto the screen.
			assignRandomOffscreenMode(*ret);
			ret->forceAnimComplete(); //skip to the offscreen points, offscreen mode will be cleared
			ownedLaserObjects.push_back(ret);
		}

		//clear any offscreen mode as user is requesting this object for direct manipulation
		ret->setOffscreenMode(std::nullopt);
		assert(ret);

		return ret;
	}

	static const char* const laserShader_vs = R"(
				#version 330 core

				layout (location = 0) in vec4 basis_vector;
				layout (location = 1) in mat4 shearMat;			//consumes locations 1, 2, 3, 4
				out vec4 color;

				uniform mat4 projection_view;

				void main()
				{
					gl_Position = projection_view * shearMat * basis_vector; //notice: debug line shader passes 4th quad as 1, but seems like this should be 0
					color = shearMat[2]; //3rd col is packed color
				}
			)";
	static const char* const laserShader_fs = R"(
				#version 330 core

				in vec4 color;
				out vec4 fragColor;

				void main()
				{
					fragColor = color;
				}
			)";


	void LaserUIPool::releaseLaser(sp<LaserUIObject>& out)
	{
		if (GameBase::isEngineShutdown())
		{
			//there exists a race condition between static-deinitialization, laser pool is dtored before game which causes things to be released after laser pool is destroyed.
			//if we're shut down, then just ignore all releases.
			return;
		}


		if (out)
		{
			unclaimedPool.push_back(out);

	#ifdef DEBUG_BUILD
			bool bFoundOwned = std::find(ownedLaserObjects.begin(), ownedLaserObjects.end(), out) != ownedLaserObjects.end();
			assert(bFoundOwned);
	#endif
			assignRandomOffscreenMode(*out);

			//clear out the pointer freeing it for the user of the laser object
			out = nullptr;
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_WARNING, "Passed null laser object to add to laser pool");
		}
	}

	void LaserUIPool::assignRandomOffscreenMode(LaserUIObject& laser)
	{		
		//set up data so that laser will interpolate off the screen.
		if (!laser.offscreenMode.has_value())
		{
			static size_t idx = 0;
			static std::array<ELaserOffscreenMode, 4> modes =
			{ //specifying these 4 so that iterating over the enum will not cause issues if new custom offscreen mode is created
				ELaserOffscreenMode::TOP,
				ELaserOffscreenMode::LEFT,
				ELaserOffscreenMode::BOTTOM,
				ELaserOffscreenMode::RIGHT,
			};

			//rotate in circle so that we evenly distribute released lasers
			ELaserOffscreenMode offMode = modes[idx];
			idx = (idx + 1) % modes.size(); //wrap around index

			laser.setOffscreenMode(offMode);
		}
	}

	void LaserUIPool::postConstruct()
	{
		GPUResource::postConstruct();

		SpaceArcade& game = SpaceArcade::get();

		//use system timer so that this works regardless of level transition
		game.getSystemTimeManager().registerTicker(sp_this());
		rng = game.getRNGSystem().getNamedRNG(LaserRNGKey);

		laserLerpCurve = game.getCurveSystem().generateSigmoid_medp(20.f);

		const sp<UISystem_Game>& gameUISystem = game.getGameUISystem();
		gameUISystem->onUIGameRender.addStrongObj(sp_this(), &LaserUIPool::renderGameUI);
		game.onShutdownInitiated.addWeakObj(sp_this(), &LaserUIPool::handleGameShutdownStarted);

		game.getWindowSystem().onPrimaryWindowChangingEvent.addWeakObj(sp_this(), &LaserUIPool::handlePrimaryWindowChanging);
		if (const sp<Window>& primaryWindow = game.getWindowSystem().getPrimaryWindow())
		{
			handlePrimaryWindowChanging(nullptr, primaryWindow);
		}

		GameBase::get().getLevelSystem().onPreLevelChange.addWeakObj(sp_this(), &LaserUIPool::handlePreLevelChange);

		laserShader = new_sp<Shader>(laserShader_vs, laserShader_fs, false);
	}

	void LaserUIPool::handleGameShutdownStarted()
	{
		SpaceArcade& game = SpaceArcade::get();

		//clean up delegates (probably not actually necessary but, juts being tidy and adding code in case this is refactored to exist somewhere else).
		const sp<UISystem_Game>& gameUISystem = game.getGameUISystem();
		gameUISystem->onUIGameRender.removeStrong(sp_this(), &LaserUIPool::renderGameUI);
	}

	void LaserUIPool::handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window)
	{
		if (old_window)
		{
			old_window->framebufferSizeChanged.removeWeak(sp_this(), &LaserUIPool::handleFramebufferResized);
		}

		if (new_window)
		{
			new_window->framebufferSizeChanged.addWeakObj(sp_this(), &LaserUIPool::handleFramebufferResized);
			std::pair<int, int> framebufferSize = new_window->getFramebufferSize();
			handleFramebufferResized(framebufferSize.first, framebufferSize.second); //this may not be safe if window isn't initialized, but probably should be
		}
	}

	void LaserUIPool::handleFramebufferResized(int width, int height)
	{
		GameUIRenderData ui_rd = {}; //made so that resources are efficiently shared between all instances.

		//this process may should wait until next frame because this is coming from GLFW code.
		for (const sp<LaserUIObject>& laser : ownedLaserObjects)
		{
			//force recalculation using new framebuffer size metrics
			laser->setOffscreenMode(laser->offscreenMode);
		}
	}

	void LaserUIPool::handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel)
	{
		//clear all lasers as we're changing levels (holding off on this as it could cause bad bugs if something releases its laser after we've done the clear)

		//TODO in order to do below, iterate over all the laser objects and set a "dead" flag that can be used to discard the laser when it is released instead of attempting to add it back to pool.
		//since we do not ever changed owned lasers, we cannot rely on these "dead" lasers.

		//activeLasers.clear();
		//unclaimedPool.clear();
		//ownedLaserObjects.clear();
	}

	void LaserUIPool::onReleaseGPUResources()
	{
		if (vao)// && hasAcquiredResources())
		{
			ec(glDeleteVertexArrays(1, &vao));
			ec(glDeleteBuffers(1, &vbo_pos));
			ec(glDeleteBuffers(1, &vbo_instance_shearModels));

			vao = 0;
		}
	}

	void LaserUIPool::onAcquireGPUResources()
	{
		if (!vao)
		{
			ec(glGenVertexArrays(1, &vao));
			ec(glBindVertexArray(vao));

			//positions
			ec(glGenBuffers(1, &vbo_pos));
			ec(glBindBuffer(GL_ARRAY_BUFFER, vbo_pos));
			ec(glBufferData(GL_ARRAY_BUFFER, basisVecsCPU.size() * sizeof(decltype(basisVecsCPU)::value_type), basisVecsCPU.data(), GL_STATIC_DRAW));
			ec(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<GLvoid*>(0)));
			glEnableVertexAttribArray(0);

			//generate instance data buffers for optimized instance rendering
			ec(glGenBuffers(1, &vbo_instance_shearModels));
			ec(glBindVertexArray(0));

		}
	}

	bool LaserUIPool::tick(float dt_sec)
	{
		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			//use dilated world time, not system time
			dt_sec = currentLevel->getWorldTimeManager()->getDeltaTimeSecs();

			//for (LaserUIObject* laser : activeLasers)
			//{
			//	laser->tick(dt_sec);
			//}
		}
		return true;
	}

	void LaserUIPool::renderGameUI(GameUIRenderData& ui_rd)
	{
		using namespace glm;

		if (const RenderData* game_rd = ui_rd.renderData())
		{
			instance_ShearMatrices.reserve(instance_ShearMatrices.size() > 1000 ? 1000 : instance_ShearMatrices.size());
			instance_ShearMatrices.clear();

			LaserUIObject::InstanceRenderData dataToInstance;
			for (const sp<LaserUIObject> laser : ownedLaserObjects)
			{
				 laser->prepareRender(ui_rd, dataToInstance);

				 //fast line shear trick -- convert basis vectors to shear matrix columns; see debug line renderer for more information
				 instance_ShearMatrices.push_back(glm::mat4(
					 vec4(dataToInstance.startPnt, 0),		//column1
					 vec4(dataToInstance.endPnt, 0),		//column2	
					 vec4(dataToInstance.color, 0),			//column3
					 vec4(0, 0, 0, 1))						//column4
				 );
			}
		
			//buffer shear matrices 
			ec(glBindVertexArray(vao));

			ec(glBindBuffer(GL_ARRAY_BUFFER, vbo_instance_shearModels));
			ec(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * instance_ShearMatrices.size(), instance_ShearMatrices.data(), GL_STATIC_DRAW));

			GLsizei numVec4AttribsInBuffer = 4;
			size_t packagedVec4Idx_matbuffer = 0;

			//load shear packed matrix
			ec(glEnableVertexAttribArray(1));
			ec(glEnableVertexAttribArray(2));
			ec(glEnableVertexAttribArray(3));
			ec(glEnableVertexAttribArray(4));

			ec(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));
			ec(glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, numVec4AttribsInBuffer * sizeof(glm::vec4), reinterpret_cast<void*>(packagedVec4Idx_matbuffer++ * sizeof(glm::vec4))));

			ec(glVertexAttribDivisor(1, 1));
			ec(glVertexAttribDivisor(2, 1));
			ec(glVertexAttribDivisor(3, 1));
			ec(glVertexAttribDivisor(4, 1));

			//render
			laserShader->use();
			laserShader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(game_rd->projection_view));

			ec(glDrawArraysInstanced(GL_LINES, 0, 2, instance_ShearMatrices.size()));
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Laser UI Object
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static void setupCameraOffsets(LaserUIObject::AnimData& out)
	{
		//initialize camera relative vectors in case we can't get a camera.
		out.camToBegin = out.begin;
		out.camToEnd = out.end;

		if (const sp<PlayerBase> player = GameBase::get().getPlayerSystem().getPlayer(0))
		{
			if (const sp<CameraBase>& camera = player->getCamera())
			{
				//create camera-relative vectors so that these can be used with a moving camera (eg returning vectors offscreen)
				glm::vec3 camPos = camera->getPosition();
				out.camToBegin = out.begin - camPos;
				out.camToEnd = out.end - camPos;

				glm::quat inverseQuat = glm::inverse(camera->getQuat()); //take negative is not inverse
				out.camToBegin = inverseQuat * out.camToBegin;
				out.camToEnd = inverseQuat * out.camToEnd;
			}
		}
	}

	static void setupAnimDataHelper(LaserUIObject::AnimData& out, const glm::vec3& current, const glm::vec3& newEnd)
	{
		out.begin = current;
		out.end = newEnd;
		out.curTickTimeSec = 0.f;

		//needs to get active player if using split screen, for now just get player 0
		setupCameraOffsets(out);
	}

	void LaserUIObject::animateStartTo(const glm::vec3& newStartPoint)
	{
		setupAnimDataHelper(anim_Start, startPos, newStartPoint);
	}

	void LaserUIObject::animateEndTo(const glm::vec3& newEndPoint)
	{
		setupAnimDataHelper(anim_End, endPos, newEndPoint);
	}

	void LaserUIObject::updateEnd_NoAnimReset(const glm::vec3& endPoint)
	{
		//just update goal position, this will not reset the curve being applied. Need to manually update the camera offsets
		anim_End.end = endPoint;
		setupCameraOffsets(anim_End);
	}

	void LaserUIObject::updateStart_NoAnimReset(const glm::vec3& startPoint)
	{
		anim_Start.end = startPoint;
		setupCameraOffsets(anim_Start);
	}

	void LaserUIObject::randomizeAnimSpeed(float targetAnimDurationSecs /*= 1.0f*/, float randomDriftSecs /*= 0.25f*/)
	{
		anim_Start.animDuration = targetAnimDurationSecs + getRandomAnimTimeOffset(randomDriftSecs);
		anim_End.animDuration = targetAnimDurationSecs + getRandomAnimTimeOffset(randomDriftSecs);
	}

	void LaserUIObject::setAnimDurations(float startSpeedSec, float endSpeedSec)
	{
		anim_Start.animDuration = startSpeedSec;
		anim_End.animDuration = endSpeedSec;
	}

	void LaserUIObject::scaleAnimSpeeds(float startScale, float endScale)
	{
		anim_Start.animDuration *= startScale;
		anim_End.animDuration *= endScale;
	}

	float LaserUIObject::getRandomAnimTimeOffset(float rangeSecs)
	{
		float halfRange = rangeSecs / 2;;
		return rng->getFloat(-halfRange, halfRange); //technically will not hit upper bound of half range, but that's okay
	}

	void LaserUIObject::forceAnimComplete()
{
		//set times to animation duration, next tick should correctly place position.
		anim_Start.curTickTimeSec = anim_Start.animDuration;
		anim_End.curTickTimeSec = anim_End.animDuration;
		LerpToGoalPositions();
	}

	void LaserUIObject::setColorImmediate(const glm::vec3& color)
	{
		this->color = color;
	}

	void LaserUIObject::postConstruct()
	{
		rng = GameBase::get().getRNGSystem().getNamedRNG(LaserRNGKey);
	}

	bool LaserUIObject::tick(float dt_sec)
	{
		anim_Start.curTickTimeSec += dt_sec;
		anim_End.curTickTimeSec += dt_sec;

		return true;
	}

	void LaserUIObject::LerpToGoalPositions()
	{
		//WARNING: be careful not to broadcast any events here! this is called from render thread

		auto LerpPosViaCurve = [](
			glm::vec3& outPos, const glm::vec3& start, const glm::vec3& end
			, float animDurSec, float curTimeSec)
		{
			float percDone = curTimeSec / animDurSec;
			float lerpAlpha = LaserUIPool::laserLerpCurve.eval_smooth(percDone);

			//may need to clamp this output pos
			outPos = glm::mix(start, end, lerpAlpha); 
		};

		LerpPosViaCurve(startPos, anim_Start.begin, anim_Start.end, anim_Start.animDuration, anim_Start.curTickTimeSec);
		LerpPosViaCurve(endPos, anim_End.begin, anim_End.end, anim_End.animDuration, anim_Start.curTickTimeSec);

	}

	void LaserUIObject::prepareRender(GameUIRenderData& ui_rd, InstanceRenderData& outInstanceData)
	{
		//WARNING: be careful not to broadcast any events here! this should be separate from the game thread tick

		//do additional any last minute additional transformations to the instance data
		if (offscreenMode.has_value())
		{
			if (CameraBase* cam = ui_rd.camera())
			{
				glm::quat camRot = ui_rd.camQuat(); //if not using quaternion camera, this will be a unit quaternion and apply no rotation
				glm::vec3 pos = cam->getPosition();

				//adjust positions so that they are camera relative
				anim_End.end = pos + (camRot * anim_End.camToEnd);
				anim_End.begin = pos + (camRot * anim_End.camToBegin);

				anim_Start.end = pos + (camRot * anim_Start.camToEnd);
				anim_Start.begin = pos + (camRot * anim_Start.camToBegin);
			}
		}

		anim_Start.curTickTimeSec += ui_rd.dt_sec();
		anim_End.curTickTimeSec += ui_rd.dt_sec();

		LerpToGoalPositions();//done here and not in tick so that camera adjustments come after any last minute camera manipulation

		outInstanceData.startPnt = startPos;
		outInstanceData.endPnt = endPos;
		outInstanceData.color = color;
	}



	void LaserUIObject::setOffscreenMode(const std::optional<ELaserOffscreenMode>& inOffscreenMode, bool bResetAnimProgress, GameUIRenderData& ui_rd)
	{
		using namespace glm;

		offscreenMode = inOffscreenMode;

		if (offscreenMode.has_value())
		{
			if (const RenderData* game_rd = ui_rd.renderData())
			{
				const HUDData3D& hud_3d = ui_rd.getHUDData3D();

				const float szOffsetFactor = 1.05f;
				vec3 upOffscreen = (szOffsetFactor*hud_3d.savezoneMax_y)*hud_3d.camUp;
				vec3 rightOffscreen = (szOffsetFactor*hud_3d.savezoneMax_x)*hud_3d.camRight;

				struct Helper {
					vec3 start;
					vec3 end;
				};
				std::optional<Helper> newGoal;

				if (*offscreenMode == ELaserOffscreenMode::BOTTOM)
				{
					newGoal = Helper{};
					vec3 basePnt = hud_3d.camPos + -upOffscreen;
					newGoal->start = basePnt + -rightOffscreen;
					newGoal->end = basePnt + rightOffscreen;
				}
				else if (*offscreenMode == ELaserOffscreenMode::TOP)
				{
					newGoal = Helper{};
					vec3 basePnt = hud_3d.camPos + upOffscreen;
					newGoal->start = basePnt + -rightOffscreen;
					newGoal->end = basePnt + rightOffscreen;
				}
				else if (*offscreenMode == ELaserOffscreenMode::LEFT)
				{
					newGoal = Helper{};
					vec3 basePnt = hud_3d.camPos + -rightOffscreen;
					newGoal->start = basePnt + -upOffscreen;
					newGoal->end = basePnt + upOffscreen;
				}
				else if (*offscreenMode == ELaserOffscreenMode::RIGHT)
				{
					newGoal = Helper{};
					vec3 basePnt = hud_3d.camPos + rightOffscreen;
					newGoal->start = basePnt + -upOffscreen;
					newGoal->end = basePnt + upOffscreen;
				}

				if (newGoal.has_value())
				{
					//safezone calculations are based on a certain distance in front of the camera.
					vec3 frontOffset = (1.01f)*hud_3d.frontOffsetDist* hud_3d.camFront;
					newGoal->start += frontOffset;
					newGoal->end += frontOffset;

					if (bResetAnimProgress)
					{
						//these calls will handle updating camera
						animateEndTo(newGoal->end);
						animateStartTo(newGoal->start);
					}
					else
					{
						updateEnd_NoAnimReset(newGoal->end);
						updateStart_NoAnimReset(newGoal->start);
					}
				}
			}
		}
	}

	void LaserUIObject::setOffscreenMode(const std::optional<ELaserOffscreenMode>& inOffscreenMode, bool bResetAnimProgress /*= true*/)
	{
		GameUIRenderData ui_rd; 
		setOffscreenMode(inOffscreenMode, bResetAnimProgress, ui_rd);
	}

}

