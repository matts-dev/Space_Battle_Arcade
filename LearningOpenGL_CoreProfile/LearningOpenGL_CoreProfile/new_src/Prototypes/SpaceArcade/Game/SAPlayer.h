#pragma once
#include "../GameFramework/SAGameEntity.h"
#include "../GameFramework/SAPlayerBase.h"

namespace SA
{
	class Player : public PlayerBase
	{
	public:
		Player(int32_t playerIndex) : PlayerBase(playerIndex) {};
		virtual ~Player() {}
		virtual sp<CameraBase> generateDefaultCamera() const;
	};
}

