#include "Widget3D_Respawn.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SAPlayerSystem.h"
#include "Game/SAPlayer.h"
#include "GameFramework/SALevel.h"
#include "GameFramework/SALevelSystem.h"
#include "Game/UI/GameUI/text/DigitalClockFont.h"
#include "Game/SpaceArcade.h"
#include "Game/GameSystems/SAUISystem_Game.h"
#include "GameFramework/SADebugRenderSystem.h"

namespace SA
{

	void Widget3D_Respawn::postConstruct()
	{
		Parent::postConstruct();

		timerDelegate = new_sp<MultiDelegate<>>();
		timerDelegate->addWeakObj(sp_this(), &Widget3D_Respawn::timerTick);

		DigitalClockFontInitData initText;
		initText.shader = getDefaultGlyphShader_instanceBased();
		//initText.shader = getDefaultGlyphShader_uniformBased();
		textRenderer = new_sp<DigitalClockFont>(initText);

		GameBase& game = GameBase::get();
		PlayerSystem& playerSystem = game.getPlayerSystem();
		if (const sp<PlayerBase>& playerBase = playerSystem.getPlayer(0))
		{
			if (SAPlayer* player = dynamic_cast<SAPlayer*>(playerBase.get()))
			{
				player->onRespawnStarted.addWeakObj(sp_this(), &Widget3D_Respawn::handleRespawnStarted);
				player->onRespawnOver.addWeakObj(sp_this(), &Widget3D_Respawn::handleRespawnOver);
			}
		}
	}

	void Widget3D_Respawn::renderGameUI(GameUIRenderData& ui_rd)
	{
		using namespace glm;

		if (bIsRespawning)
		{
			if (const RenderData* game_rd = ui_rd.renderData())
			{
				const HUDData3D& hud_3d = ui_rd.getHUDData3D();
				vec3 camPos = hud_3d.camPos;
				vec3 camFront = hud_3d.camFront;
				vec3 camRight = hud_3d.camRight;//test
				vec3 camUp = hud_3d.camUp;
				const float forwardDist = hud_3d.frontOffsetDist;
				const float upOffset = hud_3d.savezoneMax_y * 0.85f;
				
				Transform newXform;
				newXform.position = camPos + (forwardDist * camFront) + (camUp * upOffset);
				newXform.scale = vec3(hud_3d.textScale); // vec3(0.1f); for dist 10
				newXform.rotQuat = ui_rd.camQuat();
				textRenderer->setXform(newXform);

				textRenderer->renderGlyphsAsInstanced(*game_rd);
			}
		}
	}

	void Widget3D_Respawn::handleRespawnStarted(float timeUntilRespawn)
	{
		if (const sp<LevelBase>& world = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			if (const sp<TimeManager>& worldTimeManager = world->getWorldTimeManager())
			{
				worldTimeManager->createTimer(timerDelegate, tickFrequencySec , true);

				cacheSecsUntilRespawn = timeUntilRespawn + tickFrequencySec; //add 1 because we're going to immediately tick this and decrement
				bIsRespawning = true;
				timerTick();
			}
		}
	}

	void Widget3D_Respawn::handleRespawnOver(bool bSuccess)
	{
		if (const sp<LevelBase>& world = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			if (const sp<TimeManager>& worldTimeManager = world->getWorldTimeManager())
			{
				worldTimeManager->removeTimer(timerDelegate);
				bIsRespawning = false;
			}
		}
	}

	void Widget3D_Respawn::timerTick()
	{
		if (const sp<UISystem_Game>& gameUISystem = SpaceArcade::get().getGameUISystem())
		{
			float dilationFactor = 1.f;
			if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
			{
				dilationFactor = currentLevel->getWorldTimeManager()->getTimeDilationFactor();
			}

			auto& buffer = gameUISystem->textProcessingBuffer;

			cacheSecsUntilRespawn -= tickFrequencySec;
			snprintf(buffer, sizeof(buffer), "Respawn in %2.1f seconds", cacheSecsUntilRespawn);
			textRenderer->setText(buffer); 

			//{//DEBUG, DELETE ME
			//	const Transform& xform = textRenderer->getXform();
			//	glm::vec3 position = xform.position;
			//	snprintf(buffer, sizeof(buffer), "pos[%2.0f,%2.0f,%2.0f]", position.x, position.y, position.z);
			//	log(__FUNCTION__, LogLevel::LOG_ERROR, buffer);
			//	//textRenderer->setText(buffer);
			//	
			//}
		}
	}

}

