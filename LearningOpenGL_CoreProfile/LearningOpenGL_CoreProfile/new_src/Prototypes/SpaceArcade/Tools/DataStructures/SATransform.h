#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

namespace SA
{
	struct Transform
	{
		inline glm::mat4 getModelMatrix() const noexcept
		{
			glm::mat4 model(1.0f);
			model = glm::translate(model, position);
			model = model * glm::toMat4(rotQuat);
			model = glm::scale(model, scale);
			return model;
		}

		glm::vec3 position = { 0, 0, 0 };
		glm::quat rotQuat = glm::quat{};
		glm::vec3 scale = { 1, 1, 1 };
	};

	glm::quat getRotQuatFromDegrees(glm::vec3 rotDegrees);

	struct EncapsulatedTransform
	{
		inline glm::mat4 getModelMatrix() const noexcept
		{
			glm::mat4 model(1.0f);
			model = glm::translate(model, position);
			model = model * glm::toMat4(rotQuat);
			model = glm::scale(model, scale);
			return model;
		}
		inline void setPosition(glm::vec3 inPosition)	noexcept { position = inPosition; }
		inline void setScale(glm::vec3 inScale)			noexcept { scale = inScale; }
		inline void setRotation(glm::quat inRotation)	noexcept{ rotQuat = inRotation; }
		inline const glm::vec3& getPosition()	const noexcept { return position; }
		inline const glm::vec3& getScale()		const noexcept { return scale; }
		inline const glm::quat& getRotation()	const noexcept { return rotQuat; }

	private:
		glm::vec3 position = { 0, 0, 0 };
		glm::quat rotQuat = glm::quat{};
		glm::vec3 scale = { 1, 1, 1 };
	};
}