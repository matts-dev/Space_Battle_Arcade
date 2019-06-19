#include "SALevelSubsystem.h"
#include "SALevel.h"
#include <iostream>

namespace SA
{
	void LevelSubsystem::loadLevel(sp<LevelBase>& newLevel)
	{
		if (newLevel)
		{
			onPreLevelChange.broadcast(loadedLevel, newLevel);

			sp<LevelBase> previousLevel = loadedLevel;
			unloadLevel(previousLevel);
			loadedLevel = newLevel;

			//loading the level may eventually require asset loading and will need to be asynchronous 
			//for now, it is okay for it to be within the same thread since it is mostly just a 
			//spawn configuration and data structure. but if larger assets are ever loaded on demand,
			//the job should be done with a worker thread and broadcast when it is done
			//onLevelLoaded.broadcast(loadedLevel);
			loadedLevel->startLevel();
			onPostLevelChange.broadcast(previousLevel, newLevel);
		}
	}

	void LevelSubsystem::unloadLevel(sp<LevelBase>& level)
	{
		if (level == loadedLevel)
		{
			if (level)
			{
				level->endLevel();
				loadedLevel = nullptr;
			}
		}
		else
		{
			std::cerr << "LEVEL SUBSYSTEM: attempting to unload level that is not current level" << std::endl;
		}
	}

	void LevelSubsystem::tick(float deltaSec)
	{
		if (loadedLevel)
		{
			loadedLevel->tick(deltaSec);
		}
	}

	void LevelSubsystem::shutdown()
	{
		if (loadedLevel)
		{
			loadedLevel = nullptr;
		}
	}

}
