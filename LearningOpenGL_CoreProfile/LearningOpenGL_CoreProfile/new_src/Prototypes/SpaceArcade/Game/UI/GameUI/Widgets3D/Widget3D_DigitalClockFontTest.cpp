#include "Widget3D_DigitalClockFontTest.h"
#include "../text/DigitalClockFont.h"
#include "../../../../GameFramework/SAGameBase.h"
#include "../../../../GameFramework/SAPlayerSystem.h"
#include "../../../../GameFramework/SAPlayerBase.h"
#include "../../../../Rendering/Camera/SACameraBase.h"
#include "../../../../Rendering/SAShader.h"
#include "../../../../GameFramework/SARenderSystem.h"
#include "../../../../Rendering/RenderData.h"

namespace SA
{
	void Widget3D_DigitalClockFontTest::postConstruct()
	{
		rawGlyph = new_sp<DigitalClockGlyph>();
		glyphShader = getDefaultGlyphShader_uniformBased();

		DigitalClockFont::Data init;
		init.text = 
R"(a bcdefghijklmnopqrstuvwxyz
A BCDEFGHIJKLMNOPQRSTUVWXYZ
1234567890
!@#$%^&*()_+-=[]{};:'",<.>/?\`~
)";

		//init.pivotHorizontal = DigitalClockFont::EHorizontalPivot::LEFT;
		init.pivotHorizontal = DigitalClockFont::EHorizontalPivot::CENTER;
		//init.pivotHorizontal = DigitalClockFont::EHorizontalPivot::RIGHT;

		//init.pivotVertical = DigitalClockFont::EVerticalPivot::TOP;
		init.pivotVertical = DigitalClockFont::EVerticalPivot::CENTER;
		//init.pivotVertical = DigitalClockFont::EVerticalPivot::BOTTOM;
		textRenderer = new_sp<DigitalClockFont>(init);

		Transform spawnXform;
		spawnXform.scale = glm::vec3(0.1f, 0.1f, 0.1f);
		textRenderer->setXform(spawnXform);


		////////////////////////////////////////////////////////
		// instanced text render
		////////////////////////////////////////////////////////
		init.shader = getDefaultGlyphShader_instanceBased(); //use instanced shader instead of default
		instancedTextRenderer = new_sp<DigitalClockFont>(init);
		textRenderer->setXform(spawnXform);
		
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
						rawGlyph->render(*glyphShader);//just render layout
					}
				}
				else if (constexpr bool bTestLettersIndividual = false)
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
						rawGlyph->render(*glyphShader);

						//render b
						glyphShader->setUniform1i("bitVec", DigitalClockGlyph::getCharToBitvectorMap()['b']);
						glyphShader->setUniformMatrix4fv("model", 1, GL_FALSE, glm::value_ptr(bMat));
						rawGlyph->render(*glyphShader);
					}
				}
				else if (constexpr bool bTestParagraph = false)
				{
					GameBase& game = GameBase::get();
					if (const RenderData* frd = game.getRenderSystem().getFrameRenderData_Read(game.getFrameNumber()))
					{
						//Transform xform = textRenderer->getXform();
						//xform.position = camPos + forwardOffset * 3.f;
						//xform.scale = vec3(0.1f);
						//textRenderer->setXform(xform);
						textRenderer->render(*frd);
					}
				}
				else if (constexpr bool bTestInstanced = false)
				{
					GameBase& game = GameBase::get();
					if (const RenderData* frd = game.getRenderSystem().getFrameRenderData_Read(game.getFrameNumber()))
					{
						instancedTextRenderer->renderGlyphsAsInstanced(*frd);
					}
				}
				else if (constexpr bool bCompareInstanceAgainstIndividual = false)
				{
					Transform xform = textRenderer->getXform();
					xform.scale = vec3(1.f);

					xform.position = vec3(0.f, 0.f, 0.f);
					textRenderer->setXform(xform);
					textRenderer->render(*frd);

					xform.position = vec3(0.f, 0.f, -5.f);
					textRenderer->setXform(xform);
					textRenderer->render(*frd);

					xform.position = vec3(0.f, 0.f, 5.f);
					textRenderer->setXform(xform);
					textRenderer->render(*frd);
				}
				else if (constexpr bool bCompareInstanceAgainstBatch = false)
				{
					GameBase& game = GameBase::get();
					if (const RenderData* frd = game.getRenderSystem().getFrameRenderData_Read(game.getFrameNumber()))
					{
						Transform xform = instancedTextRenderer->getXform();

						xform.position = vec3(0.f, 0.f, 0.f);
						instancedTextRenderer->setXform(xform);
						instancedTextRenderer->renderGlyphsAsInstanced(*frd);

						xform.position = vec3(0.f, 0.f, -5.f);
						instancedTextRenderer->setXform(xform);
						instancedTextRenderer->renderGlyphsAsInstanced(*frd);


						xform.position = vec3(0.f, 0.f, 5.f);
						instancedTextRenderer->setXform(xform);
						instancedTextRenderer->renderGlyphsAsInstanced(*frd);
					}
				}
				else if (constexpr bool bTestBatced = true)
				{
					GameBase& game = GameBase::get();
					if (const RenderData* frd = game.getRenderSystem().getFrameRenderData_Read(game.getFrameNumber()))
					{
						//just recycling same text render but rendering it in multiple spots.
						DCFont::BatchData batch;
						Transform xform = instancedTextRenderer->getXform();

						xform.position = vec3(0.f);
						instancedTextRenderer->setXform(xform);
						instancedTextRenderer->prepareBatchedInstance(*instancedTextRenderer, batch);

						xform.position = vec3(0.f,0.f, -5.f);
						instancedTextRenderer->setXform(xform);
						if (!instancedTextRenderer->prepareBatchedInstance(*instancedTextRenderer, batch))
						{
							//can't buffer, render what we have then buffer
							instancedTextRenderer->renderBatched(*frd, batch);
							if (!instancedTextRenderer->prepareBatchedInstance(*instancedTextRenderer, batch))
							{
								log(__FUNCTION__, LogLevel::LOG_ERROR, "Cannot batch text with this batch configuration.");
							}
						}

						xform.position = vec3(0.f, 0.f, 5.f);
						instancedTextRenderer->setXform(xform);
						if (!instancedTextRenderer->prepareBatchedInstance(*instancedTextRenderer, batch))
						{
							//can't buffer, render what we have then buffer
							instancedTextRenderer->renderBatched(*frd, batch);
							if (!instancedTextRenderer->prepareBatchedInstance(*instancedTextRenderer, batch))
							{
								log(__FUNCTION__, LogLevel::LOG_ERROR, "Cannot batch text with this batch configuration.");
							}
						}

						instancedTextRenderer->renderBatched(*frd, batch);
					}
				}
			}
		}
	}

}

