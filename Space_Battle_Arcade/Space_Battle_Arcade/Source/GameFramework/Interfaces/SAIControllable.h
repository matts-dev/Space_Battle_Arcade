#pragma once

#include "../SAGameEntity.h"

namespace SA
{
	class CameraBase;
	class PlayerBase;
	class WorldEntity;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A class that the player can take control of.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class IControllable
	{
	public:
		virtual void onPlayerControlTaken(const sp<PlayerBase>& player) = 0;
		virtual void onPlayerControlReleased() = 0;
		virtual sp<CameraBase> getCamera() = 0;

		/** Because most systems require entities, this virtual is provided to avoid dynamic casting. return nullptr if inheritee is not an entity 
		    Furthermore, it doesn't make sense for player to control something that doesn't have physical presence in world. So this is a world entity
			This is very useful because it allows quick access to world transform for positional relevancy calculations*/
		virtual WorldEntity* asWorldEntity() = 0;
	};
}
