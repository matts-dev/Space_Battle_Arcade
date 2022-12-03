#include "EnigmaTutorialAnimationEntity.h"
#include "GameFramework/SALevelSystem.h"
#include "GameFramework/SAGameBase.h"
#include "EnigmaTutorialsLevel.h"
#include "GameFramework/SAPlayerBase.h"
#include "Rendering/Camera/SAQuaternionCamera.h"
#include "GameFramework/SAPlayerSystem.h"
#include "GameFramework/SAWindowSystem.h"
#include "Rendering/SAWindow.h"
#include "Game/UI/GameUI/text/DigitalClockFont.h"
#include "Game/GameSystems/SAUISystem_Game.h"
#include "Game/SpaceArcade.h"
#include "GameFramework/SARenderSystem.h"
#include "Game/UI/GameUI/SAHUD.h"
#include "Game/UI/GameUI/text/GlitchText.h"

namespace SA
{
	void EnigmaTutorialAnimationEntity::postConstruct()
	{
		//if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		//{
		//	if (EnigmaTutorialLevel* ETLevel = dynamic_cast<EnigmaTutorialLevel*>(currentLevel.get()))
		//	{
		//		//get whatever positions needed from level
		//	}
		//}
		//else
		//{
		//	log(__FUNCTION__, LogLevel::LOG_ERROR, "Failed to get current level for enigma tutorial animation");
		//}

		if (const sp<HUD> hud = SpaceArcade::get().getHUD())
		{
			hud->setVisibility(false);
		}

		GameBase::get().getWindowSystem().onPrimaryWindowChangingEvent.addWeakObj(sp_this(), &EnigmaTutorialAnimationEntity::handleWindowChanging);

		//camCurve = GameBase::get().getCurveSystem().generateSigmoid_medp(2.0);
		//camCurve = GameBase::get().getCurveSystem().generateSigmoid_medp(3.0);
		//camCurve = GameBase::get().getCurveSystem().generateSigmoid_medp(10.0);
		camCurve = GameBase::get().getCurveSystem().generateSigmoid_medp(20.0f);		//this feels the best
		//camCurve = GameBase::get().getCurveSystem().generateSigmoid_medp(100.0);
		//camCurve = GameBase::get().getCurveSystem().generateSigmoid_medp(1000.0);

		const sp<Window>& primaryWindow = GameBase::get().getWindowSystem().getPrimaryWindow();
		if(primaryWindow)
		{
			handleWindowChanging(nullptr, primaryWindow);
		}

		if (bRenderDebugText)
		{
			DigitalClockFontInitData debugTextInit;
			debugTextInit.shader = getDefaultGlyphShader_instanceBased();
			debugTextInit.text = "debug";
			debugText = new_sp<DigitalClockFont>(debugTextInit);
		}

		if (const sp<UISystem_Game>& gameUISystem = SpaceArcade::get().getGameUISystem())
		{
			gameUISystem->onUIGameRender.addWeakObj(sp_this(), &EnigmaTutorialAnimationEntity::handleGameUIRenderDispatch);
		}

		DigitalClockFontInitData init;
		init.shader = getGlitchTextShader();
		init.text = "Enigma Tutorials";
		glitchText = new_sp<GlitchTextFont>(init);

		Transform textXform;
		textXform.position = glm::normalize(glm::vec3(0.8, -0.9, 0)) * 25.f;
		glitchText->setXform(textXform);
		glitchText->play(false);
	}

	bool EnigmaTutorialAnimationEntity::tick(float dt_sec)
	{
		using namespace glm;

		glitchText->tick(dt_sec);
		if (cacheCam)
		{
			glm::quat beforeAnimQ = cacheCam->getQuat();
			static bool bPlayDiretionReversed = false;

			if (timePassedSec > animationDurationSec)
			{
				//restart animation
				timePassedSec = 0.f;
				if (EnigmaTutorialLevel* etLevel = dynamic_cast<EnigmaTutorialLevel*>(GameBase::get().getLevelSystem().getCurrentLevel().get()))
				{
					//reset the planet locations
					etLevel->resetData();
					glitchText->setAnimPlayForward(true);
					glitchText->resetAnim();
					glitchText->play(false);
					glitchText->setSpeedScale(1.0f);
					bPlayDiretionReversed = false;
				}
			}
			else
			{
				timePassedSec += dt_sec;
				
				//very addhoc way of implementing this, but each time range does a different thing and is assumed to be discrete steps
				if (timePassedSec <= cameraData.cameraTiltSec)
				{
					float percDone = timePassedSec / cameraData.cameraTiltSec; //[0,1]
					vec3 toStart_n = normalize(cameraData.startPoint - cacheCam->getPosition());
					vec3 toEnd_n = normalize(cameraData.endPoint - cacheCam->getPosition());

					quat roll = glm::angleAxis(glm::radians(-45.f), glm::vec3(0, 0, -1));
					cacheCam->setQuat(roll); //roll before doing interpolation so camera up is correct

					cacheCam->lookAt_v(cameraData.startPoint);
					quat startQ = cacheCam->getQuat();

					cacheCam->lookAt_v(cameraData.endPoint);
					quat endQ = cacheCam->getQuat();

					quat currentRotQ = glm::slerp(startQ, endQ, camCurve.eval_smooth(percDone));
					cacheCam->setQuat(currentRotQ);
				}
				else if (timePassedSec < textAnimData.animSec + animationDurationSec)
				{
					//make sure we finish the interpolation
					cacheCam->lookAt_v(cameraData.endPoint);
					glitchText->play(true);

				}
				//else if (timePassedSec < textDisplayAnimSec + animationDurationSec)
				//{
				//	//do nothing, show text!
				//}
				//else if(timePassedSec < textFadeAnimSec + animationDurationSec)
				//{
				//	if(!bPlayDiretionReversed)
				//	{
				//		glitchText->setSpeedScale(2.0f);
				//		glitchText->setAnimPlayForward(false);
				//		bPlayDiretionReversed = true;
				//	}
				//}

			}


			if (!bRotateCamera)
			{
				//restore to before animation updates, this let animation run without having a lot of branches to respect this bool.
				cacheCam->setQuat(beforeAnimQ);
			}

			Transform newTextXform = glitchText->getXform();
			newTextXform.rotQuat = cacheCam->getQuat();
			glitchText->setXform(newTextXform);

			if (bRenderDebugText)
			{
				Transform debugTextXform;
				debugTextXform.rotQuat = cacheCam->getQuat();
				debugTextXform.position = cacheCam->getPosition() + cacheCam->getFront() * 50.f;

				const glm::vec3 front = cacheCam->getFront();
				char textBuffer[1024];
				snprintf(textBuffer, sizeof(textBuffer), "(%3.1f),(%3.1f),(%3.1f)", front.x, front.y, front.z);
				debugText->setText(textBuffer);
				debugText->setXform(debugTextXform);
			}
		}

		return true;
	}

	void EnigmaTutorialAnimationEntity::render(const struct RenderData& currentFrameRenderData)
	{

	}

	void EnigmaTutorialAnimationEntity::handleWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window)
	{
		const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0);
		if (new_window && player)
		{
			//force player to have quaternion camera for easy animation interpolation
			sp<QuaternionCamera> newCam = new_sp<QuaternionCamera>();
			cacheCam = newCam;
			newCam->registerToWindowCallbacks_v(new_window);
			player->setCamera(newCam);
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Failed to get player or window");
		}
	}

	void EnigmaTutorialAnimationEntity::handleGameUIRenderDispatch(struct GameUIRenderData&)
	{
		if (const RenderData* frd = GameBase::get().getRenderSystem().getFrameRenderData_Read(GameBase::get().getFrameNumber()))
		{
			if (bRenderDebugText)
			{
				debugText->renderGlyphsAsInstanced(*frd);
			}

			//glitch text requires individual draw calls for animation effect
			glitchText->render(*frd);
		}
	}




}











































