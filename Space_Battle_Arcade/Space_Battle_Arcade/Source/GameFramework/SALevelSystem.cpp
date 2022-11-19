#include "GameFramework/SALevelSystem.h"
#include "GameFramework/SALevel.h"
#include <iostream>
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SATimeManagementSystem.h"

namespace SA
{
	LevelSystem::~LevelSystem()
	{
		unloadLevel(loadedLevel);
	}

	void LevelSystem::loadLevel(sp<LevelBase>& newLevel)
	{
		//changing levels during level tick or updating time will null out things that are in flight, we need to do this at a safe boundary.
		if (bTickingLevel || GameBase::get().getTimeSystem().isUpdatingTime())
		{
			deferredLevelChange = newLevel;
		}
		else
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

				//warning on post level change certain parts of level will be unloaded (eg time manager) prefer prelevel change for subscription modification
				onPostLevelChange.broadcast(previousLevel, newLevel); 
			}
		}
	}

	void LevelSystem::unloadLevel(sp<LevelBase>& level)
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
			std::cerr << "LEVEL SYSTEM: attempting to unload level that is not current level" << std::endl;
		}
	}

	void LevelSystem::tick(float deltaSec)
	{
		if (loadedLevel)
		{
			bTickingLevel = true;
			loadedLevel->tick(deltaSec);
			bTickingLevel = false;

			//if a level change request happened during tick, then we stage it and wait for level tick to complete.
			if (deferredLevelChange)
			{
				//may need to get more clever about when this level changes.
				loadLevel(deferredLevelChange);
				deferredLevelChange = nullptr;
			}
		}
	}

	void LevelSystem::shutdown()
	{
		if (loadedLevel)
		{
			unloadLevel(loadedLevel);
		}
	}

}
