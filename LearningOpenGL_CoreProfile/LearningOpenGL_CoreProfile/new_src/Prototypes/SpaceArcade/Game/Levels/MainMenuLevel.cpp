#include "MainMenuLevel.h"
#include<ctime>
#include "../../GameFramework/SARandomNumberGenerationSystem.h"
#include "../../GameFramework/SAGameBase.h"
#include "../Environment/Planet.h"

namespace SA
{
	void MainMenuLevel::onCreateLocalPlanets()
	{
		constexpr bool generateRandomPlanet = true;
		sp<RNG> rng = GameBase::get().getRNGSystem().getSeededRNG(generateRandomPlanet ? uint32_t(std::time(NULL)) : 13);

		planets.push_back(makeRandomPlanet(*rng));
		const sp<Planet>& myPlanet = planets.back(); //do something with planet?
	}

}














































