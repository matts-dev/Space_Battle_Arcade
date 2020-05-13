#pragma once
#include "../../GameFramework/SALevel.h"

namespace SA
{
	class ProjectileSystem;
	class TeamCommander;
	class StarField;
	class Star;
	class Planet;

	std::vector<sp<class Planet>> makeRandomizedPlanetArray(class RNG& rng);

	class SpaceLevelBase : public LevelBase
	{

	public:

		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;
		
		TeamCommander* getTeamCommander(size_t teamIdx);

	protected:
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;
		virtual void postConstruct() override;
		virtual void tick_v(float dt_sec) override;

	protected:
		//#TODO this will need to be read from a saved config file or something instead. Same for local stars.
		virtual void onCreateLocalPlanets() {};
		virtual void onCreateLocalStars();
		virtual sp<StarField> onCreateStarField();

		void refreshStarLightMapping();

	protected:
		//environment
		sp<StarField> starField;
		std::vector<sp<Star>> localStars;
		std::vector<sp<Planet>> planets;

		sp<SA::Shader> forwardShadedModelShader;
		size_t numTeams = 2;

	private: //implementation helpers
		bool bGeneratingLocalStars = false;

	private: //fields
		std::vector<sp<TeamCommander>> commanders;
	};
}