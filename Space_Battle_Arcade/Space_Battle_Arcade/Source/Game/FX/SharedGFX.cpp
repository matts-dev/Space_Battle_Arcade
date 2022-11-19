#include "SharedGFX.h"
#include "GameFramework/SALevelSystem.h"
#include "GameFramework/SAGameBase.h"

namespace SA
{

	void SharedGFX::postConstruct()
	{
		LevelSystem& levelSystem = GameBase::get().getLevelSystem();
		levelSystem.onPreLevelChange.addWeakObj(sp_this(), &SharedGFX::handlePreLevelChange);
	}

	void SharedGFX::handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel)
	{
		shieldEffects_ModelToFX->resetCache();
	}

	//const sp<SA::ShieldEffect::ParticleCache> SharedGFX::shieldEffects_ModelToFX = new_sp<ShieldEffect::ParticleCache>();

	SharedGFX& SharedGFX::get()
	{
		//causing creation to happen in a function means that engine will be available when we create this and it can hook into events (like level transfer).
		static sp<SharedGFX> fx = new_sp<SharedGFX>();
		return *fx;
	}

}

