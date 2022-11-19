#pragma once
#include "Game/UI/GameUI/Widgets3D/Widget3D_Base.h"
#include "Tools/DataStructures/AdvancedPtrs.h"
#include "Tools/DataStructures/MultiDelegate.h"

namespace SA
{
	class Ship;
	class DigitalClockFont;

	/** Destroy this object to prevent leaks. */
	class Widget3D_Ship : public Widget3D_Base
	{
		using Parent = Widget3D_Base;
	public:
		void setOwnerShip(const sp<Ship>& ship);
	protected:
		virtual void postConstruct() override;
		virtual void onDestroyed() override;
		virtual void renderGameUI(GameUIRenderData& renderData) override { handleRenderGameUI(renderData); }
	private:
		void handleOwnerDestroyed(const sp<GameEntity>& entity);
		void handleRenderGameUI(struct GameUIRenderData& rd_ui);
		void renderText_Multiplexed(GameUIRenderData& rd_ui);
	private:
		fwp<Ship> ownerShip;	//not using lifetime pointer as that will fire before destroyed event ATOW
		sp<DigitalClockFont> textRenderer = nullptr;
		bool bShouldRender = true;
	};

}