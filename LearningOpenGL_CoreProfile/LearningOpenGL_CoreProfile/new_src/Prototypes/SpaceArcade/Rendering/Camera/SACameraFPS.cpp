#include "SACameraFPS.h"
#include <iostream>
#include "..\SAWindow.h"
#include "..\..\Game\SpaceArcade.h"
#include "..\..\GameFramework\SAWindowSystem.h"
#include <gtx\compatibility.hpp>
#include "..\..\Tools\SAUtilities.h"

namespace SA
{
	CameraFPS::CameraFPS(float inFOV, float inYaw, float inPitch) :
		yaw(inYaw),
		pitch(inPitch)
	{
		setFOV(inFOV);
		calculateEulerAngles();
	}

	CameraFPS::~CameraFPS()
	{
	}

	void CameraFPS::onMouseMoved_v(double xpos, double ypos)
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
	
		if (std::isinf(yaw))
		{
			yaw = 0;
		}

		calculateEulerAngles();
	}

	void CameraFPS::onMouseWheelUpdate_v(double xOffset, double yOffset)
	{
		float sensitivity = 0.5f;

		float& mutableFOV = adjustFOV();
		if (mutableFOV >= 1.0f && mutableFOV <= 45.0f)
			mutableFOV -= sensitivity * static_cast<float>(yOffset);
		if (mutableFOV <= 1.0f)
			mutableFOV = 1.0f;
		if (mutableFOV >= 45.0)
			mutableFOV = 45.0f;
	}

	void CameraFPS::tickKeyboardInput(float dt_sec)
	{
		static WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		const sp<Window>& primaryWindow = windowSystem.getPrimaryWindow();
		if (primaryWindow)
		{
			GLFWwindow* window = primaryWindow->get();

			float bSpeedAccerlationFactor = 1.0f;
			if (bAllowSpeedModifier && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			{
				bSpeedAccerlationFactor = 10.0f;
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			{
				//cameraPosition -= cameraFront_n * cameraSpeed * bSpeedAccerlationFactor * deltaTime;
				adjustPosition(-(getFront() * cameraSpeed * bSpeedAccerlationFactor * dt_sec));
			}
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			{
				//cameraPosition += cameraFront_n * cameraSpeed * bSpeedAccerlationFactor  * deltaTime;
				adjustPosition(getFront() * cameraSpeed * bSpeedAccerlationFactor  * dt_sec);
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			{
				//the w basis vector is the -cameraFront
				glm::vec3 cameraRight = glm::normalize(glm::cross(getWorldUp_n(), -getFront()));
				//cameraPosition += cameraRight * cameraSpeed * bSpeedAccerlationFactor  * deltaTime;
				adjustPosition(cameraRight * cameraSpeed * bSpeedAccerlationFactor  * dt_sec);
			}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			{
				//the w basis vector is the -cameraFront
				glm::vec3 cameraRight = glm::normalize(glm::cross(getWorldUp_n(), -getFront()));
				//cameraPosition -= cameraRight * cameraSpeed * bSpeedAccerlationFactor * deltaTime;
				adjustPosition(-(cameraRight * cameraSpeed * bSpeedAccerlationFactor * dt_sec));
			}
		}
	}

	void CameraFPS::setYaw(float inYaw)
	{
		yaw = inYaw;
		if (std::isinf(yaw))
		{
			yaw = 0;
		}
		calculateEulerAngles();
	}

	void CameraFPS::setPitch(float inPitch)
	{
		pitch = inPitch;

		if (pitch > 89.0f)
			pitch = 89.f;
		else if (pitch < -89.0f)
			pitch = -89.f;

		calculateEulerAngles();
	}

	void CameraFPS::setSpeed(float speed)
	{
		cameraSpeed = speed;
	}

	void CameraFPS::lookAt_v(glm::vec3 point)
	{
		glm::vec3 dir = glm::normalize(point - getPosition());

		//based on euler calculation func
		// camDir.y = sin(pitch); so pitch = asin(camDir.y)
		pitch = std::asinf(dir.y);
		pitch = glm::clamp(pitch, -1.f, 1.f);

		//don't let a divide by zero happen
		if (!Utils::float_equals(pitch, 1.f))
		{
			if (!Utils::float_equals(dir.x, 0.f))
			{
				//camDir.x = cos(pitch) * cos(yaw)
				//camDir.x / cos(pitch) = cos(yaw)
				//acos(camDir.x / cos(pitch)) = yaw
				float cosval = dir.x / std::cosf(pitch);

				//float rounding errors like 1.000000012 are possible
				cosval = glm::clamp(cosval, -1.f, 1.f);
				yaw = std::acosf(cosval);
			}
			else
			{
				//camDir.z = cos(pitch) * sin(yaw)
				//camDir.z / cos(pitch) = sin(yaw)
				//asin(camDir.z / cos(pitch)) = yaw
				float sinval = dir.z / std::cosf(pitch);

				//float rounding errors like 1.000000012 are possible
				sinval = glm::clamp(sinval, -1.f, 1.f); 
				yaw = std::asinf(sinval);
			}
		}
		else //looking straight up/down
		{
			//preserve previous yaw so when looking down/up it aligns with previous direction
			//(or we could set it to zero)
		}

		//previous operations were using radians, convert to degrees before exit
		//must do this last because yaw operations depend on pitch being in radians
		pitch = glm::degrees(pitch);
		yaw = glm::degrees(yaw);

		if (std::isinf(yaw)) 
		{ 
			yaw = 0;
		}
		if (std::isinf(pitch))
		{
			pitch = 0;
		}

		calculateEulerAngles();
	}

	void CameraFPS::calculateEulerAngles()
	{
		glm::vec3 camDir;
		camDir.y = sin(glm::radians(pitch));
		camDir.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		camDir.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		camDir = glm::normalize(camDir); //make sure this is normalized for when user polls front
		child_setFront(camDir);
	}
}