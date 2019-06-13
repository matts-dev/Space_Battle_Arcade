#include "SALevel.h"

namespace SA
{

	Level::~Level()
	{
		//make sure that the collision entities are cleared before we destroy the hash map.
		worldEntities.clear();
		renderEntities.clear();
	}

	void Level::startLevel()
	{
		//base behavior should go here, before the subclass callbacks

		//start subclass specific behavior after base behavior (ctor/dtor pattern)
		startLevel_v();
	}

	void Level::endLevel()
	{
		//end subclass specific behavior before base behavior (ctor/dtor pattern)
		endLevel_v();

		//clear out the spawned entities so they are cleaned up before the spatial hash is cleaned up
		worldEntities.clear();
		renderEntities.clear();
	}

	void Level::startLevel_v()
	{
	}

	void Level::endLevel_v()
	{
	}

	void Level::tick(float dt_sec)
	{
		for (const sp<WorldEntity>& entity : worldEntities)
		{
			entity->tick(dt_sec);
		}
	}

}