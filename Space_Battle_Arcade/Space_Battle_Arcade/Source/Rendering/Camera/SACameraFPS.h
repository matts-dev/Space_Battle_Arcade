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
		CameraFPS(float inFOV=45.f, float inYaw=0.f, float inPitch=0.f);
		~CameraFPS();

		//callbacks
		virtual void onMouseMoved_v(double xpos, double ypos) override;
		virtual void onMouseWheelUpdate_v(double xOffset, double yOffset) override;
		virtual void tickKeyboardInput(float dt_sec) override;

		//setters and getters
		void setYaw(float inYaw);
		void setPitch(float inPitch);
		void setSpeed(float speed);
		float getYaw() const { return yaw; }
		float getPitch() const { return pitch; }
		virtual glm::quat getQuat() const override;

		virtual void lookAt_v(glm::vec3 point) override;

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
	};
}