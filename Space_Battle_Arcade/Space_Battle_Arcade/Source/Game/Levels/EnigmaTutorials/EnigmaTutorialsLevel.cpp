#include "EnigmaTutorialsLevel.h"

#include<ctime>

#include "Game/Environment/Planet.h"
#include "../../Environment/Star.h"
#include "../../../GameFramework/SARandomNumberGenerationSystem.h"
#include "GameFramework/SAGameBase.h"
#include "Tools/DataStructures/SATransform.h"
#include "EnigmaTutorialAnimationEntity.h"
#include "../../../GameFramework/SARenderSystem.h"

namespace SA
{

	void EnigmaTutorialLevel::resetData()
	{
		if (planets.size() > 0)
		{
			if (const sp<Planet>& planet = planets.back())
			{
				Transform transform = planet->getTransform();
				transform.rotQuat = planetStartQuat;
				planet->setTransform(transform);
			}
		}
	}

	void EnigmaTutorialLevel::onCreateLocalPlanets()
	{
		constexpr bool generateRandomPlanet = false;
		sp<RNG> rng = GameBase::get().getRNGSystem().getSeededRNG(generateRandomPlanet ? uint32_t(std::time(NULL)) : 13);

		planets.push_back(makeRandomPlanet(*rng));
		const sp<Planet>& myPlanet = planets.back(); 
		Transform plantXform;
		//plantXform.position = glm::vec3(0, -5, 2);
		//plantXform.position *= 5.f;
		//plantXform.position = glm::normalize(glm::vec3(1, 0, 0)) * 10.0f;
		//plantXform.position = glm::normalize(glm::vec3(1, -0.4, 0)) * 10.0f;
		plantXform.position = glm::normalize(glm::vec3(1, -0.6, 0)) * 7.0f;
		myPlanet->setTransform(plantXform);
	}

	void EnigmaTutorialLevel::onCreateLocalStars()
	{
		SpaceLevelBase::onCreateLocalStars();

		const sp<Star>& star = localStars.back();
		SA::Transform newStarXform;
		newStarXform.position = glm::vec3(50, 50, -25);
		star->setXform(newStarXform);

	}

	void EnigmaTutorialLevel::startLevel_v()
	{
		SpaceLevelBase::startLevel_v();

		animation = new_sp<EnigmaTutorialAnimationEntity>();
	}


	void EnigmaTutorialLevel::tick(float dt_sec)
	{
		using namespace glm;

		//SpaceLevelBase::tick(dt_sec); 
		animation->tick(dt_sec);

		//spin the planet
		if (const sp<Planet>& planet = planets.back())
		{
			Transform xform = planet->getTransform();
			/*xform.rotQuat = glm::angleAxis(glm::radians(12.0f * dt_sec), glm::vec3(0, 1, 0)) * xform.rotQuat;*/
			xform.rotQuat = glm::angleAxis(glm::radians(-6.0f * dt_sec), normalize(vec3(0, 0, 1))) * xform.rotQuat;
			planet->setTransform(xform);
		}
	}

	void EnigmaTutorialLevel::render(float dt_sec, const glm::mat4& view, const glm::mat4& projection)
	{
		SpaceLevelBase::render(dt_sec, view, projection);
		if (const RenderData* FRD = GameBase::get().getRenderSystem().getFrameRenderData_Read(GameBase::get().getFrameNumber()))
		{
			animation->render(*FRD);
		}
	}

}



