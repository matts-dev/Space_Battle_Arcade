#include "SACameraFPS.h"
#include <iostream>
#include "..\SAWindow.h"

namespace SA
{
	CameraFPS::CameraFPS(float inFOV, float inYaw, float inPitch) :
		worldUp_n(0.f, 1.f, 0.f),
		FOV(inFOV),
		yaw(inYaw),
		pitch(inPitch)
	{
		calculateEulerAngles();
	}

	CameraFPS::~CameraFPS()
	{
	}

	glm::mat4 CameraFPS::getView() const
	{
		//this creates a view matrix from two matrices.
		//the first matrix (ie the matrix on the right which gets applied first) is just translating points by the opposite of the camera's position.
		//the second matrix (ie matrix on the left) is a matrix where the first 3 rows are the U, V, and W axes of the camera.
		//		The matrix with the axes of the camera is a change of basis matrix.
		//		If you think about it, what it does to a vector is a dot product with each of the camera's axes.
		//		Since the camera axes are all normalized, what this is really doing is projecting the vector on the 3 axes of the camera!
		//			(recall that a vector dotted with a normal vector is equivalent of getting the scalar projection onto the normal)
		//		So, the result of the projection is the vector's position in terms of the camera position.
		//		Since we're saying that the camera's axes align with our NDC axes, we get coordinates in terms x y z that are ready for clipping and projection to NDC 
		//So, ultimately the lookAt matrix shifts points based on camera's position, then projects them onto the camera's axes -- which are aligned with OpenGL's axes 

		glm::vec3 cameraAxisW = glm::normalize(cameraPosition - (cameraFront_n + cameraPosition)); //points towards camera; camera looks down its -z axis (ie down its -w axis)
		glm::vec3 cameraAxisU = glm::normalize(glm::cross(worldUp_n, cameraAxisW)); //this is the right axis (ie x-axis)
		glm::vec3 cameraAxisV = glm::normalize(glm::cross(cameraAxisW, cameraAxisU));
		//make each row in the matrix a camera's basis vector; glm is column-major
		glm::mat4 cameraBasisProjection;
		cameraBasisProjection[0][0] = cameraAxisU.x;
		cameraBasisProjection[1][0] = cameraAxisU.y;
		cameraBasisProjection[2][0] = cameraAxisU.z;

		cameraBasisProjection[0][1] = cameraAxisV.x;
		cameraBasisProjection[1][1] = cameraAxisV.y;
		cameraBasisProjection[2][1] = cameraAxisV.z;

		cameraBasisProjection[0][2] = cameraAxisW.x;
		cameraBasisProjection[1][2] = cameraAxisW.y;
		cameraBasisProjection[2][2] = cameraAxisW.z;
		glm::mat4 cameraTranslate = glm::translate(glm::mat4(), -1.f * cameraPosition);

		glm::mat4 view = cameraBasisProjection * cameraTranslate;
		//glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, worldUp);

		return view;
	}

	void CameraFPS::mouseMoved(double xpos, double ypos)
	{
		if (refocused)
		{
			lastX = xpos;
			lastY = ypos;
			refocused = false;
		}

		double xOff = xpos - lastX;
		double yOff = lastY - ypos;

		xOff *= mouseSensitivity;
		yOff *= mouseSensitivity;

		yaw += static_cast<float>(xOff);
		pitch += static_cast<float>(yOff);

		lastX = xpos;
		lastY = ypos;

		if (pitch > 89.0f)
			pitch = 89.f;
		else if (pitch < -89.0f)
			pitch = -89.f;

		calculateEulerAngles();
	}

	void CameraFPS::windowFocusedChanged(int focusEntered)
	{
		refocused = focusEntered;
	}

	void CameraFPS::mouseWheelUpdate(double xOffset, double yOffset)
	{
		if (FOV >= 1.0f && FOV <= 45.0f)
			FOV -= static_cast<float>(yOffset);
		if (FOV <= 1.0f)
			FOV = 1.0f;
		if (FOV >= 45.0)
			FOV = 45.0f;
	}

	void CameraFPS::handleInput(GLFWwindow* window, float deltaTime)
	{
		float bSpeedAccerlationFactor = 1.0f;
		if (bAllowSpeedModifier && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		{
			bSpeedAccerlationFactor = 10.0f;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			cameraPosition -= cameraFront_n * cameraSpeed * bSpeedAccerlationFactor * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			cameraPosition += cameraFront_n * cameraSpeed * bSpeedAccerlationFactor  * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			//the w basis vector is the -cameraFront
			glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp_n, -cameraFront_n));
			cameraPosition += cameraRight * cameraSpeed * bSpeedAccerlationFactor  * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			//the w basis vector is the -cameraFront
			glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp_n, -cameraFront_n));
			cameraPosition -= cameraRight * cameraSpeed * bSpeedAccerlationFactor * deltaTime;
		}
	}

	void CameraFPS::setPosition(float x, float y, float z)
	{
		cameraPosition.x = x;
		cameraPosition.y = y;
		cameraPosition.z = z;
	}

	void CameraFPS::setPosition(glm::vec3 newPos)
	{
		cameraPosition = newPos;
	}

	void CameraFPS::setYaw(float inYaw)
	{
		yaw = inYaw;
		calculateEulerAngles();
	}

	void CameraFPS::setPitch(float inPitch)
	{
		pitch = inPitch;
		calculateEulerAngles();
	}

	void CameraFPS::setSpeed(float speed)
	{
		cameraSpeed = speed;
	}

	const glm::vec3 CameraFPS::getRight() const
	{
		glm::vec3 cameraRight_n = glm::normalize(glm::cross(worldUp_n, -cameraFront_n));
		return cameraRight_n;
	}

	const glm::vec3 CameraFPS::getUp() const
	{
		glm::vec3 cameraRight_n = glm::normalize(glm::cross(worldUp_n, -cameraFront_n));
		glm::vec3 cameraUp_n = glm::normalize(glm::cross(-cameraFront_n, cameraRight_n));
		return cameraUp_n;
	}

	void CameraFPS::setCursorMode(bool inCursorMode)
	{
		cursorMode = inCursorMode;
		if (!cursorMode)
		{
			//don't gitter camera
			refocused = true;
		}
	}

	void CameraFPS::registerToWindowCallbacks(sp<Window>& window)
	{
		deregisterToWindowCallbacks();

		if (window)
		{
			registeredWindow = window;

			//adding strong bindings for speed, but this will keep camera alive through shared_ptrs and must be cleared via deregister
			window->cursorPosEvent.addStrongObj(sp_this(), &CameraFPS::mouseMoved);
			window->mouseLeftEvent.addStrongObj(sp_this(), &CameraFPS::windowFocusedChanged);
			window->scrollChanged.addStrongObj(sp_this(), &CameraFPS::mouseWheelUpdate);
		}
	}

	void CameraFPS::deregisterToWindowCallbacks()
	{
		if (!registeredWindow.expired())
		{
			sp<Window> window = registeredWindow.lock();
			window->cursorPosEvent.removeStrong(sp_this(), &CameraFPS::mouseMoved);
			window->mouseLeftEvent.removeStrong(sp_this(), &CameraFPS::windowFocusedChanged);
			window->scrollChanged.removeStrong(sp_this(), &CameraFPS::mouseWheelUpdate);
		}
	}

	void CameraFPS::calculateEulerAngles()
	{
		glm::vec3 camDir;
		camDir.y = sin(glm::radians(pitch));
		camDir.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		camDir.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		camDir = glm::normalize(camDir); //make sure this is normalized for when user polls front
		cameraFront_n = camDir;
	}
}