#pragma once
#include "..\GameFramework\SASubsystemBase.h"
#include "..\Tools\DataStructures\MultiDelegate.h"
//#include "SALevel.h"

namespace SA
{
	class Level;

	class LevelSubsystem : public SubsystemBase
	{
	public:
		/** Broadcasts just before level is changed */
		MultiDelegate<const sp<Level>& /*currentLevel*/, const sp<Level>& /*newLevel*/> onPreLevelChange;

		/** Broadcasts just after level has changed */
		MultiDelegate<const sp<Level>& /*previousLevel*/, const sp<Level>& /*newCurrentLevel*/> onPostLevelChange;

		/** Broadcasts when level start up is finished*/
		MultiDelegate<const sp<Level>& /*currentLevel*/> onLevelStartupComplete;

	public:
		void loadLevel(sp<Level>& newLevel);
		void unloadLevel(sp<Level>& level);
		inline const sp<Level>& getCurrentLevel() { return loadedLevel; }
		
	private:
		virtual void tick(float deltaSec);
		virtual void shutdown() override;

	private:
		sp<Level> loadedLevel = nullptr;
	};


}