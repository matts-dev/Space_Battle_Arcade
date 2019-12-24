#include "SAGameEntity.h"
#include "..\Tools\DataStructures\MultiDelegate.h"

namespace //local to translation units; 
{
	using namespace SA;

	struct StaticImpl
	{
		StaticImpl()
		{
			pendingDestroy.reserve(5000);
		}

		std::vector<sp<GameEntity>> pendingDestroy;

	} staticImplementation;


}


namespace SA
{


	GameEntity::GameEntity() 
		: onDestroyedEvent(new_sp<MultiDelegate<const sp<GameEntity>&>>())
		, onLifetimeOverEvent(new_sp<MultiDelegate<>>())
	{
	}

	void GameEntity::destroy()
	{
		onDestroyed();
		bPendingDestroy = true;

		//broadcast of destroyed needs to be delayed to next tick so that sp doesn't call dtor within during member function call
		// eg:
		//	 1. member detects projectile hit, 
		//	 2. member calls destroy, 
		//	 3. level is listening to destroy and cleans up only reference,
		//	 4. call stack frame returns to member function, object attempts to do final clean up on deleted memory
		// deferring destroy means call stack should not have any game entity functions on call stack during destroy
		staticImplementation.pendingDestroy.push_back(sp_this());
	}

	void GameEntity::onDestroyed()
	{
	}

	void GameEntity::cleanupPendingDestroy(CleanKey)
	{
		//#TODO perhaps this should be registered as a ticker rather than being called directly by engine? but need to be able to tick static functions? new delegate feature?
		//#TODO with static ticker, there's not need for cleanup key -- so it can be removed
		for (sp<GameEntity>& entity : staticImplementation.pendingDestroy)
		{
			//lifetime over event happens separately so that no race condition will exist on the onDestroyedEvent. 
			//By separating events, all life time pointers will be cleared before the destroyed events happen.
			entity->onLifetimeOverEvent->broadcast();
			entity->onDestroyedEvent->broadcast(entity);
		}

		//remove all sp references -- this will probably be the last reference for many properly managed entities. meaning a large dtor clean up
		//this may need to be amortized over multiple frames (in a separate datastrcuture) if this causes performance hitches 
		//when a large number of entiteis are destroyed
		staticImplementation.pendingDestroy.clear();
	}

}


