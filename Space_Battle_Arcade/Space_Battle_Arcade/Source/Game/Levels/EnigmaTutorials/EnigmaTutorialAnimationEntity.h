#pragma once
#include "GameFramework/Interfaces/SATickable.h"
#include "GameFramework/SAGameEntity.h"
#include "Tools/DataStructures/SATransform.h"
#include "GameFramework/CurveSystem.h"


namespace SA
{
	class Window;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Entity that does enigma tutorial animation
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class EnigmaTutorialAnimationEntity : public GameEntity, public ITickable
	{
	public:
		virtual bool tick(float dt_sec);
		virtual void render(const struct RenderData& currentFrameRenderData);
	protected:
		virtual void postConstruct() override;
	private:
		void handleWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window);
		void handleGameUIRenderDispatch(struct GameUIRenderData&);
	private:
		float animationDurationSec = 20.0f;
		float timePassedSec = 0.f;
		struct
		{
			float cameraTiltSec = 3.0f;	
			glm::vec3 startPoint = glm::vec3(0.7f, 0.7f, 0.f);
			glm::vec3 endPoint = glm::vec3(0.7, -0.65, 0.f); //be careful not to specify two points 180 apart as that will produce nans
		} cameraData;
		struct
		{
			float animSec = 2.0f;
		}textAnimData;
		//float textDisplayAnimSec = 3.0f;
		//float textFadeAnimSec = 2.0f;
	private:
		bool bRotateCamera = true;
		bool bRenderDebugText = false;
		sp<class QuaternionCamera> cacheCam = nullptr;
		sp<class DigitalClockFont> debugText = nullptr;
		sp<class GlitchTextFont> glitchText = nullptr;
		Curve_highp camCurve;
	};

}

































