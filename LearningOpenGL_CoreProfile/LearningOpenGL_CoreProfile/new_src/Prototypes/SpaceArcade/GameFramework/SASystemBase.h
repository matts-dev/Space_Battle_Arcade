#pragma once
#include "SAGameEntity.h"
#include "..\Tools\RemoveSpecialMemberFunctionUtils.h"


namespace SA
{
	class GameBase;

	/**
		INVARIANT: No system shall attempt to retrieve another system within its constructor; handle that in "initSystem" virtuals
	 */
	class SystemBase : public GameEntity, public RemoveCopies, public RemoveMoves
	{
	public:

	private:
		friend GameBase; //driver of virtual functions

		virtual void tick(float deltaSec){};

		/** Called when main game systems can safely be accessed; though not all may be initialized */
		virtual void initSystem() {};

		/** Called when game is shuting down for resource releasing; resources that require other systems should prefer releasing here rather than the dtor of the system */
		virtual void shutdown() {}
		virtual void handlePostRender() {}
	};
}