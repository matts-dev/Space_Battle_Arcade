#include "Widget3D_DigitalClockFontTest.h"
#include "../text/DigitalClockFont.h"
#include "../../../GameFramework/SAGameBase.h"
#include "../../../GameFramework/SAPlayerSystem.h"
#include "../../../GameFramework/SAPlayerBase.h"
#include "../../../Rendering/Camera/SACameraBase.h"
#include "../../../Rendering/SAShader.h"
#include "../../../GameFramework/SARenderSystem.h"
#include "../../../Rendering/RenderData.h"

namespace SA
{
	void Widget3D_DigitalClockFontTest::postConstruct()
	{
		rawGlyph = new_sp<DigitalClockGlyph>();
		glyphShader = getDefaultGlyphShader_uniformBased();

	}

	void Widget3D_DigitalClockFontTest::render(GameUIRenderData& renderData)
	{
		using namespace glm;
		GameBase& game = GameBase::get();
		if (const sp<PlayerBase>& player = game.getPlayerSystem().getPlayer(0)) 
		{
			RenderSystem& renderSystem = game.getRenderSystem();
			const RenderData* frd = renderSystem.getFrameRenderData_Read(game.getFrameNumber());
			const sp<CameraBase>& camera = player->getCamera();

			if (camera && frd)
			{
				vec3 camPos = camera->getPosition();
				vec3 front = camera->getFront(); 
				vec3 forwardOffset = front * 5.f;


				if (constexpr bool bTestAllBars = false)
				{
					if (glyphShader)
					{
						rawGlyph->render(renderData, *glyphShader);//just render layout
					}
				}
				else if (constexpr bool bTestLettersIndividual = true)
				{

					if (glyphShader)
					{
						using DCF = DigitalClockGlyph;

						mat4 aMat = glm::translate(mat4(1.f), camPos + forwardOffset);
						mat4 bMat = glm::translate(mat4(1.f), camPos + forwardOffset + vec3(DCF::GLYPH_WIDTH + DCF::BETWEEN_GLYPH_SPACE, 0.f, 0.f)); //offset to side by 1

						glyphShader->use();
						//glyphShader->setUniformMatrix4fv("view", 1, GL_FALSE, glm::value_ptr(frd->view));
						//glyphShader->setUniformMatrix4fv("projection", 1, GL_FALSE, glm::value_ptr(frd->projection));
						glyphShader->setUniformMatrix4fv("projection_view", 1, GL_FALSE, glm::value_ptr(frd->projection_view));

						//render a
						glyphShader->setUniform1i("bitVec", DigitalClockGlyph::getCharToBitvectorMap()['a']);
						glyphShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(aMat));
						rawGlyph->render(renderData, *glyphShader);

						//render b
						glyphShader->setUniform1i("bitVec", DigitalClockGlyph::getCharToBitvectorMap()['b']);
						glyphShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(bMat));
						rawGlyph->render(renderData, *glyphShader);
					}
				}
				else if (constexpr bool bTestParagraph = false)
				{

				}
				else if (constexpr bool bTestInstanced = false)
				{

				}
			}
		}
	}

}

