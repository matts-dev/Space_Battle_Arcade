#pragma once
#include "Game/Levels/SASpaceLevelBase.h"
#include "GameFramework/SAGameEntity.h"

namespace SA
{
	class EnigmaTutorialLevel : public SpaceLevelBase
	{
	public:
		void resetData();
	protected:
		virtual void onCreateLocalPlanets() override;
		virtual void onCreateLocalStars() override;
		virtual void startLevel_v() override;
		virtual bool isMenuLevel() { return true; }
		virtual void tick(float dt_sec) override;
		virtual void render(float dt_sec, const glm::mat4& view, const glm::mat4& projection) override;
	private:
		sp<class EnigmaTutorialAnimationEntity> animation = nullptr;
		glm::quat planetStartQuat = glm::angleAxis(glm::radians(12.f), glm::vec3(0, 1, 0));
	};

}