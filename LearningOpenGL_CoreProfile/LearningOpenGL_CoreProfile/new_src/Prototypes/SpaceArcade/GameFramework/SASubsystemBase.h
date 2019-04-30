#pragma once
#include "SAGameEntity.h"
#include "..\Tools\RemoveSpecialMemberFunctionUtils.h"


namespace SA
{
	class GameBase;

	/**
		INVARIANT: No subsystem shall attempt to retrieve another subsystem within its constructor

	 */
	class SubsystemBase : public GameEntity, public RemoveCopies, public RemoveMoves
	{
	public:

	private:
		friend GameBase;
		virtual void tick(float deltaSec) = 0;

		/** Called when main game subsystems can safely be accessed; though not all may be initialized */
		virtual void initSystem() {};
		virtual void handlePostRender() {}
	};
}