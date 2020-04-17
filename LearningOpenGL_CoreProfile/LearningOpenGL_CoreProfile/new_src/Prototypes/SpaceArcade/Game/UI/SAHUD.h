#pragma once
#include "../../GameFramework/SAGameEntity.h"

namespace SA
{
	class Window;
	class Texture_2D;
	class TexturedQuad;
	class Shader;

#define HUD_FONT_TEST 1

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// The object that renders the HUD to the screen
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class HUD : public GameEntity
	{
	public:
		void render(float dt_sec) const;
	protected:
		virtual void postConstruct() override;
	private:
		bool bRenderHUD = true;
		sp<Texture_2D> reticleTexture;
		sp<Shader> spriteShader;
		sp<TexturedQuad> quadShape;
		sp<class Widget3D_Respawn> respawnWidget = nullptr;
#if HUD_FONT_TEST 
		sp<class Widget3D_DClockFontTest> fontTest = nullptr;
#endif
	};
}