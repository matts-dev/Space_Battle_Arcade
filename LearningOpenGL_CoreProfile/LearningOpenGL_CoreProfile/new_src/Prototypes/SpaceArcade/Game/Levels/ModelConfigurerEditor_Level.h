#pragma once
#include "..\..\GameFramework\SALevel.h"

namespace SA
{
	class PlayerBase;

	class ModelConfigurerEditor_Level : public LevelBase
	{
	protected:
		virtual void startLevel_v() override;
		virtual void endLevel_v() override;
	private:
		void handleUIFrameStarted();
		void handlePlayerCreated(const sp<PlayerBase>& player, uint32_t playerIdx);
		void handleKey(int key, int state, int modifier_keys, int scancode);
		void handleMouseButton(int button, int state, int modifier_keys);
	private:
	};

}