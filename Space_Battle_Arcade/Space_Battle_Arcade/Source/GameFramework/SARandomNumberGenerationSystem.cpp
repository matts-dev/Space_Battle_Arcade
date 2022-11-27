#include <ctime>

#include "SARandomNumberGenerationSystem.h"
#include <cstdint>

namespace SA
{
	sp<RNG> RNGSystem::getNamedRNG(const std::string rngName)
	{
		auto findResult = namedGenerators.find(rngName);
		if (findResult != namedGenerators.end())
		{
			return findResult->second;
		}
		else
		{
			sp<RNG> newRNG = createNewRNG(rootNamedRNG);
			namedGenerators[rngName] = newRNG;
			return newRNG;
		}
	}

	sp<SA::RNG> RNGSystem::getSeededRNG(uint32_t seed)
	{
		sp<RNG> newRNG{ new RNG({seed}) };
		return newRNG;
	}

	sp<SA::RNG> RNGSystem::getTimeInfluencedRNG()
	{
		return createNewRNG(rootTimeInfluencedRNG);
	}

	void RNGSystem::postConstruct()
	{
		SystemBase::postConstruct();

		if (SA_RNG_USE_TIME)
		{
			rootNamedRNG = sp<RNG>(new RNG{ std::initializer_list			{ (uint32_t)std::time(nullptr) } });
			rootTimeInfluencedRNG = sp<RNG>(new RNG{ std::initializer_list	{ 2*(uint32_t)std::time(nullptr)} });
		}
		else
		{
			rootNamedRNG = sp<RNG>(new RNG{ std::initializer_list {7u, 54u, 11u, 29u, 0u} });
			rootTimeInfluencedRNG = sp<RNG>(new RNG{ std::initializer_list {19u, 3u, 107u, 67u, 9u} });
		}
	}

	sp<RNG> RNGSystem::createNewRNG(sp<RNG>& seedSrcRNG)
	{
		uint32_t seed = seedSrcRNG->getInt<uint32_t>();
		sp<RNG> newRNG{ new RNG({seed}) };
		return newRNG;
	}

}

