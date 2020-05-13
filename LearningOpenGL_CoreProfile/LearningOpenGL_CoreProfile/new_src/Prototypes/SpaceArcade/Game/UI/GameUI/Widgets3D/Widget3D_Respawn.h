#pragma once

#include "Widget3D_Base.h"
#include "../../../../Tools/DataStructures/MultiDelegate.h"
#include "../../../../GameFramework/SAGameEntity.h"

namespace SA
{
	class DigitalClockFont;

	class Widget3D_Respawn : public Widget3D_Base
	{
		using Parent = Widget3D_Base;
	protected:
		virtual void postConstruct();
	public:
		virtual void renderGameUI(struct GameUIRenderData& renderData);
	private:
		void handleRespawnStarted(float timeUntilRespawn);
		void handleRespawnOver(bool bSuccess);
		void timerTick();
	private:
		sp<MultiDelegate<>> timerDelegate;
		sp<DigitalClockFont> textRenderer;
		float cacheSecsUntilRespawn = 0.f;
		float tickFrequencySec = 0.1f;
		bool bIsRespawning = false;
	};
}

