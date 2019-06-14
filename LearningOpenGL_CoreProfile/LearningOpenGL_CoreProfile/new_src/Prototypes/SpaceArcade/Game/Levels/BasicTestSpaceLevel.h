#pragma once
#include <map>

#include "SABaseLevel.h"


namespace SA
{
	class Ship;
	class RenderModelEntity;
	class ProjectileClassHandle;

	class BasicTestSpaceLevel : public BaseLevel
	{
	private:
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;

		void handleLeftMouseButton(int state, int modifier_keys);

	private:
		//std::multimap<RenderModelEntity*, wp<RenderModelEntity>> cachedSpawnEntities;

		sp<ProjectileClassHandle> laserBoltHandle;
	};
}
