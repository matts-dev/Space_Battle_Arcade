#pragma once
#include "../GameFramework/SAGameEntity.h"
#include "../GameFramework/SAPlayerBase.h"

namespace SA
{
	class Player : public PlayerBase
	{
	public:
		virtual ~Player() {}
		virtual sp<CameraBase> generateDefaultCamera() const;
	};
}

