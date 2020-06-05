#pragma once
#include <string>
#include <vector>
#include "../../../../GameFramework/Interfaces/SATickable.h"
#include "../../../../GameFramework/SAGameEntity.h"
#include "../../../../Tools/DataStructures/SATransform.h"
#include "DigitalClockFont.h"

namespace SA
{
	class DigitalClockGlyph;
	class DigitalClockFont;
	class Shader;
	class RNG;

	sp<Shader> getGlitchTextShader();

	////////////////////////////////////////////////////////
	// GlitchTextFont
	////////////////////////////////////////////////////////
	class GlitchTextFont : public DigitalClockFont, public ITickable
	{
	public:
		/** Shader assumes structure similar to glitch text shader*/
		GlitchTextFont(const DigitalClockFont::Data& init);
		virtual void preIndividualGlyphRender(size_t idx, Shader& shader) override;
		virtual void onGlyphCacheRebuilt(const DigitalClockFont::GlyphCalculationCache& data) override;
		void resetAnim();
		void setAnimPlayForward(bool bForward);
		void play(bool bPlay) { bPlaying = bPlay; }
		void setSpeedScale(float newScale) { speedScale = newScale; }
		bool isPlaying() const { return bPlaying; }
	public:
		virtual bool tick(float dt_sec) override;
	private:
		const size_t baseDisplacedBits = 30; 
		struct GlyphAnimData
		{
			int32_t bitvecOffset;
			int32_t shiftVal = 0;
			float updateSec = 0.1f; //this is overwritten with random change
			float timeAccum = 0.f;
		};
		std::vector<GlyphAnimData> animData;
		bool bPlaying = true;
		bool bForwardDirection = true;
		float speedScale = 1.0;
		sp<RNG> rng;
	};

}
