#pragma once
#include "Tools/DataStructures/SATransform.h"
#include <optional>

namespace SA
{
	struct Plane;

	glm::vec3 projectAontoB(const glm::vec3 a, const glm::vec3 b);
	float scalarProjectAontoB_AutoNormalize(const glm::vec3 a, const glm::vec3 b);
	float scalarProjectAontoB(const glm::vec3 a, const glm::vec3 b_normalized);

	std::optional<glm::vec3> rayPlaneIntersection(const Plane& plane, glm::vec3 rayStart, glm::vec3 rayDir);

}
