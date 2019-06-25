#pragma once

#include "../../GameFramework/SAGameEntity.h"
#include "../../GameFramework/Interfaces/SATickable.h"
#include<cstdint>

namespace SA
{
	enum class UIMenuState : uint8_t
	{
		NO_MENU,
		MAIN_MENU,
		DEV_MENU,
		MOD_MENU,
	};

	class UIRootWindow : public GameEntity, public Tickable
	{
	public:
		void setUIVisible(bool bVisible) { bInUIMode = bVisible; }
		void toggleUIVisible() { setUIVisible(!bInUIMode); }
		bool getUIVisible() { return bInUIMode; }

		virtual void tick(float dt_sec) override;
	private:
		virtual void postConstruct();
		void handleUIFrameStarted();

		void buildMainMenu();
		void buildDevMenu();
		void buildModMenu();

		bool bInUIMode = false;
		UIMenuState menuState = UIMenuState::MAIN_MENU;
	};
}