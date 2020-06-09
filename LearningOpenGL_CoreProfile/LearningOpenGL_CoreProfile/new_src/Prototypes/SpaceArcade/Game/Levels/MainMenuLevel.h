#pragma once
#include "SASpaceLevelBase.h"
#include "../../Tools/DataStructures/SATransform.h"
#include "../../GameFramework/CurveSystem.h"
#include <optional>

namespace SA
{
	struct GameUIRenderData;
	class Planet;
	class Widget3D_ActivatableBase;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Level representing the main menu. This level allows player to play campaign, skirmishes, etc. It is the
	//		entry point for the game.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class MainMenuLevel : public SpaceLevelBase
	{
		using Parent = SpaceLevelBase;
	protected:
		virtual void postConstruct() override;
		virtual void startLevel_v() override;
		virtual void onCreateLocalPlanets() override;
		virtual void onCreateLocalStars() override;
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;
		virtual void tick_v(float dt_sec) override;
	private:
		void handleGameUIRenderDispatch(GameUIRenderData& uiRenderData);
		void handlePrimaryWindowChanging(const sp<Window>& old_window, const sp<Window>& new_window);
		void setScreensData();
	private:
		const float MainMenuActivationDelaySec = 1.f;
	private://implementation helpers
		sp<MultiDelegate<>> mainMenuStartLevelDelayActiveTimer = new_sp<MultiDelegate<>>();
		void handleMainMenuStartupDelayOver();
	private:
		void updateCamera(float dt_sec);
		void animateCameraTo(glm::vec3 endPoint, float animDuration);
		void setPendingScreenToActivate(Widget3D_ActivatableBase* screen);
		void deactivateAllScreens();
	private: //transitions
		void handleCampaignClicked();
		void handleSkirmishClicked();
		void handleModsClicked();
		void handleSettingsClicked();
		void handleExitClicked();
		void handleReturnToMainMenuClicked();
	private:
		struct CameraAnimData
		{
			float cameraAnimDuration = 3.0f;
			glm::vec3 startPoint = glm::vec3(0.f);
			glm::vec3 endPoint = glm::vec3(0.f); //be careful not to specify two points 180 apart as that will produce nans
			float timePassedSec = 0.f;
			Widget3D_ActivatableBase* pendingScreenToActivate = nullptr;
		};
		std::optional<CameraAnimData> cameraAnimData;
		Curve_highp camCurve;
	private://debug
		bool bRenderDebugText = true;
		bool bFreeCamera = true;
	private: //logical states
		sp<class DigitalClockFont> debugText = nullptr;
		sp<class QuaternionCamera> menuCamera = nullptr;
		//std::vector<MainMenuScreenData> screensData;

		sp<Planet> mainScreen_planet1 = nullptr;
		sp<Planet> mainScreen_planet2_far = nullptr;

		std::vector<class Widget3D_MenuScreenBase*> menuScreens;

		sp<class Widget3D_GameMainMenuScreen> mainMenuScreen;
		sp<class Widget3D_CampaignScreen> campaignScreen;
		sp<class Widget3D_SkirmishScreen> skirmishScreen;
		sp<class Widget3D_SettingsScreen> settingsScreen;
		sp<class Widget3D_ModsScreen> modsScreen;
		sp<class Widget3D_ExitScreen> exitScreen;
		
	};

}