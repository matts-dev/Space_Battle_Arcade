#include "SALevel.h"
#include "SAGameBase.h"

namespace SA
{

	LevelBase::LevelBase()
	{
		//beware, some lone object may keep the level alive. Relying on ctor/dtor RAII becomes dangerous
		//I hope to make special pointers that set themselves to null when marked destroy, but until then
		//be very careful about accessing/destroying resources from gamebase in ctor/dtor
	}

	LevelBase::~LevelBase()
	{
		//beware, some lone object may keep the level alive. Relying on ctor/dtor RAII becomes dangerous
		//I hope to make special pointers that set themselves to null when marked destroy, but until then
		//be very careful about accessing/destroying resources from gamebase in ctor/dtor

		//make sure that the collision entities are cleared before we destroy the hash map.

		if (bLevelStarted)
		{
			endLevel();
		}

	}

	void LevelBase::startLevel()
	{
		//base behavior should go here, before the subclass callbacks
		bLevelStarted = true;
		worldTimeManager = GameBase::get().getTimeSystem().createManager();

		//start subclass specific behavior after base behavior (ctor/dtor pattern)
		startLevel_v();
	}

	void LevelBase::endLevel()
	{
		//end subclass specific behavior before base behavior (ctor/dtor pattern)
		endLevel_v();

		bLevelStarted = false;

		//clear out the spawned entities so they are cleaned up before the spatial hash is cleaned up
		worldEntities.clear();
		renderEntities.clear();

		GameBase::get().getTimeSystem().destroyManager(worldTimeManager);
	}

	void LevelBase::startLevel_v()
	{
	}

	void LevelBase::endLevel_v()
	{
	}

	void LevelBase::onEntitySpawned_v(const sp<WorldEntity>& spawned)
	{
	}

	void LevelBase::onEntityUnspawned_v(const sp<WorldEntity>& unspawned)
	{
	}

	void LevelBase::tick(float dt_sec)
	{
		float dialated_dt_sec = worldTimeManager->getDeltaTimeSecs();

		if (!worldTimeManager->isTimeFrozen())
		{
			for (const sp<WorldEntity>& entity : worldEntities)
			{
				entity->tick(dialated_dt_sec);
			}
		}
	}

}