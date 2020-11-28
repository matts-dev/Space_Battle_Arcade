#pragma once
#pragma once
#include <array>
#include "SASystemBase.h"
#include "../Tools/DataStructures/SATransform.h"


constexpr bool CLAMP_CURVE_PARAMS = true;

namespace SA
{
	class CurveSystem;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Numerical representation of a curve with fast lookup
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	template<std::size_t RESOLUTION = 20>
	struct Curve
	{
		/** Give a float percentage on range [0,1] and a mapped float on range [0,1] will be returned. */
		float eval_rigid(float a)
		{
			if constexpr (CLAMP_CURVE_PARAMS) { a = glm::clamp(a, 0.f, 1.f); } //this can be removed at compile time for performance; though that is not advised

			size_t stepIdx = size_t((RESOLUTION - 1) * a + 0.5f);
			return curveSteps[stepIdx];
		}
		float eval_smooth(float a)
		{
			if constexpr (CLAMP_CURVE_PARAMS) { a = glm::clamp(a, 0.f, 1.f); } //this can be removed at compile time for performance; though that is not advised
			
			//todo this code may need to be re-evaluated
			size_t bottomIdx = size_t((RESOLUTION - 1) * a);
			size_t topIdx = glm::clamp<size_t>(bottomIdx + 1, 0, RESOLUTION - 1);
			float bottomSample = curveSteps[bottomIdx];
			float topSample = curveSteps[topIdx];
			
			float sampleDelta = 1.0f / RESOLUTION; //perc between samples,
			float lerpAlpha = a - (float(bottomIdx) / RESOLUTION); //remove bottom fraction from passed alpha, this will let use see how far into sampleDelta we are, ie how close we are to next sample index

			float sample = glm::mix(bottomSample, topSample, lerpAlpha);

			return sample;
		}
	private:
		friend CurveSystem; 
		std::array<float, RESOLUTION> curveSteps;
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Curve system that can be used to get curves.
	// This is hardly a system and perhaps should be a standalone utility.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	using Curve_highp = Curve<200>;
	class CurveSystem : public SystemBase
	{
	public: //curves are provided by copy to avoid memory corruption issues; cache these in your PostConstruct and use them.
		Curve_highp sigmoid_medp() const;
	public:
		Curve_highp generateSigmoid_medp(float tuning = 20.0f);
	public:
		CurveSystem();
	protected:
		virtual void initSystem() override;
	public:
		static float sampleAnalyticSigmoid(float a, float tuning = 3.0f);
	private:
		Curve_highp curve_sigmoidmedp;
	};


	////////////////////////////////////////////////////////
	// Implementation
	////////////////////////////////////////////////////////
}