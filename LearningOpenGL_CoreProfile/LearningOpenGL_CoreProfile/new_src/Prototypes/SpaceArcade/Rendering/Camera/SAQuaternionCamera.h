#pragma once

#include "SACameraBase.h"

namespace SA
{
	class QuaternionCamera : public CameraBase
	{
	public:
		QuaternionCamera();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CameraBase interface
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	public:
		//virtual void registerToWindowCallbacks_v(sp<Window>& window) override;
		//virtual void deregisterToWindowCallbacks_v() override;
		virtual void lookAt_v(glm::vec3 point) override;
		virtual const glm::vec3 getFront() const override {return -w_axis;}
		virtual const glm::vec3 getRight() const override {return u_axis;}
		virtual const glm::vec3 getUp() const override { return v_axis;}
		virtual glm::mat4 getView() const;
		virtual glm::quat getQuat() { return myQuat; }
		virtual void setQuat(glm::quat newQuat);
	public:
		void setSpeed(float newSpeed) { freeRoamSpeed = newSpeed; }
		float getSpeed() const { return freeRoamSpeed; }
	protected:
		virtual void onMouseMoved_v(double xpos, double ypos) override;
		virtual void onMouseWheelUpdate_v(double xOffset, double yOffset) override;
	private:
		virtual void tickKeyboardInput(float dt_sec) override;
		//virtual void onPositionSet_v(const glm::vec3& newPosition) override;
		//virtual void onFOVSet_v(float FOV) override;
		//virtual void onCursorModeSet_v(bool inCursorMode) override;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Quaternion Camera
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		void updateRoll(float rollAmount_rad);

	private:
		void updateAxes();

	private: //helper fields
		double lastX = 0.f;
		double lastY = 0.f;
	private:
		const glm::vec3 DEFAULT_AXIS_U = glm::vec3(1.f, 0, 0);
		const glm::vec3 DEFAULT_AXIS_V = glm::vec3(0, 1.f, 0);
		const glm::vec3 DEFAULT_AXIS_W = glm::vec3(0, 0, 1.f);

		glm::vec3 u_axis;
		glm::vec3 v_axis;
		glm::vec3 w_axis;

		glm::quat myQuat;

		float mouseSensitivity = 0.00125f;
		float freeRoamSpeed = 10.0f;
		float freeRoamRollSpeed_radSec = glm::radians(90.0f);
	};
}