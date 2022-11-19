#pragma once
#include "Game/UI/GameUI/Widgets3D/Widget3D_Base.h"

namespace SA
{

	/////////////////////////////////////////////////////////////////////////////////////
	// Class for testing the digital clock font
	/////////////////////////////////////////////////////////////////////////////////////
	class Widget3D_DigitalClockFontTest final : public Widget3D_Base
	{
	public:
		void renderGameUI(GameUIRenderData& renderData) override;
	protected:
		virtual void postConstruct() override;
	private:
		sp<class DigitalClockGlyph> rawGlyph = nullptr;
		sp<class Shader> glyphShader = nullptr;
		sp<class DigitalClockFont> textRenderer = nullptr;
		sp<class DigitalClockFont> instancedTextRenderer = nullptr;
	};


}