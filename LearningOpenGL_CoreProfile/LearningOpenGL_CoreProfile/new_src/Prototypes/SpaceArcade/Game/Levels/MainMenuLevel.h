#pragma once
#include "SASpaceLevelBase.h"
#include "../../Tools/DataStructures/SATransform.h"

namespace SA
{
	struct GameUIRenderData;
	class Planet;


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

	private: //logical state
		bool bRenderDebugText = true;
		sp<class DigitalClockFont> debugText = nullptr;
		sp<class QuaternionCamera> menuCamera = nullptr;
		//std::vector<MainMenuScreenData> screensData;

		sp<Planet> mainScreen_planet1 = nullptr;
		sp<Planet> mainScreen_planet2_far = nullptr;

		std::vector<class Widget3D_MenuScreenBase*> menuScreens;

		sp<class Widget3D_GameMainMenuScreen> mainMenuScreen;
	};

}