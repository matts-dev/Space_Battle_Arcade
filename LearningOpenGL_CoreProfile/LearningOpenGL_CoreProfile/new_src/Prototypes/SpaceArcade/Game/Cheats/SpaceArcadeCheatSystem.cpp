#include "SpaceArcadeCheatSystem.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/Interfaces/SAIControllable.h"
#include "../../GameFramework/Components/SAComponentEntity.h"
#include "../../GameFramework/Components/GameplayComponents.h"
#include "../../GameFramework/SAPlayerSystem.h"
#include "../Levels/LevelConfigs/SpaceLevelConfig.h"
#include "../Environment/Planet.h"
#include "../GameSystems/SAModSystem.h"
#include "../SpaceArcade.h"
#include "../AssetConfigs/SASpawnConfig.h"
#include "../../Tools/PlatformUtils.h"
#include "../Levels/SASpaceLevelBase.h"
#include "../../GameFramework/SALevel.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../AssetConfigs/SaveGameConfig.h"
#include <string>
#include "../AssetConfigs/CampaignConfig.h"
#include <vcruntime_exception.h>
#include "../../Rendering/Camera/SAQuaternionCamera.h"
#include "../../GameFramework/SAWindowSystem.h"
#include "../../GameFramework/SAAudioSystem.h"
#include "../SAPlayer.h"
#include "../Environment/StarField.h"

using namespace glm;

namespace SA
{
	void SpaceArcadeCheatSystem::postConstruct()
	{
		Parent::postConstruct();

#define REGISTER_CHEAT(name, fptr)\
{\
	sp<CheatDelegate> cheatDelegate = new_sp<CheatDelegate>();\
	cheatDelegate->addWeakObj(sp_this(), &fptr);\
	registerCheat(name, cheatDelegate);\
}
		REGISTER_CHEAT("one_shot_objectives", SpaceArcadeCheatSystem::cheat_oneShotObjectives);
		REGISTER_CHEAT("destroy_all_objectives", SpaceArcadeCheatSystem::cheat_destroyAllObjectives);
		REGISTER_CHEAT("destroy_all_generators", SpaceArcadeCheatSystem::cheat_destroyAllGenerators);
		REGISTER_CHEAT("turrets_target_player", SpaceArcadeCheatSystem::cheat_turretsTargetPlayer);
		REGISTER_CHEAT("comms_target_player", SpaceArcadeCheatSystem::cheat_commsTargetPlayer);
		REGISTER_CHEAT("kill_player", SpaceArcadeCheatSystem::cheat_killPlayer);
		REGISTER_CHEAT("make_json_template_spacelevelconfig", SpaceArcadeCheatSystem::cheat_make_json_template_spacelevelconfig);
		REGISTER_CHEAT("make_json_template_savegame", SpaceArcadeCheatSystem::cheat_make_json_template_savegame);
		REGISTER_CHEAT("levelCheat_transitionToMainMenuLevel", SpaceArcadeCheatSystem::cheat_mainMenuTransitionTest);
		REGISTER_CHEAT("unlock_And_Complete_All_Levels_In_Campaign", SpaceArcadeCheatSystem::cheat_unlockAndCompleteAllLevelsInCampaign);
#if COMPILE_AUDIO_DEBUG_RENDERING_CODE
		REGISTER_CHEAT("debug_sound", SpaceArcadeCheatSystem::cheat_debugSound);
#endif //COMPILE_AUDIO_DEBUG_RENDERING_CODE
		REGISTER_CHEAT("debug_sound_log_dump", SpaceArcadeCheatSystem::cheat_debugSound_logDump);
		REGISTER_CHEAT("infinite_slowmo", SpaceArcadeCheatSystem::cheat_infiniteTimeDilation);
		REGISTER_CHEAT("toggle_star_jump", SpaceArcadeCheatSystem::cheat_toggleStarJump);
	}

	void SpaceArcadeCheatSystem::cheat_oneShotObjectives(const std::vector<std::string>& cheatArgs)
	{
		oneShotShipObjectivesCheat.broadcast();
	}

	void SpaceArcadeCheatSystem::cheat_destroyAllObjectives(const std::vector<std::string>& cheatArgs)
	{
		destroyAllShipObjectivesCheat.broadcast();
	}

	void SpaceArcadeCheatSystem::cheat_turretsTargetPlayer(const std::vector<std::string>& cheatArgs)
	{
		turretsTargetPlayerCheat.broadcast();
	}

	void SpaceArcadeCheatSystem::cheat_commsTargetPlayer(const std::vector<std::string>& cheatArgs)
	{
		commsTargetPlayerCheat.broadcast();
	}

	void SpaceArcadeCheatSystem::cheat_destroyAllGenerators(const std::vector<std::string>& cheatArgs)
	{
		destroyAllGeneratorsCheat.broadcast();
	}

	void SpaceArcadeCheatSystem::cheat_killPlayer(const std::vector<std::string>& cheatArgs)
	{
#if COMPILE_CHEATS
		if (const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0))
		{
			if (IControllable* controlTarget = player->getControlTarget())
			{
				if (GameplayComponentEntity* componentEntity = dynamic_cast<GameplayComponentEntity*>(controlTarget))
				{
					if (HitPointComponent* hpComp = componentEntity->getGameComponent<HitPointComponent>())
					{
						//take away a lot of health, hopefully enough to bypass any dampening effects
						hpComp->adjust(-(hpComp->getHP().max * 10.f));
					}
				}
			}
		}
#endif
	}

	void SpaceArcadeCheatSystem::cheat_make_json_template_spacelevelconfig(const std::vector<std::string>& cheatArgs)
	{
#if COMPILE_CHEATS
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			sp<SpaceLevelConfig> spaceConfig = new_sp<SpaceLevelConfig>();
			spaceConfig->applyDemoDataIfEmpty();

			spaceConfig->save();

			std::string representativeFilePath = spaceConfig->getRepresentativeFilePath();
			sp<ConfigBase> baseConfig = ConfigBase::load(representativeFilePath, []() {return new_sp<SpaceLevelConfig>(); });
			if (SpaceLevelConfig* reloadedConfig = dynamic_cast<SpaceLevelConfig*>(baseConfig.get()))
			{
				log(__FUNCTION__, LogLevel::LOG, "loaded template config just saved");	//use debug to inspect values
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_WARNING, "failed to load template config just saved");
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get active mod");
		}
#endif
	}

	void SpaceArcadeCheatSystem::cheat_make_json_template_savegame(const std::vector<std::string>& cheatArgs)
	{
#if COMPILE_CHEATS
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			sp<SaveGameConfig> saveGameConfig = new_sp<SaveGameConfig>();
			saveGameConfig->applyDemoDataIfEmpty();
			saveGameConfig->save();

			std::string representativeFilePath = saveGameConfig->getRepresentativeFilePath();
			sp<ConfigBase> baseConfig = ConfigBase::load(representativeFilePath, []() {return new_sp<SaveGameConfig>(); });
			if (SaveGameConfig* reloadedConfig = dynamic_cast<SaveGameConfig*>(baseConfig.get()))
			{
				log(__FUNCTION__, LogLevel::LOG, "loaded template config just saved");	//use debug to inspect values
			}
			else
			{
				log(__FUNCTION__, LogLevel::LOG_WARNING, "failed to load template config just saved");
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Could not get active mod");
		}
#endif
	}

	void SpaceArcadeCheatSystem::cheat_mainMenuTransitionTest(const std::vector<std::string>& cheatArgs)
	{
#if COMPILE_CHEATS
		LevelSystem& levelSystem = SpaceArcade::get().getLevelSystem();
		if (const sp<LevelBase>& currentLevel = levelSystem.getCurrentLevel())
		{
			if (SpaceLevelBase* spaceLevel = dynamic_cast<SpaceLevelBase*>(currentLevel.get()))
			{
				EndGameParameters endP;
				endP.delayTransitionMainmenuSec = 0.1f;
				spaceLevel->endGame(endP);
			}
		}
		else
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "No current level");
			STOP_DEBUGGER_HERE();
		}
#endif
	}

	void SpaceArcadeCheatSystem::cheat_unlockAndCompleteAllLevelsInCampaign(const std::vector<std::string>& cheatArgs)
	{
#if COMPILE_CHEATS
		if (const sp<Mod>& activeMod = SpaceArcade::get().getModSystem()->getActiveMod())
		{
			size_t activeCampaignIdx = 0;
			try {activeCampaignIdx = cheatArgs.size() > 1 ? std::stoi(cheatArgs[1]) : activeCampaignIdx;} //first arg is cheat, second is arg
			catch (std::exception& ) { log(__FUNCTION__, LogLevel::LOG_ERROR, "First cheat arg is not a valid index"); }

			sp<CampaignConfig> campaign = activeMod->getCampaign(activeCampaignIdx);
			sp<SaveGameConfig> saveGameConfig = activeMod->getSaveGameConfig();
			if (campaign && saveGameConfig)
			{
				//intentionally not saving so this cheat can be used without changing underlying file
				std::string campaignFilePath = campaign->getRepresentativeFilePath();
				SaveGameConfig::CampaignData* savedData = saveGameConfig->findCampaignByName_Mutable(campaignFilePath);
				if (!savedData)
				{
					saveGameConfig->addCampaign(campaignFilePath);
					savedData = saveGameConfig->findCampaignByName_Mutable(campaignFilePath);
				}

				if (savedData)
				{
					const std::vector<CampaignConfig::LevelData>& levels = campaign->getLevels();
					for (size_t levelIdx = 0; levelIdx < levels.size(); ++levelIdx)
					{
						savedData->completedLevels.push_back(levelIdx);
					}
				}
				else
				{
					STOP_DEBUGGER_HERE(); //we should have made a config... what happened?
				}
			}
			else
			{
				STOP_DEBUGGER_HERE(); //no campaign or save game config?
			}
		}
	}

	void SpaceArcadeCheatSystem::cheat_debugSound(const std::vector<std::string>& cheatArgs)
	{
#if COMPILE_AUDIO_DEBUG_RENDERING_CODE
		AudioSystem& audioSystem = GameBase::get().getAudioSystem();
		audioSystem.bRenderSoundLocations = !audioSystem.bRenderSoundLocations;
#endif //COMPILE_AUDIO_DEBUG_RENDERING_CODE
	}

	void SpaceArcadeCheatSystem::cheat_debugSound_logDump(const std::vector<std::string>& cheatArgs)
	{
		GameBase::get().getAudioSystem().logDebugInformation();
	}

	void SpaceArcadeCheatSystem::cheat_infiniteTimeDilation(const std::vector<std::string>& cheatArgs)
	{
		if (const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0))
		{
			if (SAPlayer* shipPlayerCheat = dynamic_cast<SAPlayer*>(player.get()))
			{
				shipPlayerCheat->cheat_infiniteTimeDilation();
			}
		}
	}

	void SpaceArcadeCheatSystem::cheat_toggleStarJump(const std::vector<std::string>& cheatArgs)
	{
		if (const sp<LevelBase>& currentLevel = GameBase::get().getLevelSystem().getCurrentLevel())
		{
			if (SpaceLevelBase* level = dynamic_cast<SpaceLevelBase*>(currentLevel.get()))
			{
				level->enableStarJump(!level->isStarJumping());
			}
		}
	}

#endif

	void CheatStatics::givePlayerQuaternionCamera()
	{
		const sp<PlayerBase>& player = GameBase::get().getPlayerSystem().getPlayer(0);
		const sp<Window>& primaryWindow = GameBase::get().getWindowSystem().getPrimaryWindow();
		if (player && primaryWindow)
		{
			vec3 oldCamPos = vec3(0.f);
			quat oldCamQuat = quat(1.f, 0, 0, 0);
			if (const sp<CameraBase>& camera = player->getCamera())
			{
				oldCamPos = camera->getPosition();
				oldCamQuat = camera->getQuat(); //fps camera has conversion functions
			}

			sp<QuaternionCamera> newCamera = new_sp<QuaternionCamera>();
			newCamera->setPosition(oldCamPos);
			newCamera->setQuat(oldCamQuat);
			newCamera->registerToWindowCallbacks_v(primaryWindow);
			player->setCamera(newCamera);
		}
	}

}
