#include "SAPlayer.h"
#include "../Rendering/Camera/SACameraFPS.h"
#include "../GameFramework/SAWindowSystem.h"
#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/Interfaces/SAIControllable.h"
#include "../GameFramework/SALevel.h"
#include "../GameFramework/SALevelSystem.h"
#include "../GameFramework/Components/SAComponentEntity.h"
#include "../GameFramework/Components/GameplayComponents.h"
#include "Levels/SASpaceLevelBase.h"
#include "SAShip.h"
#include "Components/FighterSpawnComponent.h"
#include "SpaceArcade.h"
#include "../GameFramework/Input/SAInput.h"

namespace SA
{
	sp<CameraBase> SAPlayer::generateDefaultCamera() const
	{
		const sp<Window>& primaryWindow = GameBase::get().getWindowSystem().getPrimaryWindow();
		sp<CameraBase> defaultCamera = new_sp<CameraFPS>();
		defaultCamera->registerToWindowCallbacks_v(primaryWindow);

		//configure camera to match current camera if there is one
		if (const sp<CameraBase>& currentCamera = getCamera())
		{
			defaultCamera->setPosition(currentCamera->getPosition());
			defaultCamera->lookAt_v(currentCamera->getPosition() + currentCamera->getFront());
		}

		return defaultCamera;
	}

	void SAPlayer::postConstruct()
	{
		PlayerBase::postConstruct();
		respawnTimerDelegate = new_sp<MultiDelegate<>>();
		respawnTimerDelegate->addWeakObj(sp_this(), &SAPlayer::handleRespawnTimerUp);

		getInput().getKeyEvent(GLFW_KEY_ESCAPE).addWeakObj(sp_this(), &SAPlayer::handleEscapeKey);

	}

	void SAPlayer::onNewControlTargetSet(IControllable* oldTarget, IControllable* newTarget)
	{
		PlayerBase::onNewControlTargetSet(oldTarget, newTarget);

		/////////////////////////////////////////////////////////////////////////////////////
		// clean up old entity state
		/////////////////////////////////////////////////////////////////////////////////////
		if (GameEntity* oldEntity = oldTarget ? oldTarget->asEntity() : nullptr)
		{
 			oldEntity->onDestroyedEvent->removeAll(sp_this());
		}
		spawnComp_safeCache = nullptr;

		////////////////////////////////////////////////////////
		// subscribe new entity state
		////////////////////////////////////////////////////////
		if (GameEntity* newEntity = newTarget? newTarget->asEntity() : nullptr)
		{
			newEntity->onDestroyedEvent->addWeakObj(sp_this(), &SAPlayer::handleControlTargetDestroyed);

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// dynamic casting sucks, but this is done by player so will not happen too often (relatively speaking)
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			//cache team info for the entity we're controlling 
			if (GameplayComponentEntity* GPEntity = dynamic_cast<GameplayComponentEntity*>(newEntity))
			{
				if (TeamComponent* TeamComp = GPEntity->getGameComponent<TeamComponent>())
				{
					currentTeamIdx = TeamComp->getTeam();
				}
			}
			if (Ship* ship = dynamic_cast<Ship*>(newEntity))
			{
				spawnComp_safeCache = ship->getOwningSpawner_cacheAsWeak();
			}
		}
	}

	void SAPlayer::handleControlTargetDestroyed(const sp<GameEntity>& entity)
	{
		LevelSystem& lvlSys = GameBase::get().getLevelSystem();
		if (const sp<LevelBase>& currentLevel = lvlSys.getCurrentLevel())
		{
			if (const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager())
			{
				float respawnTimeSec = 5.f; //this should be controlled by a gamemode like class, avoiding putting in player header right now
				worldTimeManager->createTimer(respawnTimerDelegate, respawnTimeSec);
				onRespawnStarted.broadcast(respawnTimeSec);
			}
		}
	}

	void SAPlayer::handleRespawnTimerUp()
	{
		bool bRespawnSucess = false;

		////////////////////////////////////////////////////////
		// find valid spawn component to use
		////////////////////////////////////////////////////////
		FighterSpawnComponent* spawnComp = nullptr;
		if (spawnComp_safeCache)
		{
			spawnComp = spawnComp_safeCache.fastGet();
		}
		else
		{
			//static LevelSystem& LevelSys = GameBase::get().getLevelSystem();
			//const sp<LevelBase>& level = LevelSys.getCurrentLevel();
			//if (level)
			//{
			//	if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(level.get()))
			//	{
			//		if (TeamCommander* teamCommander = spaceLevel->getTeamCommander(currentTeamIdx))
			//		{
						//#TODO find another spawn component to spawn on? Perhaps through team commander? Probably better system
						//this case may not even need to be addressed if there are never more than 1 carrier ship per team
			//		}
			//	}
			//}
		}


		////////////////////////////////////////////////////////
		// use spawn component to spawn ship
		////////////////////////////////////////////////////////
		if (spawnComp)
		{
			if (sp<Ship> newShip = spawnComp->spawnEntity())
			{
				setControlTarget(newShip);
				bRespawnSucess = true;
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_WARNING, "attempted player respawn but could not find a valid spawn component");
		}

		onRespawnOver.broadcast(bRespawnSucess);
	}

	void SAPlayer::handleEscapeKey(int state, int modifier_keys, int scancode)
	{
		if (state == GLFW_PRESS)
		{
			SpaceArcade& game = SpaceArcade::get();
			if (modifier_keys == GLFW_KEY_LEFT_SHIFT)
			{
				game.startShutdown();
			}
			else
			{
				if (game.isEditorMainmenuFeatureEnabled() && game.escapeShouldOpenEditorMenu())
				{
					if (const sp<CameraBase>& camera = getCamera())
					{
						//probably should move editor UI to level off of some subsystem
						game.toggleEditorUIMainMenuVisible();
						camera->setCursorMode(SpaceArcade::get().isEditorMainMenuOnScreen() || camera->cameraRequiresCursorMode());
					}
				}
			}
		}
	}

}
