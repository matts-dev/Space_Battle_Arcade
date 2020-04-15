#include "SpaceArcadeCheatSystem.h"
#include "../../GameFramework/SAPlayerBase.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/Interfaces/SAIControllable.h"
#include "../../GameFramework/Components/SAComponentEntity.h"
#include "../../GameFramework/Components/GameplayComponents.h"
#include "../../GameFramework/SAPlayerSystem.h"

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

}
