#include "Camera.h"

Camera::Camera() :
	cameraPosition(0.f, 0.f, 3.f),
	cameraFrontAxis(0.f, 0.f, 0.f),
	cameraUp(0.f, 1.f, 0.f)
{
	updateRotationUsingPitchAndYaw();
}

void Camera::updateRotationUsingPitchAndYaw()
{
	cameraFrontAxis.y = sin(glm::radians(pitch));
	cameraFrontAxis.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	cameraFrontAxis.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	cameraFrontAxis = glm::normalize(cameraFrontAxis);
}

Camera::~Camera()
{
}

glm::mat4 Camera::getViewMatrix()
{
	glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraFrontAxis + cameraPosition, cameraUp);
	return viewMatrix;
}

void Camera::pollMovements(GLFWwindow* window, float deltaTime)
{
	float adjustedMovementSpeed = movementSpeed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		cameraPosition += cameraFrontAxis * adjustedMovementSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		cameraPosition -= cameraFrontAxis * adjustedMovementSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		cameraPosition -= glm::normalize(glm::cross(cameraFrontAxis, cameraUp)) * adjustedMovementSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		cameraPosition += glm::normalize(glm::cross(cameraFrontAxis, cameraUp)) * adjustedMovementSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		//figure out upward basis vector on fly;
		//1. we get the right basis vector from the inner most cross product, we don't need to normalize it since we only need for its direction in the next cross product
		//2. the second (outter) cross product returns the upward basis vector using the front basis vector and the derived right basis vector (from the inner cross product)
		//3. normalize it since we are going to use its magnitude 
		cameraPosition += glm::normalize(glm::cross(glm::cross(cameraFrontAxis, cameraUp), cameraFrontAxis)) * adjustedMovementSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
	{
		//figure out upward basis vector on fly (see above note)
		cameraPosition -= glm::normalize(glm::cross(glm::cross(cameraFrontAxis, cameraUp), cameraFrontAxis)) * adjustedMovementSpeed;
	}
}

void Camera::incrementFOV(float incrmentValue)
{
	FOV -= incrmentValue;

	//clamp FOV between the two values
	if (FOV < 1.0f)
		FOV = 1.0f;
	if (FOV > 45.0f)
		FOV = 45.0f;
}

void Camera::updateRotation(float xpos, float ypos)
{
	if (firstMouseCall)
	{
		lastMouseX = static_cast<float>(xpos);
		lastMouseY = static_cast<float>(ypos);
		firstMouseCall = false;
	}

	float deltaX = static_cast<float>(xpos - lastMouseX);
	float deltaY = static_cast<float>(lastMouseY - ypos);
	lastMouseX = static_cast<float>(xpos);
	lastMouseY = static_cast<float>(ypos);

	yaw += deltaX * mouseSensitivity;
	pitch += deltaY * mouseSensitivity;

	if (pitch > 89.f)
		pitch = 89.f;
	if (pitch < -89.f)
		pitch = -89.f;

	updateRotationUsingPitchAndYaw();
}

