#pragma once
#include "..\..\GameFramework\SALevel.h"


namespace SA
{
	class ProjectileSystem;
	class TeamCommander;

	class SpaceLevelBase : public LevelBase
	{

	public:
		SpaceLevelBase();

		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;
		
		TeamCommander* getTeamCommander(size_t teamIdx);

	protected:
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;

	protected:
		sp<SA::Shader> forwardShadedModelShader;
		size_t numTeams = 2;

	private:
		std::vector<sp<TeamCommander>> commanders;
	};
}