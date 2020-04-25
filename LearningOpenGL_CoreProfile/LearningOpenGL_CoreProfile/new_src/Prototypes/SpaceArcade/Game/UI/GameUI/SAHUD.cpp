#include "SAHUD.h"
#include "../../../GameFramework/SAGameBase.h"
#include "../../../GameFramework/SAWindowSystem.h"
#include "../../../Rendering/BuiltInShaders.h"
#include "../../../Rendering/SAGPUResource.h"
#include "../../../Rendering/SAShader.h"
#include "../../../Tools/Geometry/SimpleShapes.h"
#include "../../../Rendering/Camera/Texture_2D.h"
#include "Widgets3D/Widget3D_Respawn.h"
#include "Widgets3D/Widget3D_DigitalClockFontTest.h"
#include "../../../Tools/DataStructures/SATransform.h"
#include "../../GameSystems/SAUISystem_Game.h"
#include "../../SpaceArcade.h"

using namespace glm;

namespace SA
{



	void HUD::postConstruct()
	{
		SpaceArcade& game = SpaceArcade::get();
		game.getGameUISystem()->onUIGameRender.addWeakObj(sp_this(), &HUD::handleGameUIRender);

		reticleTexture = new_sp<Texture_2D>("GameData/engine_assets/SimpleCrosshair.png");
		quadShape = new_sp<TexturedQuad>();
		spriteShader = new_sp<Shader>(spriteVS_src, spriteFS_src, false);

		respawnWidget = new_sp<Widget3D_Respawn>();


#if HUD_FONT_TEST 
		fontTest = new_sp<class Widget3D_DigitalClockFontTest>();
#endif
	}

	void HUD::handleGameUIRender(GameUIRenderData& rd_ui)
	{
		if (bRenderHUD)
		{
			if (spriteShader)
			{
				//draw reticle
				if (quadShape && reticleTexture && reticleTexture->hasAcquiredResources() && spriteShader)
				{
					float reticalSize = 0.015f * float(rd_ui.frameBuffer_MinDimension()); //X% of smallest screen dimension 

					mat4 model(1.f);
					model = glm::scale(model, vec3(reticalSize));

					spriteShader->use();
					spriteShader->setUniform1i("textureData", 0); 
					spriteShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
					spriteShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(rd_ui.orthographicProjection_m()));
					reticleTexture->bindTexture(GL_TEXTURE0); 
					quadShape->render();
				}
			}

			respawnWidget->render(rd_ui);
#if HUD_FONT_TEST 
			fontTest->render(rd_ui);
#endif
		}
	}


}

