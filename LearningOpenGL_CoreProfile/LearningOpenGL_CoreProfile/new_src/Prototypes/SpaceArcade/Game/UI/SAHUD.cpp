#include "SAHUD.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SAWindowSystem.h"
#include "../../Rendering/BuiltInShaders.h"
#include "../../Rendering/SAGPUResource.h"
#include "../../Rendering/SAShader.h"
#include "../../Tools/Geometry/SimpleShapes.h"
#include "../../Rendering/Camera/Texture_2D.h"

using namespace glm;

namespace SA
{



	void HUD::postConstruct()
	{
		GameBase& game = GameBase::get();

		reticleTexture = new_sp<Texture_2D>("GameData/engine_assets/SimpleCrosshair.png");
		quadShape = new_sp<TexturedQuad>();
		spriteShader = new_sp<Shader>(spriteVS_src, spriteFS_src, "", false);

	}

	void HUD::render(float dt_sec) const
	{
		if (bRenderHUD)
		{
			static WindowSystem& windowSystem = GameBase::get().getWindowSystem();
			const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow();

			const std::pair<int, int> framebufferSize = primaryWindow->getFramebufferSize();
			const int width = framebufferSize.first;
			const int height = framebufferSize.second;
			const int minSize = glm::min<int>(width, height);

			float nearPlane = -0.1f;
			float farPlane = 100.0f;
			//mat4 ortho_projection(1.0f);
			mat4 ortho_projection = glm::ortho( 
				-(float)width / 2.0f, (float)width / 2.0f,
				-(float)height / 2.0f, (float)height / 2.0f
			);

			if (spriteShader && primaryWindow)
			{
				//draw reticle
				if (quadShape && reticleTexture && reticleTexture->hasAcquiredResources() && spriteShader)
				{
					float reticalSize = 0.015f * float(minSize); //X% of smallest screen dimension 

					mat4 model(1.f);
					//model = glm::translate(model, vec3(width/2.f, height/2.f, 0.f));
					model = glm::scale(model, vec3(reticalSize));

					spriteShader->use();
					spriteShader->setUniform1i("textureData", 0); 
					spriteShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(model));
					spriteShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(ortho_projection));
					reticleTexture->bindTexture(GL_TEXTURE0);
					quadShape->render();
				}
			}
		}
	}

}

