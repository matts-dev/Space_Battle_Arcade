#pragma once
#include "../../GameFramework/Components/SAComponentEntity.h"

namespace SA
{
	constexpr float MAX_ENERGY = 100;

	class ShipEnergyComponent : public GameComponentBase
	{
	public:
		float spendEnergy(float request);
		float spendEnergyFractional(float energyRequest);
		float getEnergy() const { return energy; }
		float getMaxEnergy() const { return MAX_ENERGY; }

		void notify_tick(float dt_sec);
	public:
		float energy = MAX_ENERGY;
		float regenPerSec = 20; //eg 20 takes 5 seconds to regenerate energy to full
	};
}
