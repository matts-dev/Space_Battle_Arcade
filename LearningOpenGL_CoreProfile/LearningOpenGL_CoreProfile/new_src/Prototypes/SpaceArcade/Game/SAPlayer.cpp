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
#include "GameSystems/SAModSystem.h"
#include "../Tools/PlatformUtils.h"
#include "AssetConfigs/SASettingsProfileConfig.h"
#include "../GameFramework/SALog.h"
#include "GameModes/ServerGameMode_CarrierTakedown.h"
#include "Team/Commanders.h"

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

	void SAPlayer::setSettingsProfile(const sp<SettingsProfileConfig>& newSettingsProfile)
	{
		sp<SettingsProfileConfig> oldSettings = settings;
		settings = newSettingsProfile;
		onSettingsChanged.broadcast(oldSettings, newSettingsProfile);
	}

	void SAPlayer::postConstruct()
	{
		PlayerBase::postConstruct();
		respawnTimerDelegate = new_sp<MultiDelegate<>>();
		respawnTimerDelegate->addWeakObj(sp_this(), &SAPlayer::handleRespawnTimerUp);

		const sp<ModSystem>& modSystem = SpaceArcade::get().getModSystem();
		modSystem->onActiveModChanging.addWeakObj(sp_this(), &SAPlayer::handleActiveModChanging);

		//register to system ticker, rather than world ticker, so that player gets ticked even during level transitions and no resubscription is needed
		GameBase::get().getSystemTimeManager().registerTicker(sp_this());
		GameBase::get().onShutdownInitiated.addWeakObj(sp_this(), &SAPlayer::handleShutdownStarted);

		if (const sp<Mod>& activeMod = modSystem->getActiveMod())
		{
			handleActiveModChanging(nullptr, activeMod); //this will get the settings profile from the modk
		}

		getInput().getKeyEvent(GLFW_KEY_ESCAPE).addWeakObj(sp_this(), &SAPlayer::handleEscapeKey);
		getInput().getKeyEvent(GLFW_KEY_LEFT_CONTROL).addWeakObj(sp_this(), &SAPlayer::handleControlPressed);
		getInput().getKeyEvent(GLFW_KEY_RIGHT_CONTROL).addWeakObj(sp_this(), &SAPlayer::handleControlPressed);
		getInput().getKeyEvent(GLFW_KEY_TAB).addWeakObj(sp_this(), &SAPlayer::handleTabPressed);
	}

	void SAPlayer::onNewControlTargetSet(IControllable* oldTarget, IControllable* newTarget)
	{
		PlayerBase::onNewControlTargetSet(oldTarget, newTarget);

		/////////////////////////////////////////////////////////////////////////////////////
		// clean up old entity state
		/////////////////////////////////////////////////////////////////////////////////////
		if (GameEntity* oldEntity = oldTarget ? oldTarget->asWorldEntity() : nullptr)
		{
 			oldEntity->onDestroyedEvent->removeAll(sp_this());
		}
		spawnComp_safeCache = nullptr;

		////////////////////////////////////////////////////////
		// subscribe new entity state
		////////////////////////////////////////////////////////
		if (GameEntity* newEntity = newTarget? newTarget->asWorldEntity() : nullptr)
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
			if (spawnComp_safeCache->isActive())
			{
				spawnComp = spawnComp_safeCache.fastGet();
			}
		}

		if(!spawnComp) 
		{
			//fix bug: player spawns at dead carrier after it has been destroyed in skrimish match where there are more than 1 carrier per team.
			static LevelSystem& LevelSys = GameBase::get().getLevelSystem();
			const sp<LevelBase>& level = LevelSys.getCurrentLevel();
			if (level)
			{
				if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(level.get()))
				{
					if (TeamCommander* teamCommander = spaceLevel->getTeamCommander(currentTeamIdx))
					{
						for (const fwp<WorldEntity>& carrierWeak : teamCommander->getTeamCarriers())
						{
							if (WorldEntity* carrier = carrierWeak.isValid() ? carrierWeak.fastGet() : nullptr)
							{
								FighterSpawnComponent* carrierSpawnComp = carrier->getGameComponent<FighterSpawnComponent>();
								if (carrierSpawnComp && carrierSpawnComp->isActive())
								{
									spawnComp_safeCache = carrierSpawnComp->requestTypedReference_Safe<FighterSpawnComponent>();
									spawnComp = carrierSpawnComp;
									break; //stop looking for a spawn component, we found one that will work
								}
							}
						}
					}
				}
			}
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

	bool SAPlayer::canDilateTime() const 
	{
		if (td.bUseCooldownMode)
		{
			if (td.bEnablePlayerTimeDilationControl)
			{
				if (currentTimeSec > td.lastDilationTimestamp + td.DILATION_COOLDOWN_SEC
					&& currentTimeSec > td.lastDilationStart + td.DILATION_COOLDOWN_SEC //extra check so that this returns false immediately when activating slowmo
					)
				{
					return true;
				}
			}
		}
		else
		{
			if (td.nextSlomoKillCount <= shipsKilled)
			{
				return true;
			}
		}

		return false;
	}

	void SAPlayer::incrementKillCount()
	{
		shipsKilled += 1;
	}

	void SAPlayer::cheat_infiniteTimeDilation()
	{
		td.bEnablePlayerTimeDilationControl = true;

		//cooldown version
		td.DILATION_COOLDOWN_SEC = 0.001f;
		td.DILATION_MAX_DURATION = 100000.f;

		//kill count version
		td.NUM_KILLS_FOR_SLOWMO_INCREMENT = 0;
		td.nextSlomoKillCount = 0;
	}

	void SAPlayer::handleControlPressed(int state, int modifier_keys, int scancode)
	{
		if (state == GLFW_PRESS)
		{
			if (!td.bIsDilatingTime && canDilateTime())
			{
				startDilation();
			}
		}
		else if (state == GLFW_RELEASE) //must check release, as repeat, etc, will cause this to fire.
		{
			if (td.bIsDilatingTime)
			{
				stopDilation();
			}
		}
	}

	void SAPlayer::handleTabPressed(int state, int modifier_keys, int scancode)
	{
		if (state == GLFW_PRESS)
		{
			SpaceArcade& game = SpaceArcade::get();
			game.bOnlyHighlightTargets = !game.bOnlyHighlightTargets;
		}
	}

	bool SAPlayer::tick(float dt_sec)
	{
		//this is registered to system time manager, if you want to use dilated time you need to get that from the current level time manager.
		currentTimeSec += dt_sec;

		if (bool bIsFirstPlayer = getPlayerIndex() == 0)
		{
			const float dilationChangeSpeedSec = 1.f;
			const float dilationMin = 0.2f;
			const float dilationMax = 1.f;

			float dilationChange = (td.bIsDilatingTime ? -1.f : 1.f) * dilationChangeSpeedSec * dt_sec;
			td.playerTimeDilationMultiplier = glm::clamp(td.playerTimeDilationMultiplier + (dilationChange), dilationMin, dilationMax);

			if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
			{
				if (const sp<TimeManager>& worldTimeManager = currentLevel->getWorldTimeManager())
				{
					const float currentDilation = worldTimeManager->getTimeDilationFactor();
					const float& playerDilation = td.playerTimeDilationMultiplier;

					if (currentDilation != playerDilation && !td.bTimeRestored)
					{
						worldTimeManager->setTimeDilationFactor_OnNextFrame(td.playerTimeDilationMultiplier);
						td.bTimeRestored = playerDilation == 1.f;
					}
				}
			}

			//stop time dilating after some amount of time
			if (td.bIsDilatingTime && currentTimeSec > td.lastDilationStart + td.DILATION_MAX_DURATION)
			{
				stopDilation();
			}
		}

		return true;
	}

	void SAPlayer::startDilation()
	{
		td.bIsDilatingTime = true;
		td.bTimeRestored = false;
		td.lastDilationStart = currentTimeSec;
		td.nextSlomoKillCount += td.NUM_KILLS_FOR_SLOWMO_INCREMENT;
	}

	void SAPlayer::stopDilation()
	{
		td.bIsDilatingTime = false;
		td.bTimeRestored = false;
		td.lastDilationTimestamp = currentTimeSec;
	}

	void SAPlayer::handleActiveModChanging(const sp<Mod>& previous, const sp<Mod>& active)
	{
		if (active)
		{
			settings = active->getSettingsProfile(0); 
		}
		else
		{
			settings = new_sp<SettingsProfileConfig>(); //create default settings if we cannot get a settings profile
		}
	}

	void SAPlayer::handleShutdownStarted()
	{
		//clear strong binding to this 
		GameBase::get().getSystemTimeManager().removeTicker(sp_this());
	}

}
