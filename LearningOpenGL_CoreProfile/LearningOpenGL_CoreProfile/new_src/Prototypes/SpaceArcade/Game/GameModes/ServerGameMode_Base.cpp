#include "ServerGameMode_Base.h"
#include "../Levels/SASpaceLevelBase.h"
#include "../../Tools/PlatformUtils.h"

namespace SA
{

	void ServerGameMode_Base::setOwningLevel(const sp<SpaceLevelBase>& level)
	{
		owningLevel = level;
	}


	void ServerGameMode_Base::initialize(const InitKey& key)
	{
		sp<SpaceLevelBase> level = owningLevel.expired() ? nullptr : owningLevel.lock();
		onInitialize(level);

		if (!bBaseInitialized) //runtime checking if super class method was called
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Gamemode did not call super OnInitialize - shutting down");
			STOP_DEBUGGER_HERE();
		}

	}

	void ServerGameMode_Base::onInitialize(const sp<SpaceLevelBase>& level)
	{
		bBaseInitialized = true;
	}

	void ServerGameMode_Base::endGame(const EndGameParameters& endParameters)
	{
		//game mode acts on level because if client/server architecture is ever set up, clients will have access to level but not gamemode
		if (sp<SpaceLevelBase> level = owningLevel.lock())
		{
			level->endGame(endParameters);
		}
		else
		{
			STOP_DEBUGGER_HERE(); //no level, how did this happen?
		}
	}

}

