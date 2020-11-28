#include "SATransform.h"

namespace SA
{


	glm::quat SA::getRotQuatFromDegrees(glm::vec3 rotDegrees)
	{
		glm::quat rot = glm::angleAxis(glm::radians(rotDegrees.x), glm::vec3(1, 0, 0));
		rot *= glm::angleAxis(glm::radians(rotDegrees.y), glm::vec3(0, 1, 0));
		rot *= glm::angleAxis(glm::radians(rotDegrees.z), glm::vec3(0, 0, 1));
		return rot;
	}


}

