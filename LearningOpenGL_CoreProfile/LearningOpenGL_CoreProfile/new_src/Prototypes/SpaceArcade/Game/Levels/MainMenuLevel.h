#pragma once
#include "SASpaceLevelBase.h"

namespace SA
{
	class MainMenuLevel : public SpaceLevelBase
	{
		virtual void onCreateLocalPlanets() override;
	};

}