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
	protected:
		virtual void postConstruct() override;
	private:
		void cheat_oneShotObjectives(const std::vector<std::string>& cheatArgs);
		void cheat_destroyAllObjectives(const std::vector<std::string>& cheatArgs);
	};
}