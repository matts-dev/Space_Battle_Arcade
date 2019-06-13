#pragma once
#include <map>

#include "SABaseLevel.h"


namespace SA
{
	class Ship;
	class RenderModelEntity;

	class BasicTestSpaceLevel : public BaseLevel
	{
	private:
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;

	private:
		std::multimap<RenderModelEntity*, wp<RenderModelEntity>> cachedSpawnEntities;
	};
}
