#pragma once

#include "Widget3D_MenuScreenBase.h"

namespace SA
{
	class Widget3D_SettingsScreen : public Widget3D_MenuScreenBase
	{
		using Parent = Widget3D_MenuScreenBase;
	public:
		GAMEMENUSCREENBASE_EXPOSE_CLICK_DELEGATE(getBackButton, backButton);
	public:
		virtual void tick(float dt_sec) override;
	protected:
		virtual void postConstruct() override;
		virtual void onActivationChanged(bool bActive) override;
		virtual void renderGameUI(GameUIRenderData& ui_rd) override;
	private:
		sp<Widget3D_LaserButton> backButton = nullptr;
	};
}