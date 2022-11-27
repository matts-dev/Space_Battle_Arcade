#pragma once

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/** 
	Vectors with the suffix _n can be assumed to be normalized.
*/
class CameraFPS
{
public:
	CameraFPS(float inFOV, float inYaw, float inPitch);
	~CameraFPS();

	glm::mat4 getView() const;

	//callbacks
	void mouseMoved(double xpos, double ypos);
	void windowFocusedChanged(int focusEntered);
	void mouseWheelUpdate(double xOffset, double yOffset);
	void handleInput(GLFWwindow* window, float deltaTime);
	void exclusiveGLFWCallbackRegister(GLFWwindow* window);

	//setters and getters
	void setPosition(float x, float y, float z);
	void setPosition(glm::vec3 newPos);
	void setYaw(float inYaw);
	void setPitch(float inPitch);
	void setSpeed(float speed);

	const glm::vec3& getPosition() const { return cameraPosition; }
	const glm::vec3 getFront() const { return cameraFront_n; }
	const glm::vec3 getRight() const;
	const glm::vec3 getUp() const;
	float getFOV() const { return FOV; }
	float getYaw() const { return yaw; }
	float getPitch() const { return pitch; }
	void setCursorMode(bool inCursorMode);
	bool isInCursorMode() { return cursorMode; }

private: //helper fields
	double lastX;
	double lastY;
	bool refocused = true;
	void calculateEulerAngles();

private:
	glm::vec3 cameraPosition;
	glm::vec3 cameraFront_n;
	glm::vec3 worldUp_n;

	float pitch = 0.f;
	float yaw = -90.f;
	float FOV = 45.0f;

	float mouseSensitivity = 0.05f;
	float cameraSpeed = 2.5f;
	bool cursorMode = false;
};