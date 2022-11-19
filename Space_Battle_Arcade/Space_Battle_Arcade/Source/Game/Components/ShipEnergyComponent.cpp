#include "ShipEnergyComponent.h"
#include "Tools/DataStructures/SATransform.h"

namespace SA
{
	float ShipEnergyComponent::spendEnergy(float request)
	{
		energy -= request;
		float spentEnergy = request;

		if(energy < 0)
		{
			//went below zero, deduct what we couldn't spend and remove that from the return.
			spentEnergy = request - glm::abs<float>(energy);
			energy = 0;
		}

		return spentEnergy;
	}

	float ShipEnergyComponent::spendEnergyFractional(float energyRequest)
	{
		float actualSpent = spendEnergy(energyRequest);
		return actualSpent / energyRequest;
	}

	void ShipEnergyComponent::notify_tick(float dt_sec)
	{
		energy = glm::clamp<float>(energy + regenPerSec * dt_sec, 0.f, MAX_ENERGY);
	}

}