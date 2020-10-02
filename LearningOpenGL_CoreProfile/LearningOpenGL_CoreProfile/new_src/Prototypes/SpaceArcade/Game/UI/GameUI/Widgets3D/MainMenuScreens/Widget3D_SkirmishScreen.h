#pragma once
#include "Widget3D_MenuScreenBase.h"
#include "../Widget3D_DiscreteSelector.h"
#include <string>

namespace SA
{
	class SpaceLevelConfig;
	class GlitchTextFont;

	class Widget3D_SkirmishScreen : public Widget3D_MenuScreenBase
	{
		using Parent = Widget3D_MenuScreenBase;
	public:
		GAMEMENUSCREENBASE_EXPOSE_CLICK_DELEGATE(getBackButton, backButton);
	public:
		virtual void tick(float dt_sec) override;
	protected:
		virtual void postConstruct() override;
		virtual void onActivationChanged(bool bActive) override;
		void renderGameUI(GameUIRenderData& ui_rd);
	private:
		void configurSelectorPositions(GameUIRenderData& ui_rd);
		void handleStartClicked();
		void handleLevelTransitionAnimationOver();
		void setError(const std::string& errorStr);
	private:
		sp<SpaceLevelConfig> cachedLevelConfig = nullptr;
		sp<MultiDelegate<>> levelAnimTransitionTimerHandle = nullptr;
		sp<GlitchTextFont> errorText = nullptr;
	private:
		sp<Widget3D_LaserButton> backButton = nullptr;
		sp<Widget3D_LaserButton> startButton = nullptr;
		bool bTransitioningToLevel = false;
		float transitionToLevelAnimationDurationSec = 6.f;

		sp<Widget3D_DiscreteSelector<size_t>> selector_numPlanets = nullptr;
		sp<Widget3D_DiscreteSelector<size_t>> selector_numStars = nullptr;
		sp<Widget3D_DiscreteSelector<size_t>> selector_numCarriersPerTeam = nullptr;
		sp<Widget3D_DiscreteSelector<size_t>> selector_numFightersPerCarrier = nullptr;
		sp<Widget3D_DiscreteSelector<size_t>> selector_fighterPercSpawnedAtStart = nullptr;
		std::vector<Widget3D_DiscreteSelectorBase*> allSelectors;
	};

}