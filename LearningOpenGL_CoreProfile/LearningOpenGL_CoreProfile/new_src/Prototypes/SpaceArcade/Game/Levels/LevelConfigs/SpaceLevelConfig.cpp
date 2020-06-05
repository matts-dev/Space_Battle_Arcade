#include "SpaceLevelConfig.h"
#include "../../../GameFramework/SALog.h"

namespace SA
{

	void SpaceLevelConfig::setNumPlanets(size_t number)
	{
		numPlanets = number;

		//prevent json hacks from doing crazy size
		constexpr size_t planetLimit = 20;
		numPlanets = glm::clamp<size_t>(numPlanets, 0, planetLimit);

		while (numPlanets < number)
		{
			planets.emplace_back();
		}
	}

	bool SpaceLevelConfig::overridePlanetData(size_t idx, const PlanetData& inData)
	{
		if (idx < planets.size())
		{
			planets[idx] = inData;
		}
		else
		{
			//perhaps should just attempt to resize and see if it becomes available? that will be easier API but might create planets people are not expecting.
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Attempting to set planet data but array of planets does not have this index");
		}

		return false;
	}

	void SpaceLevelConfig::setNumStars(size_t number)
	{
		numStars = number;

		//prevent json hacks from doing crazy size
		constexpr size_t starLimit = 20;
		numStars = glm::clamp<size_t>(numStars, 0, starLimit);

		while (numStars < number)
		{
			stars.emplace_back();
		}
	}

	bool SpaceLevelConfig::overrideStarData(size_t idx, const StarData& inData)
	{
		if (idx < stars.size())
		{
			stars[idx] = inData;
		}
		else
		{
			//perhaps should just attempt to resize and see if it becomes available? that will be easier API but might create planets people are not expecting.
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Attempting to set star data but array of planets does not have this index");
		}

		return false;
	}

	std::string SpaceLevelConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("Assets/levels_configs/") + name + std::string(".json");
	}

	void SpaceLevelConfig::onSerialize(json& outData)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void SpaceLevelConfig::onDeserialize(const json& inData)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}


}


