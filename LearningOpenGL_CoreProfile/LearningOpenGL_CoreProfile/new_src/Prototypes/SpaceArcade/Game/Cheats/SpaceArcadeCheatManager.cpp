#include "SpaceArcadeCheatManager.h"

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
		REGISTER_CHEAT("turrets_target_player", SpaceArcadeCheatSystem::cheat_turretsTargetPlayer);
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

}
