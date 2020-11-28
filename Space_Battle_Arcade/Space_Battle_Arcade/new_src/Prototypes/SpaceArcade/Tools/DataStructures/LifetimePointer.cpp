#include "LifetimePointer.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SATimeManagementSystem.h"

namespace SA
{
	FrameDeferredEntityDeleter& FrameDeferredEntityDeleter::get()
	{
		static sp<FrameDeferredEntityDeleter> theDeleter = new_sp<FrameDeferredEntityDeleter>();
		return *theDeleter;
	}

	void FrameDeferredEntityDeleter::deleteLater(const sp<const GameEntity>& entity)
	{
		pendingDelete.push_back(entity);
	}

	void FrameDeferredEntityDeleter::postConstruct()
	{
		pendingDelete.reserve(10);

		//if the engine has shut down, any memory should be cleaned up with cleaning up of file-scope static variables.
		if (!GameBase::isEngineShutdown())
		{
			GameBase& game = GameBase::get();
			game.getSystemTimeManager().registerTicker(sp_this());
		}
	}

	bool FrameDeferredEntityDeleter::tick(float dt_sec)
	{
		if (pendingDelete.size() > 0)
		{
			pendingDelete.clear();
		}
		return true;
	}
}

