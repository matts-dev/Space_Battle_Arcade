#pragma once
#include "Widget3D_ActivatableBase.h"

namespace SA
{
	class Widget3D_MenuScreenBase : public Widget3D_ActivatableBase
	{
	public:
		virtual void tick(float dt_sec) = 0;
	};
}
