#pragma once

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

class CameraFPS
{
public:
	CameraFPS(float inFOV, float inYaw, float inPitch);
	~CameraFPS();

	glm::mat4 getView();

	//callbacks
	void mouseMoved(double xpos, double ypos);
	void windowFocusedChanged(int focusEntered);
	void mouseWheelUpdate(double xOffset, double yOffset);
	void handleInput(GLFWwindow* window, float deltaTime);
	void exclusiveGLFWCallbackRegister(GLFWwindow* window);

	//setters and getters
	void setPosition(float x, float y, float z);

private: //helper fields
	double lastX;
	double lastY;
	bool refocused = true;
	void calculateEulerAngles();

private:
	glm::vec3 cameraPosition;
	glm::vec3 cameraFront;
	glm::vec3 worldUp;

	float pitch = 0.f;
	float yaw = -90.f;
	float FOV = 45.0f;

	float mouseSensitivity = 0.05f;
	float cameraSpeed = 2.5f;
	
};