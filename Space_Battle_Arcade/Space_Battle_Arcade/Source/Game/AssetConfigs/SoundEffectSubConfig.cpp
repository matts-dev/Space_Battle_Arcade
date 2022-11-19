#include "SoundEffectSubConfig.h"
#include "JsonUtils.h"
#include "GameFramework/SAAudioSystem.h"
#include "Game/GameSystems/SAModSystem.h"

namespace SA
{
	void SoundEffectSubConfig::onSerialize(nlohmann::json& outData)
	{
		outData[SYMBOL_TO_STR(assetPath)] = assetPath;
		outData[SYMBOL_TO_STR(bLooping)] = bLooping;
		outData[SYMBOL_TO_STR(maxDistance)] = maxDistance;
		outData[SYMBOL_TO_STR(gain)] = gain;
		outData[SYMBOL_TO_STR(pitchVariationRange)] = pitchVariationRange;
	}

	void SoundEffectSubConfig::onDeserialize(const nlohmann::json& inData)
	{
		READ_JSON_STRING_OPTIONAL(assetPath, inData);
		READ_JSON_BOOL_OPTIONAL(bLooping, inData);
		READ_JSON_FLOAT_OPTIONAL(maxDistance, inData);
		READ_JSON_FLOAT_OPTIONAL(pitchVariationRange, inData);
		READ_JSON_FLOAT_OPTIONAL(gain, inData);
	}

	void SoundEffectSubConfig::configureEmitter(const sp<AudioEmitter>& emitter) const
	{
		if (assetPath.size() > 0)
		{
			emitter->setSoundAssetPath(convertModRelativePathToAbsolute(assetPath));
			emitter->setLooping(bLooping);
			emitter->setMaxRadius(maxDistance);
			emitter->setOneShotPlayTimeout(oneshotTimeoutSec);
			emitter->setPitchVariationRange(pitchVariationRange);
			emitter->setGain(gain);
		}
	}

}

