#pragma once
#include "../../../../GameFramework/Components/SAComponentEntity.h"
#include "../../../GameSystems/SAUISystem_Game.h"

namespace SA
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Widget 3D base class
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_Base : public SystemComponentEntity
	{
	public:
		/** Mutable parameter is to allow lazy calculation*/
		virtual void render(GameUIRenderData& renderData) = 0;

	};

}