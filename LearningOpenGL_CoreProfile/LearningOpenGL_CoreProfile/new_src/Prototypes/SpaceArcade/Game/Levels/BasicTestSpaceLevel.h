#pragma once
#include "..\..\GameFramework\SALevel.h"
#include <map>


namespace SA
{
	class Ship;
	class RenderModelEntity;

	class BasicTestSpaceLevel : public Level
	{
	private:
		virtual void startLevel() override;
		virtual void endLevel() override;

	private:
		std::multimap<RenderModelEntity*, wp<RenderModelEntity>> cachedSpawnEntities;
	};
}
