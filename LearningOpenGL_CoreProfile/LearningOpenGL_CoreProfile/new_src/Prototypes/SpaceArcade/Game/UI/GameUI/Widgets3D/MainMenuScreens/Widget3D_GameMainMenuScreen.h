#pragma once
#include "Widget3D_MenuScreenBase.h"
#include "../Widget3D_LaserButton.h"
#include "../../../../../GameFramework/CurveSystem.h"

namespace SA
{
	//class Widget3D_LaserButton; //exposing click delegate

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The in game (not editor) main menu screen. 
	//	  This is the entry point of the game.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_GameMainMenuScreen : public Widget3D_MenuScreenBase
	{
		using Parent = Widget3D_MenuScreenBase;
	public:
		virtual void tick(float dt_sec) override;
		GAMEMENUSCREENBASE_EXPOSE_CLICK_DELEGATE(getCampaignClicked, campaignButton);
		GAMEMENUSCREENBASE_EXPOSE_CLICK_DELEGATE(getSkirmishClicked, skirmishButton);
		GAMEMENUSCREENBASE_EXPOSE_CLICK_DELEGATE(getModsClicked, modsButton);
		GAMEMENUSCREENBASE_EXPOSE_CLICK_DELEGATE(getSettingsClicked, settingsButton);
		GAMEMENUSCREENBASE_EXPOSE_CLICK_DELEGATE(getExitClicked, exitButton);
	protected:
		virtual void postConstruct() override;
		virtual void onActivationChanged(bool bActive) override;
	private:
		virtual void renderGameUI(GameUIRenderData& ui_rd) override;
		//void alignButtonPositionsVertically();
		void tryShowNewButton(float dt_sec);
	private:
		const float buttonSpacing = 1.0f;
	private:
		float animInTime = 0.f;
		std::optional<size_t> animInButtonIdx;
		//float buttonDelay = 0.5f;
		float showAllButtonsAnimDurSec = 2.5f;
		bool bSeenOnce = false;
		Curve_highp sigmoid;
	private:
		sp<Widget3D_LaserButton> campaignButton = nullptr;
		sp<Widget3D_LaserButton> skirmishButton = nullptr;
		sp<Widget3D_LaserButton> modsButton = nullptr;
		sp<Widget3D_LaserButton> settingsButton = nullptr;
		sp<Widget3D_LaserButton> exitButton = nullptr;
	};

}
