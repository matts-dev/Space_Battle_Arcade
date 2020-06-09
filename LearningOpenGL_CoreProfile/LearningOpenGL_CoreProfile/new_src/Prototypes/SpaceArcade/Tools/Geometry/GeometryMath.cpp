#include "GeometryMath.h"
#include "Plane.h"

namespace SA
{
	using namespace glm;

	float scalarProjectAontoB_AutoNormalize(const glm::vec3 a, const glm::vec3 b)
	{
		return scalarProjectAontoB(a, glm::normalize(b));
	}

	float scalarProjectAontoB(const glm::vec3 a, const glm::vec3 b_normalized)
	{
		return glm::dot(a, b_normalized);
	}

	glm::vec3 projectAontoB(const glm::vec3 a, const glm::vec3 b)
	{
		using namespace glm;

		//scalar projection is doting a vector onto the normalized vector of b
		//		vec3 bUnitVec = b / ||b||;	//ie normali
		//		scalarProj = a dot (b/||b||);	//ie dot with bUnitVec
		// you then then need to scale up the bUnitVector to get a projection vector
		//		vec3 projection = bUnitVector * scalarProjection
		// this can be simplified so we never have to do a square root for normalization (this looks better when written in 
		//		vec3 projection = bUnitVector * scalarProjection
		//		vec3 projection = b / ||b||	  * a dot (b/||b||)
		//		vec3 projection = b / ||b||	  * (a dot b) / ||b||		//factor around dot product
		//		vec3 projection =     b * (a dot b) / ||b||*||b||		//multiply these two terms
		//		vec3 projection =     b * ((a dot b) / ||b||*||b||)		//recale dot product will product scalar, lump scalars in parenthesis
		//		vec3 projection =     ((a dot b) / ||b||*||b||) * b;	//here b is a vector, so we have scalar * vector;
		//		vec3 projection =     ((a dot b) / (b dot b)) * b;	//recall that dot(b,b) == ||b||^2 == ||b||*||b||
		vec3 projection = (glm::dot(a, b) / glm::dot(b, b)) * b;
		return projection;
	}

	std::optional<glm::vec3> rayPlaneIntersection(const Plane& plane, glm::vec3 rayStart, glm::vec3 rayDir)
	{
		//calculate t value when ray hits plane
		float n_dot_d = glm::dot(plane.normal_v, rayDir);
		if (glm::abs(n_dot_d) < 0.0001f)
		{
			return std::nullopt;
		}

		float n_dot_ps = glm::dot(plane.normal_v, plane.point - rayStart);
		float camRayT = n_dot_ps / n_dot_d;

		vec3 planePoint = rayStart + camRayT * rayDir;

		return planePoint;
	}

}

