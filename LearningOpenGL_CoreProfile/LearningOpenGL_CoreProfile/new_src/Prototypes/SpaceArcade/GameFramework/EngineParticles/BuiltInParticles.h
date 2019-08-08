#pragma once

#include <unordered_map>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "../SAGameEntity.h"
#include "../SAParticleSystem.h"
#include "../../Tools/RemoveSpecialMemberFunctionUtils.h"

namespace SA
{
	class ParticleConfig;
	class Model3D;
	
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

		private:
			//cache colored configs
			std::unordered_multimap<size_t, sp<ShieldParticleConfig>> modelToParticleHashMap;
		};
	}
}

