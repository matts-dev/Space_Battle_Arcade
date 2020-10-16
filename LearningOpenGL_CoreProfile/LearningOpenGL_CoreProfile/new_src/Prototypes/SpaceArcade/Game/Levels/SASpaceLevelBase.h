#pragma once
#include "../../GameFramework/SALevel.h"
#include "../Environment/StarJumpData.h"


#include "../Environment/Planet.h" //included for init data... probably should be refactored so we can forward declare

namespace SA
{
	class ProjectileSystem;
	class TeamCommander;
	class StarField;
	class Star;
	//class Planet; //included for now so that we can functionify the init data part that takes Planet::Data (For space level editor)
	//struct Planet::Data;
	class SpaceLevelConfig;
	class ServerGameMode_SpaceBase;
	class RNG;
	struct PlanetData;
	struct EndGameParameters;

	std::vector<sp<class Planet>> makeRandomizedPlanetArray(RNG& rng);

	constexpr bool bShouldUseDebugLevel = true;

	////////////////////////////////////////////////////////
	// The base class for space levels
	////////////////////////////////////////////////////////
	class SpaceLevelBase : public LevelBase
	{
	public:
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;
		TeamCommander* getTeamCommander(size_t teamIdx);
		virtual void setConfig(const sp<const SpaceLevelConfig>& config);
		const sp<const SpaceLevelConfig>& getConfig() const { return levelConfig; }
		const sp<RNG>& getGenerationRNG() { return generationRNG; } //non-const as user is likely about to modify state of RNG
		bool hasLevelConfig() { return levelConfig != nullptr; }
		virtual bool isTestLevel() { return false; }
		ServerGameMode_SpaceBase* getServerGameMode_SpaceBase();
		const sp<StarField>& getStarField() { return starField; }
		void endGame(const EndGameParameters& endParameters);
		void enableStarJump(bool bEnable, bool bSkipTransition = false);
		static void staticEnableStarJump(bool bEnable, bool bSkipTransition = false);
		bool isStarJumping() const;
	protected:
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;
		virtual void postConstruct() override;
		virtual void tick_v(float dt_sec) override;
		virtual sp<ServerGameMode_Base> onServerCreateGameMode() override;
		virtual void onEntitySpawned_v(const sp<WorldEntity>& spawned) override;
		void handleEntityDestroyed(const sp<GameEntity>& entity);
	protected:
		//#TODO this will need to be read from a saved config file or something instead. Same for local stars.
		virtual void onCreateLocalPlanets() {};
		virtual void onCreateLocalStars();
		virtual sp<StarField> onCreateStarField();
		void refreshStarLightMapping();
	protected:
		void copyPlanetDataToInitData(const PlanetData& editorData, Planet::Data& outInitData);
	private:
		virtual void applyLevelConfig();
		void transitionToMainMenu();
	public:
		MultiDelegate<const EndGameParameters&> onGameEnding;
	private:
		sp<MultiDelegate<>> endTransitionTimerDelegate;
	protected:
		//environment
		sp<StarField> starField;
		std::vector<sp<Star>> localStars;
		std::vector<sp<Planet>> planets;
		sp<SA::Shader> forwardShadedModelShader;
		sp<SA::Shader> highlightForwardModelShader;
		std::vector<class RenderModelEntity*> stencilHighlightEntities;
		StarJumpData sj;
	protected:
		sp<ServerGameMode_SpaceBase> spaceGameMode = nullptr;
	private: //implementation helpers
		bool bGeneratingLocalStars = false;
		sp<RNG> generationRNG = nullptr;
	private: //fields
		std::vector<sp<TeamCommander>> commanders;
		sp<const SpaceLevelConfig> levelConfig = nullptr;
	};

	////////////////////////////////////////////////////////
	// The data present on end level call
	////////////////////////////////////////////////////////
	struct EndGameParameters
	{
		std::vector<size_t> winningTeams;
		std::vector<size_t> losingTeams;
		float delayTransitionMainmenuSec = 6.0f;
	};
}