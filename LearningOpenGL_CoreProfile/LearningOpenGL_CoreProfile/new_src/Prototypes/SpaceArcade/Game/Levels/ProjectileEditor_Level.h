#pragma once
#include "..\..\GameFramework\SALevel.h"
#include <map>

namespace SA
{
	class RenderModelEntity;

	class ProjectileEditor_Level : public Level
	{
	private:
		virtual void startLevel() override;
		virtual void endLevel() override;
	private:
		std::multimap<RenderModelEntity*, wp<RenderModelEntity>> spawnedModels;
	};
}
