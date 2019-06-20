#include "SALevel.h"

namespace SA
{

	LevelBase::~LevelBase()
	{
		//make sure that the collision entities are cleared before we destroy the hash map.
		worldEntities.clear();
		renderEntities.clear();
	}

	void LevelBase::startLevel()
	{
		//base behavior should go here, before the subclass callbacks

		//start subclass specific behavior after base behavior (ctor/dtor pattern)
		startLevel_v();
	}

	void LevelBase::endLevel()
	{
		//end subclass specific behavior before base behavior (ctor/dtor pattern)
		endLevel_v();

		//clear out the spawned entities so they are cleaned up before the spatial hash is cleaned up
		worldEntities.clear();
		renderEntities.clear();
	}

	void LevelBase::startLevel_v()
	{
	}

	void LevelBase::endLevel_v()
	{
	}

	void LevelBase::tick(float dt_sec)
	{
		float dialated_dt_sec = dt_sec * timeDialationFactor;

		for (const sp<WorldEntity>& entity : worldEntities)
		{
			entity->tick(dialated_dt_sec);
		}
	}

}