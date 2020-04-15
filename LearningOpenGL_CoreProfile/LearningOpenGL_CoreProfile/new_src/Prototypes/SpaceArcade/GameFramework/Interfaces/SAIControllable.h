#pragma once

#include "../SAGameEntity.h"

namespace SA
{
	class CameraBase;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// A class that the player can take control of.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class IControllable
	{
	public:
		virtual void onPlayerControlTaken() = 0;
		virtual void onPlayerControlReleased() = 0;
		virtual sp<CameraBase> getCamera() = 0;

		/** Because most systems require entities, this virtual is provided to avoid dynamic casting. return nullptr if inheritee is not an entity */
		virtual GameEntity* asEntity() = 0;
	};
}
