#include "SALevel.h"
#include "SAGameBase.h"
#include "SALog.h"

namespace SA
{

	LevelBase::LevelBase(const LevelInitializer& init/* = {}*/)
		: worldCollisionGrid{init.worldGridSize}
		//: worldCollisionGrid{ glm::vec3(64.f) } //debug -- change to this to quickly change level defaults without recompiling everything
		//: worldCollisionGrid{ glm::vec3(32.f) } //debug -- change to this to quickly change level defaults without recompiling everything
		//: worldCollisionGrid{ glm::vec3(16.f) } //debug -- change to this to quickly change level defaults without recompiling everything
		//: worldCollisionGrid{ glm::vec3(8.f) } //debug -- change to this to quickly change level defaults without recompiling everything
	{
		//beware, some lone object may keep the level alive. Relying on ctor/dtor RAII becomes dangerous
		//I hope to make special pointers that set themselves to null when marked destroy, but until then
		//be very careful about accessing/destroying resources from gamebase in ctor/dtor

		worldTimeManager = GameBase::get().getTimeSystem().createManager();
	}

	LevelBase::~LevelBase()
	{
		//beware, some lone object may keep the level alive. Relying on ctor/dtor RAII becomes dangerous
		//I hope to make special pointers that set themselves to null when marked destroy, but until then
		//be very careful about accessing/destroying resources from gamebase in ctor/dtor

		if (bLevelActive)
		{
			//Do not call endLevel here because it calls a virtual; which will not invoke subclass behavior as it has subclass portion has already been destroyed
			log("LevelBase", LogLevel::LOG_ERROR, "Level was active during dtor; this should have been shutdown by level system.");
			assert(false);
		}

	}

	void LevelBase::startLevel()
	{
		//base behavior should go here, before the subclass callbacks
		bLevelActive = true;

		//start subclass specific behavior after base behavior (ctor/dtor pattern)
		startLevel_v();
	}

	void LevelBase::endLevel()
	{
		//end subclass specific behavior before base behavior (ctor/dtor pattern)
		//reminder: if this is being called by a dtor the virtual will not be a subclass function call
		endLevel_v();

		bLevelActive = false;

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
		float dilated_dt_sec = worldTimeManager->getDeltaTimeSecs();

		if (!worldTimeManager->isTimeFrozen())
		{
			for (const sp<WorldEntity>& entity : worldEntities)
			{
				entity->tick(dilated_dt_sec);
			}
		}
	}

}