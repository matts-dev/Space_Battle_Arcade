#pragma once
#include "../../GameFramework/SAGameEntity.h"

#include "../../GameFramework/EngineParticles/BuiltInParticles.h"

namespace SA
{
	/** A location for instantiated GFX for use */
	class SharedGFX : public GameEntity
	{
	protected:
		virtual void postConstruct() override;
	private:
		void handlePreLevelChange(const sp<LevelBase>& currentLevel, const sp<LevelBase>& newLevel);
	public:
		static SharedGFX& get();

		const sp<ShieldEffect::ParticleCache> shieldEffects_ModelToFX = new_sp<ShieldEffect::ParticleCache>();
		const sp<EngineParticleCache> engineParticleCache = new_sp<EngineParticleCache>();
	};
}