#include "Widget3D_Ship.h"
#include "../../../SAShip.h"
#include "../../../GameSystems/SAUISystem_Game.h"
#include "../../../SpaceArcade.h"
#include "../text/DigitalClockFont.h"
#include "../text/DigitalClockFont.h"

namespace SA
{
	enum class ETextRenderMode
	{
		SINGLE_GLYPH,
		INSTANCE_STRING,
		INSTANCE_STRING_BATCHED
	};
	//static constexpr ETextRenderMode TEXT_RENDER_MODE = ETextRenderMode::SINGLE_GLYPH;
	//static constexpr ETextRenderMode TEXT_RENDER_MODE = ETextRenderMode::INSTANCE_STRING;
	static constexpr ETextRenderMode TEXT_RENDER_MODE = ETextRenderMode::INSTANCE_STRING_BATCHED;

	static char textBuffer[1024];

#define DEBUG_SHIP_POS 1

	void Widget3D_Ship::setOwnerShip(const sp<Ship>& ship)
	{
		//remove bindings
		if (ownerShip)
		{
			ownerShip->onDestroyedEvent->removeAll(sp_this());
		}

		ownerShip = ship;

		//add bindings to new owner
		if (ownerShip)
		{
			ownerShip->onDestroyedEvent->addWeakObj(sp_this(), &Widget3D_Ship::handleOwnerDestroyed);
		}
	}

	void Widget3D_Ship::postConstruct()
	{
		const sp<UISystem_Game>& gameUISystem = SpaceArcade::get().getGameUISystem();
		gameUISystem->onUIGameRender.addStrongObj(sp_this(), &Widget3D_Ship::handleRenderGameUI);

		DigitalClockFont::Data textInit;
		if constexpr (TEXT_RENDER_MODE == ETextRenderMode::SINGLE_GLYPH)
		{
			textInit.shader = getDefaultGlyphShader_uniformBased();
		}
		else if constexpr (TEXT_RENDER_MODE == ETextRenderMode::INSTANCE_STRING || TEXT_RENDER_MODE == ETextRenderMode::INSTANCE_STRING_BATCHED)
		{
			textInit.shader = getDefaultGlyphShader_instanceBased();
		}
		textRenderer = new_sp<DigitalClockFont>(textInit);
	}

	void Widget3D_Ship::onDestroyed()
	{
		Parent::onDestroyed();

		//prevent leak on fast binding
		const sp<UISystem_Game>& gameUISystem = SpaceArcade::get().getGameUISystem();
		gameUISystem->onUIGameRender.removeStrong(sp_this(), &Widget3D_Ship::handleRenderGameUI);
	}

	void Widget3D_Ship::handleOwnerDestroyed(const sp<GameEntity>& entity)
	{
		if (ownerShip) //as fwp, this should not clear until the actual entity is freed from memory
		{
			ownerShip->onDestroyedEvent->removeWeak(sp_this(), &Widget3D_Ship::handleOwnerDestroyed);

			//go ahead and clear the owner, don't wait for the object to be freed. 
			ownerShip = nullptr;
		}
	}

	void Widget3D_Ship::handleRenderGameUI(GameUIRenderData& rd_ui)
	{
		using namespace glm;

		if (ownerShip && bShouldRender)
		{
			Transform textXform = textRenderer->getXform();

			vec3 shipPos = ownerShip->getWorldPosition();
			glm::vec3 camUp_n = rd_ui.camUp();
			glm::quat camRot = rd_ui.camQuat();
#if DEBUG_SHIP_POS
			float upOffset = 2.0f;
			textXform.position = shipPos + camUp_n * upOffset;
			textXform.rotQuat = camRot;
			textXform.scale = vec3(0.25f);
			textRenderer->setXform(textXform);

			snprintf(textBuffer, sizeof(textBuffer), "pos: (%3.1f), (%3.1f), (%3.1f)", shipPos.x, shipPos.y, shipPos.z);
			textRenderer->setText(textBuffer);
			renderText_Multiplexed(rd_ui);
#endif
		}
	}

	void Widget3D_Ship::renderText_Multiplexed(GameUIRenderData& rd_ui)
	{
		const RenderData* rd = rd_ui.renderData();
		if (textRenderer && rd)
		{
			if constexpr (TEXT_RENDER_MODE == ETextRenderMode::SINGLE_GLYPH)
			{
				textRenderer->render(*rd);
			}
			else if constexpr (TEXT_RENDER_MODE == ETextRenderMode::INSTANCE_STRING)
			{
				textRenderer->renderGlyphsAsInstanced(*rd);
			}
			else if constexpr (TEXT_RENDER_MODE == ETextRenderMode::INSTANCE_STRING_BATCHED)
			{
				static const sp<UISystem_Game>& ui = SpaceArcade::get().getGameUISystem();
				ui->batchToDefaultText(*textRenderer, rd_ui);
			}
		}
	}



}

