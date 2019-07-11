#include "SAProjectileConfig.h"

namespace SA
{
	std::string ProjectileConfig::getRepresentativeFilePath()
	{
		return owningModDir + std::string("Assets/ProjectileConfigs/") + name + std::string(".json");
	}

}

