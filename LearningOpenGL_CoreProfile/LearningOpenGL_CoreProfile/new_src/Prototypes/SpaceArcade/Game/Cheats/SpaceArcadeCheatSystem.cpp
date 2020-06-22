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

}
