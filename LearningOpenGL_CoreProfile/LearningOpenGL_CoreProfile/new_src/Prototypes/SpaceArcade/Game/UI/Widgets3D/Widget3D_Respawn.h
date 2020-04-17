#pragma once

#include "Widget3D_Base.h"

namespace SA
{
	class Widget3D_Respawn : public Widget3D_Base
	{
		using Parent = Widget3D_Base;
	protected:
		virtual void postConstruct();
	public:
		virtual void render(GameUIRenderData& renderData);
	};
}

