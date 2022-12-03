#pragma once

#include "GameFramework/SAGameEntity.h"
#include "GameFramework/Interfaces/SATickable.h"
#include<cstdint>

namespace SA
{
	enum class UIMenuState : uint8_t
	{
		NO_MENU,
		MAIN_MENU,
		DEV_MENU,
		MOD_MENU,
		SETTINGS_MENU,
	};

	class UIRootWindow : public GameEntity, public Tickable
	{
	public:
		void setUIVisible(bool bVisible) { bInUIMode = bVisible; }
		void toggleUIVisible() { setUIVisible(!bInUIMode); }
		bool getUIVisible() { return bInUIMode; }

		virtual void tick(float dt_sec) override;
	private:
		virtual void postConstruct() override;
		void handleUIFrameStarted();

		void buildMainMenu();
		void buildDevMenu();
		void buildModMenu();
		void buildSettingsMenu();

		bool bInUIMode = false;
		bool bFPSOverlay = false;
		UIMenuState menuState = UIMenuState::MAIN_MENU;
	};
}