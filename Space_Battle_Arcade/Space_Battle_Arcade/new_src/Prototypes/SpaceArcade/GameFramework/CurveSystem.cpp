#include "CurveSystem.h"

namespace SA
{
	CurveSystem::CurveSystem()
	{
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Create curves in ctor so that subclasses cannot forget to call super.
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		curve_sigmoidmedp = generateSigmoid_medp(3.0);
	}

	void CurveSystem::initSystem()
	{

	}

	float CurveSystem::sampleAnalyticSigmoid(float a, float tuning)
	{
		// math: https://stats.stackexchange.com/questions/214877/is-there-a-formula-for-an-s-shaped-curve-with-domain-and-range-0-1

		float sigmoidSample = 1.f / (1 + std::powf(a / (1.f - a), -tuning));
		return sigmoidSample;
	}

	Curve_highp CurveSystem::sigmoid_medp() const
	{
		//this intentionally returns a copy so that any bad code that has out of bound memory access will not be able to corrupt this system's memory.
		return curve_sigmoidmedp;
	}

	SA::Curve_highp CurveSystem::generateSigmoid_medp(float tuning)
	{
		Curve_highp curve;

		for (size_t sample = 0; sample < curve.curveSteps.size(); ++sample)
		{
			curve.curveSteps[sample] = sampleAnalyticSigmoid(float(sample) / (curve.curveSteps.size() - 1), 3.0); //have to remember to -1 to get lats element at 1
		}

		return curve;
	}

}
