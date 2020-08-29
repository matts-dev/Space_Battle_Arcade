#pragma once
#include <string>
#include "../../../../../Libraries/nlohmann/json.hpp"
#include "../../GameFramework/SAGameEntity.h"
#include <optional>

namespace SA
{
	struct SoundEffectSubConfig
	{
	public:
		std::string assetPath = "";
		bool bLooping = false;
		float maxDistance = 10.f;
		float pitchVariationRange = 0.f;
		std::optional<float> oneshotTimeoutSec;

	public:
		virtual void onSerialize(nlohmann::json& outData);
		virtual void onDeserialize(const nlohmann::json& inData);
		void configureEmitter(const sp<class AudioEmitter>& emitter) const;
	};
}