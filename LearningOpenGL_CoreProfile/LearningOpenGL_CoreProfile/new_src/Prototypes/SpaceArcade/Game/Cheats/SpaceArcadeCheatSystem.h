#pragma once
#include "../../GameFramework/CheatSystemBase.h"


namespace SA
{
	class SpaceArcadeCheatSystem : public CheatSystemBase
	{
		using Parent = CheatSystemBase;
	public:
		MultiDelegate<> oneShotShipObjectivesCheat;
		MultiDelegate<> destroyAllShipObjectivesCheat;
		MultiDelegate<> destroyAllGeneratorsCheat;
		MultiDelegate<> turretsTargetPlayerCheat;
		MultiDelegate<> commsTargetPlayerCheat;
	protected:
		virtual void postConstruct() override;
	private:
		void cheat_oneShotObjectives(const std::vector<std::string>& cheatArgs);
		void cheat_destroyAllObjectives(const std::vector<std::string>& cheatArgs);
		void cheat_turretsTargetPlayer(const std::vector<std::string>& cheatArgs);
		void cheat_commsTargetPlayer(const std::vector<std::string>& cheatArgs);
		void cheat_destroyAllGenerators(const std::vector<std::string>& cheatArgs);
		void cheat_killPlayer(const std::vector<std::string>& cheatArgs);
	};
}