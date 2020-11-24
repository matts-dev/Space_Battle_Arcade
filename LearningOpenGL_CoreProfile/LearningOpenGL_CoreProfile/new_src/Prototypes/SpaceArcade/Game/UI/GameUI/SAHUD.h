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
	class Widget3D_LaserButton;
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
		void toggleEscapeMenu() { setEscapeMenuOpen(!bEscapeMenuOpen); }
		void setEscapeMenuOpen(bool bNewValue);
		bool requiresCursorMode() const { return bEscapeMenuOpen || false; }
	protected:
		virtual void postConstruct() override;
	private:
		void tryRegenerateTeamWidgets();
		void handleReturnToMainMenuClicked();
		void handleStarJumpOver();
		void handleEnterPressed(int state, int modifier_keys, int scancode);
	private:
		bool bRenderHUD = true;
		bool bEscapeMenuOpen = false;
		size_t playerIdx = 0; //#todo #splitscreen
		sp<Texture_2D> reticleTexture;
		sp<Shader> spriteShader;
		sp<TexturedQuad> quadShape;
		sp<class Widget3D_Respawn> respawnWidget = nullptr;
		sp<Widget3D_HealthBar> healthBar;
		sp<Widget3D_EnergyBar> energyBar;
		//sp<Widget3D_LaserButton> escapeMenuLeaveButton = nullptr; //disabling as there are some bugs with a moving button, just going with text for now
		sp<GlitchTextFont> escapeButtonText_Leave;
		sp<GlitchTextFont> escapeButtonText_Resume;


		std::vector<sp<Widget3D_TeamProgressBar>> teamHealthBars;
		sp<GlitchTextFont> slowmoText = nullptr;
		sp<MultiDelegate<>> starJumpTimer = nullptr;
		
#if HUD_FONT_TEST 
		sp<class Widget3D_DigitalClockFontTest> fontTest = nullptr;
#endif
	};
}