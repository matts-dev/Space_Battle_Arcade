#pragma once

#include <unordered_map>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "GameFramework/SAGameEntity.h"
#include "GameFramework/SAParticleSystem.h"
#include "Tools/RemoveSpecialMemberFunctionUtils.h"

namespace SA
{
	class ParticleConfig;
	class Model3D;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// shield effect (eg when something takes damage
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	namespace ShieldEffect
	{
		using byte = unsigned char;

		//helper class for doing hash-based cache lookups.
		class ShieldParticleConfig : public ParticleConfig
		{
		public:
			ShieldParticleConfig(std::vector<sp<Particle::Effect>>&& effectsToMove) : ParticleConfig(std::move(effectsToMove)) {}
			friend class ParticleCache;
		private:
			sp<Model3D> model;
			byte colorBytes[3];
		};

		//spawns sheild particles; caches particles so that future requests are rendered with same instance batch
		class ParticleCache : public GameEntity, public RemoveCopies, public RemoveMoves
		{
		public:
			//interally caches effects model and color combinations; managed by weak pointers. 
			sp<ParticleConfig> getEffect(const sp<Model3D>& model, const glm::vec3& color);
			void resetCache();
		private:
			//cache colored configs
			std::unordered_multimap<size_t, sp<ShieldParticleConfig>> modelToParticleHashMap;
		};
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// engine effect (ie the fire behind a ship)
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class EngineParticleEffectConfig : public ParticleConfig
	{
		friend class EngineParticleCache;
	public:
		EngineParticleEffectConfig(std::vector<sp<Particle::Effect>>&& effectsToMove) : ParticleConfig(std::move(effectsToMove)) 
		{ 
			bLoop = true;
		}
	public:
		const glm::vec3& getColor() { return color; }
		const float getHdrIntensity() { return colorHdrIntensity; }
	private:
		glm::vec3 color;
		float colorHdrIntensity;
	};

	//find an engine particle (so it it can leverage instancing) based on color
	class EngineParticleCache
	{
	public:
		sp<EngineParticleEffectConfig> getParticle(const struct EngineEffectData& fxData);
		void resetCache(); //#todo really need to make a generic version of this, but going fast and I think this is last particle for this game
	private:
		std::unordered_multimap</*hash*/size_t, sp<EngineParticleEffectConfig>> coloredParticles;
	};
}

