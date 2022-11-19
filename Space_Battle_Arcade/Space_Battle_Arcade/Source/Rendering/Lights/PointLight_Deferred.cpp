#include "PointLight_Deferred.h"
#include <cmath>
#include "Rendering/SAShader.h"
#include "GameFramework/SARenderSystem.h"

namespace SA
{


	void PointLight_Deferred::applyUniforms(Shader& shader)
	{
		/* --UNIFORM TO UPDATE--
		struct PointLight
		{
			vec3 position;
			vec3 ambientIntensity;	vec3 diffuseIntensity;	vec3 specularIntensity;
			float constant;			float linear;			float quadratic;
		};
		uniform PointLight pointLights
		*/

		shader.setUniform3f("pointLights.ambientIntensity", userData.ambientIntensity);
		shader.setUniform3f("pointLights.diffuseIntensity", userData.diffuseIntensity);
		shader.setUniform3f("pointLights.specularIntensity", userData.specularIntensity);
		shader.setUniform3f("pointLights.position", userData.position);
		shader.setUniform1f("pointLights.constant", userData.attenuationConstant);
		shader.setUniform1f("pointLights.linear", userData.attenuationLinear);
		shader.setUniform1f("pointLights.quadratic", userData.attenuationQuadratic);
	}

	void PointLight_Deferred::clean()
	{
		systemMetaData.bUserDataDirty = false;

		//The light volume is a sphere that's radius is considered the maximum possible distance that the light has influence (ie lights objects up)
		//We can figure out this radius by solving for when the light's influence would be 0 due to being too far away (at least sort of).
		//since the attenuation function we used (light / (Constant + LinearConstant * d + QuadraticConstant * d^2) only approaches 0, we don't solve directly for 0;
		//instead we pick some very dark value and solve for that instead.
		//The tutorial recommends 5/256 as a illumination amount because it is near dark

		// Let's say that A = Constant + LinearConstant * d + QuadraticConstant * d^2; and I = light
		//		also that Kl = LienarConstant, Kq = QuadraticConstant, and Kc = Constant
		// thus, as our equation to solve we have we have we have:		(5/256) = light	/ A
		//		light is a fixed amount, but our distances in attenuation are not; let's solve for the distance
		//
		//	(5/256) = I / A
		//	A * (5/256) = I
		//	A * 5 = 256 * I
		//	A = (256/5) * I
		//	Kc + Kl*d + Kq*d^2 = (256/5) * I
		// Kc + Kl*d + Kq*d^2 - ((256/5) * I) = 0
		//
		// We can use the quadratic formula to solve for this
		// note that all the constants should be positive values
		// note also that we want a positive distance for the radius,
		// and because the first term of the quadratic equation is -B; we only want to add the sqrt(-B^2 - 4AC) (because sqrt will be positive and we don't want (negative - positive) because distance would then be negative
		//
		// Kc + Kl*d + Kq*d^2 - ((256/5) * I) = 0
		// A = Kq
		// B = Kl
		// C = Kc - (256/5)*I
		//
		// d = (-B +- sqrt(B^2 - 4AC))   /     (2A)
		//
		// d = (  -B  + sqrt( B^2  - 4*A *        C      ))   /     (2A)
		// d = ((-Kl) + sqrt(Kl*Kl - 4*Kq*(Kc - (256/5)*I))   /      2 * Kq
		glm::vec3 color = userData.diffuseIntensity; 
		float Kc = userData.attenuationConstant;	//light.getAttenuationConstant();
		float Kl = userData.attenuationLinear;		//light.getAttenuationLinear();
		float Kq = userData.attenuationQuadratic;	//light.getAttenuationQuadratic();

		//take the strongest component as it will have the furthest attenuation distance
		//float maxLight = std::fmaxf(color.r, std::fmax(color.g, color.b));
		float maxLight = color.r + color.g + color.b;

		//float distance = (-Kl + std::sqrt(Kl*Kl - 4*Kq*(Kc-((256.0f/5.0f)* maxLight) ) ) / (2 * Kq)
		float A = Kq;
		float B = Kl;
		float C = Kc - ((256.0f / 5.0f) * maxLight);
		float distance = (-B + std::sqrt(B*B - 4 * A*C)) / (2 * A);

		//for tutorial simplicity, just store this as a public variable
		systemMetaData.maxRadius = distance;

#if !IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE
		todo_add_filter_step_so_pointlights_with_one_user_are_not_rendered;
#endif //IGNORE_INCOMPLETE_DEFERRED_RENDER_CODE

	}
}

