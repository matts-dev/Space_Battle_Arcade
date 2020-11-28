#pragma once

#include "Widget3D_MenuScreenBase.h"
#include "../Widget3D_DiscreteSelector.h"

namespace SA
{
	class Widget3D_DiscreteSelectorBase;
	class Widget3D_Slider;
	class SettingsProfileConfig;

	class Widget3D_SettingsScreen : public Widget3D_MenuScreenBase
	{
		using Parent = Widget3D_MenuScreenBase;
	public:
		GAMEMENUSCREENBASE_EXPOSE_CLICK_DELEGATE(getBackButton, backButton);
	public:
		virtual void tick(float dt_sec) override;
	protected:
		virtual void postConstruct() override;
		virtual void onActivationChanged(bool bActive) override;
		virtual void renderGameUI(GameUIRenderData& ui_rd) override;
	private:
		void handleDevConsoleChanged(const size_t& newValue);
		void layoutSettings();
		void applySettings();
		void readPlayerSettings();
	private:
		float sliderWidths = 7.f;
		size_t selectedPlayer = 0; 
		size_t selectedSettingsProfile = 0;
		sp<SettingsProfileConfig> activeSettingsProfile;
	private:
		sp<Widget3D_LaserButton> backButton = nullptr;
		sp<Widget3D_LaserButton> applyButton = nullptr;

		sp<Widget3D_DiscreteSelector<size_t>> selector_devConsole = nullptr;
		sp<Widget3D_DiscreteSelector<size_t>> selector_team = nullptr;
		sp<Widget3D_Slider> slider_masterAudio = nullptr;
		sp<Widget3D_Slider> slider_perfRespawnCooldownMultiplier = nullptr;
		sp<Widget3D_Slider> slider_perfRespawnMaxCount = nullptr;

		std::vector<Widget3D_DiscreteSelectorBase*> allSelectors;
		std::vector<Widget3D_Slider*> allSliders;
		std::vector<Widget3D_ActivatableBase*> ordered_options;
	};
}