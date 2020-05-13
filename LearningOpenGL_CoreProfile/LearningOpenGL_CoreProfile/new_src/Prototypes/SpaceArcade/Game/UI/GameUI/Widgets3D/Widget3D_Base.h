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
		using Parent = SystemComponentEntity;
	public:
		/** Mutable parameter is to allow lazy calculation*/
		virtual void renderGameUI(GameUIRenderData& renderData) = 0;
	protected:
		void setRenderWithGameUIDispatch(bool bRender);
		virtual void onDestroyed() override;
	};

}