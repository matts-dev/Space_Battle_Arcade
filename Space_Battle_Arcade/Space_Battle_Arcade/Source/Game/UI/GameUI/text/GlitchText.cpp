#include "GlitchText.h"
#include "DigitalClockFont.h"
#include "Rendering/SAShader.h"
#include "GameFramework/SAGameBase.h"
#include "GameFramework/SARandomNumberGenerationSystem.h"

namespace SA
{

	static const char*const  glitchText_vs = R"(
		#version 330 core
		layout (location = 0) in vec4 vertPos;				
		layout (location = 1) in vec2 vertUVs;				
		layout (location = 2) in int vertBitVec;				
				
		out vec2 uvs;
		out vec4 color;

		uniform mat4 glyphModel = mat4(1.f);
		uniform mat4 projection_view = mat4(1.f);
		uniform mat4 pivotMat = mat4(1.f);
		uniform mat4 parentModel = mat4(1.f);
		uniform vec4 uColor = vec4(1.f);

		uniform int bitVec = 0xFFFFFFFF; //replace this with an instanced attribute
		uniform int animOffset = 0; 

		void main()
		{
			//either create identity matrix or zero matrix to filter out digital clock segments
			int animatedBitVec = bitVec << animOffset;

			mat4 filterMatrix =	mat4((animatedBitVec & vertBitVec) > 0 ? 1.f : 0.f);		//mat4(1.f) == identity matrix

			gl_Position = projection_view * parentModel * pivotMat * glyphModel * filterMatrix * vertPos;
			uvs = vertUVs;
			color = uColor;
		}
	)";

	static const char*const glitchText_fs = R"(
		#version 330 core
		out vec4 fragmentColor;

		in vec4 color;

		void main()
		{
			fragmentColor = color;
		}
	)";

	////////////////////////////////////////////////////////
	// Glitch text font
	////////////////////////////////////////////////////////

	GlitchTextFont::GlitchTextFont(const DigitalClockFont::Data& init) : DigitalClockFont(init)
	{
		if (!init.shader)
		{
			setNewShader(getGlitchTextShader());
		}

		//buffer shader data!
		AdditionalGlyphSpacingFactor = 5.0f;
		rng = GameBase::get().getRNGSystem().getSeededRNG(13);
	}

	void GlitchTextFont::preIndividualGlyphRender(size_t idx, Shader& shader)
	{
		if (idx >= 0 && idx < animData.size())
		{
			GlyphAnimData& glyphData = animData[idx];
			shader.setUniform1i("animOffset", glyphData.shiftVal); 
		}
	}

	void GlitchTextFont::onGlyphCacheRebuilt(const DigitalClockFont::GlyphCalculationCache& data)
	{
		//animTime = 0.f;
		animData.reserve(data.bufferedChars);
		for (size_t glyph = 0; glyph < data.bufferedChars; ++glyph)
		{
			animData.push_back({});
			GlyphAnimData& glyphAnim = animData.back();
			glyphAnim.bitvecOffset = baseDisplacedBits;// +rng->getInt(0, 10); //values over 32 will loop 
			glyphAnim.updateSec = 0.05f +rng->getFloat(0.f, 0.05f);
			glyphAnim.shiftVal = glyphAnim.bitvecOffset;
		}
	}

	void GlitchTextFont::resetAnim()
	{
		for (GlyphAnimData& glyph : animData)
		{
			glyph.timeAccum = 0;
			if (bForwardDirection) { glyph.shiftVal = glyph.bitvecOffset; }
			else { glyph.shiftVal = 0; }
		}
	}

	void GlitchTextFont::completeAnimation()
	{
		for (GlyphAnimData& glyph : animData)
		{
			if (bForwardDirection) { glyph.shiftVal = 0; }
			else { glyph.shiftVal  = glyph.bitvecOffset; }
		}
	}

	bool GlitchTextFont::tick(float dt_sec)
	{
		if (bPlaying)
		{
			for (GlyphAnimData& glyph : animData)
			{
				glyph.timeAccum += dt_sec * speedScale;
				if (glyph.timeAccum > glyph.updateSec)
				{
					glyph.timeAccum -= glyph.updateSec;
					if (bForwardDirection) { glyph.shiftVal -= 1; }
					else { glyph.shiftVal += 1; }
					//clamp to range
					glyph.shiftVal = glm::clamp(glyph.shiftVal, 0, glyph.bitvecOffset);
				}
			}
		}

		return true;
	}

	void GlitchTextFont::setAnimPlayForward(bool bForward)
	{
		bForwardDirection = bForward;
		resetAnim();
	}

	sp<Shader> getGlitchTextShader()
	{
		static sp<Shader> shader = new_sp<Shader>(glitchText_vs, glitchText_fs, false);
		return shader;
	}
}

