#pragma once
#include "../../../GameFramework/SAGameEntity.h"
#include <vector>

namespace SA
{
	class Window;
	class Texture_2D;
	class TexturedQuad;
	class Shader;
	class Widget3D_HealthBar;
	class Widget3D_EnergyBar;
	class Widget3D_TeamProgressBar;
	class GlitchTextFont;

#define HUD_FONT_TEST 0

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The object that renders the HUD to the screen
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class HUD : public GameEntity
	{
	public:
		void handleGameUIRender(struct GameUIRenderData& uiRenderData);
		void setVisibility(bool bVisible) { bRenderHUD = bVisible; };
		bool isVisible() { return bRenderHUD; };
	protected:
		virtual void postConstruct() override;
	private:
		void tryRegenerateTeamWidgets();
	private:
		bool bRenderHUD = true;
		size_t playerIdx = 0; //#todo #splitscreen
		sp<Texture_2D> reticleTexture;
		sp<Shader> spriteShader;
		sp<TexturedQuad> quadShape;
		sp<class Widget3D_Respawn> respawnWidget = nullptr;
		sp<Widget3D_HealthBar> healthBar;
		sp<Widget3D_EnergyBar> energyBar;

		std::vector<sp<Widget3D_TeamProgressBar>> teamHealthBars;
		sp<GlitchTextFont> slowmoText = nullptr;
		
#if HUD_FONT_TEST 
		sp<class Widget3D_DigitalClockFontTest> fontTest = nullptr;
#endif
	};
}