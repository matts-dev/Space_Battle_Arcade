#pragma once

#include "SACameraBase.h"

namespace SA
{
	/**
		Vectors with the suffix _n can be assumed to be normalized.
	*/
	class CameraFPS : public CameraBase
	{
	public:
		CameraFPS(float inFOV, float inYaw, float inPitch);
		~CameraFPS();

		//callbacks
		virtual void onMouseMoved_v(double xpos, double ypos) override;
		virtual void onWindowFocusedChanged_v(int focusEntered) override;
		virtual void onMouseWheelUpdate_v(double xOffset, double yOffset) override;

		void handleInput(GLFWwindow* window, float deltaTime);

		//setters and getters
		void setYaw(float inYaw);
		void setPitch(float inPitch);
		void setSpeed(float speed);
		virtual void onCursorModeSet_v(bool inCursorMode) override;

		float getYaw() const { return yaw; }
		float getPitch() const { return pitch; }

		virtual void lookAt_v(glm::vec3 point) override;

	public:

		//virtual void registerToWindowCallbacks(sp<Window>& window) override;
		//virtual void deregisterToWindowCallbacks() override;

	private: //helper fields
		double lastX;
		double lastY;
		void calculateEulerAngles();

	private:
		float pitch = 0.f;
		float yaw = -90.f;
		
		float mouseSensitivity = 0.05f;
		float cameraSpeed = 2.5f;
		bool bAllowSpeedModifier = true;
		bool refocused = true;
	};
}