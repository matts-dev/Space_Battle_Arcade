#pragma once
#include "Widget3D_MenuScreenBase.h"
#include "../../../../../Tools/DataStructures/MultiDelegate.h"

namespace SA 
{
	class CampaignConfig;
	class SpaceLevelConfig;
	class Mod;
	class Planet;
	class Widget3D_GenericMouseHoverable;

	class Widget3D_CampaignScreen : public Widget3D_MenuScreenBase
	{
		using Parent = Widget3D_MenuScreenBase;
	public:
		struct SelectableLevelData
		{
			size_t levelIndexNumberInCampaignConfig = 0; //invalid entries are culled so order is not necessarily the order fond in campaign config; hence it is tracked here
			std::vector<size_t> outGoingLevelPathIndices; 
			glm::vec3 worldPos{ 0.f };
			sp<Planet> uiPlanet = nullptr;
			float uiPlanetScale = 1.f;
			sp<Widget3D_GenericMouseHoverable> hoverWidget = nullptr;
		};
		struct TierData
		{
			bool bAnimStarted = false;
			std::vector<sp<LaserUIObject>> lasers;
			std::vector<size_t> levelIndices;
		};
		~Widget3D_CampaignScreen();
	public:
		GAMEMENUSCREENBASE_EXPOSE_CLICK_DELEGATE(getBackButton, backButton);
	public:
		virtual void tick(float dt_sec) override;
	protected:
		virtual void postConstruct() override;
		virtual void onActivationChanged(bool bActive) override;
		virtual void renderGameUI(GameUIRenderData& ui_rd) override;
	private:
		bool withinTickAfterDeactivationWindow();
		void getCampaign();
		void generateSelectableLevelData();
		void startTierAnimation(TierData& tier);
		std::optional<size_t> findSelectedPlanetIdx();
	private:
		void handleActiveModChanging(const sp<Mod>& previous, const sp<Mod>& active);
		void handlePlanetClicked();
		void handleStartClicked();
		void handleLevelTransitionTimerUp();
	public:
		MultiDelegate<const glm::vec3&> loadingPlanetAtUILocation;
	private://animation data
		bool bHasPlanetSelected = false; //this is currently built to always be true once one has been clicked
		bool bTierAnimationComplete = false;
		const float tierAnimDelaySec = 1.0f;
		float timestamp_activation = 0.f;
		float timestamp_deactivated = 0.f;
		float accumulatedTime = 0.f;
		float hoverCollisionScaleup = 1.5f;
	private:
		bool bLoadingLevel = false;
		float levelTransitionDelaySec = 1.5f;
		sp<MultiDelegate<>> levelTransitionTimerHandle = nullptr;
		sp<SpaceLevelConfig> cachedLevelConfig = nullptr;
	private:
		size_t activeCampaignIdx = 0;
		sp<CampaignConfig> activeCampaign;
		std::vector<TierData> tiers;
		std::vector<SelectableLevelData> linearLevels;
	private: //ui
		sp<Widget3D_LaserButton> backButton = nullptr;
		sp<Widget3D_LaserButton> startButton = nullptr;
	};
}