#pragma once
#include "GameFramework/CheatSystemBase.h"


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
		void cheat_make_json_template_campaign(const std::vector<std::string>& cheatArgs);
		void cheat_killPlayer(const std::vector<std::string>& cheatArgs);
		void cheat_make_json_template_spacelevelconfig(const std::vector<std::string>& cheatArgs);
		void cheat_make_json_template_savegame(const std::vector<std::string>& cheatArgs);
		void cheat_mainMenuTransitionTest(const std::vector<std::string>& cheatArgs);
		void cheat_unlockAndCompleteAllLevelsInCampaign(const std::vector<std::string>& cheatArgs);
		void cheat_debugSound(const std::vector<std::string>& cheatArgs);
		void cheat_debugSound_logDump(const std::vector<std::string>& cheatArgs);
		void cheat_infiniteTimeDilation(const std::vector<std::string>& cheatArgs);
		void cheat_toggleStarJump(const std::vector<std::string>& cheatArgs);
		void cheat_toggleInvincible(const std::vector<std::string>& cheatArgs);
	};


	class CheatStatics
	{
	public:
		static void givePlayerQuaternionCamera();
	};

}