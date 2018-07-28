#pragma once
#include"glm.hpp"
#include"gtc/matrix_transform.hpp"
#include"gtc/type_ptr.hpp"
#include<glad/glad.h>
#include<GLFW/glfw3.h>

class Camera
{
public:
	glm::mat4 getViewMatrix();
	void pollMovements(GLFWwindow* window, float deltaTime);
	void incrementFOV(float incrmentValue);
	void updateRotation(float mousePosX, float mousePosY);
	float getFOV() { return FOV; }
	glm::vec3 getCameraPositionCopy();

	Camera();
	~Camera();
private:
	Camera(const Camera& copy) = delete;
	Camera& operator=(const Camera& copy) = delete;

	void updateRotationUsingPitchAndYaw();

	glm::vec3 cameraPosition;
	glm::vec3 cameraFrontAxis;
	glm::vec3 cameraUp;
	float movementSpeed = 10.f;
	float FOV = 45.f;
	float yaw = -90.f;
	float pitch = 0.f;
	float mouseSensitivity = 0.1f;
	float lastMouseX= 0.f;
	float lastMouseY= 0.f;
	bool firstMouseCall = true;

};

