#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Utility
{
	struct Transform
	{
		glm::vec3 position = { 0, 0, 0 };
		glm::quat rotQuat{ 1,0,0,0 };
		glm::vec3 scale = { 1, 1, 1 };

		glm::mat4 getModelMatrix()
		{
			glm::mat4 model(1.0f);
			model = glm::translate(model, position);
			model = model * glm::toMat4(rotQuat);
			model = glm::scale(model, scale);
			return model;
		}
	};
}