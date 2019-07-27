#pragma once

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "..\..\GameFramework\SAGameEntity.h"

struct GLFWwindow;

namespace SA
{
	class Window;

	/**
	Vectors with the suffix _n can be assumed to be normalized.
	*/
	class CameraBase : public GameEntity
	{

	public:
		virtual ~CameraBase();

		/* When overridding registration, call super CamerBase::Method to get default registrations*/
		virtual void registerToWindowCallbacks_v(sp<Window>& window);
		virtual void deregisterToWindowCallbacks_v();
		virtual void lookAt_v(glm::vec3 point) = 0;

	public:
		const glm::vec3& getPosition() const { return cameraPosition; }
		void setPosition(float x, float y, float z);
		void setPosition(const glm::vec3& newPos);
		void setNear(float newNear) { nearZ = newNear; }
		void setFar(float newFar) { farZ = newFar; }

		//camera axes/bases
		const glm::vec3 getFront() const { return cameraFront_n; }
		const glm::vec3 getRight() const;
		const glm::vec3 getUp() const;

		const glm::vec3 getWorldUp_n() const { return worldUp_n; }

		void setFOV(float inFOV);
		float getFOV() const { return FOV; }
		float getNear() const { return nearZ; }
		float getFar() const { return farZ; }
		glm::mat4 getView() const;
		glm::mat4 getPerspective() const;

		bool isInCursorMode() { return cursorMode; }
		void setCursorMode(bool inCursorMode);

	protected:
		/** The setter virtuals are provided for recalcuation when things outside of the camera are setting values. In some cases
		values have just been calculated, and it would be redundant to call the virtuals. In this case, the base class may set values
		without invoking the virtual functions. The functions below meet that requirement. */
		/** does not call native functions for setting position */
		void adjustPosition(const glm::vec3& deltaPosition) { cameraPosition += deltaPosition; }

		/* Not a great pattern to follow, but FOV case is trivial */
		float& adjustFOV() { return FOV; }
		void child_setFront(const glm::vec3& newFront_n) { cameraFront_n = newFront_n; }

	private:
		virtual void onPositionSet_v(const glm::vec3& newPosition) {}
		virtual void onFOVSet_v(float FOV) {}
		virtual void onCursorModeSet_v(bool inCursorMode) {};

	protected: //protected so subclasses can call super
		virtual void onMouseMoved_v(double xpos, double ypos);
		virtual void onWindowFocusedChanged_v(int focusEntered);
		virtual void onMouseWheelUpdate_v(double xOffset, double yOffset);

	private:
		glm::vec3 cameraPosition;
		glm::vec3 cameraFront_n;
		glm::vec3 worldUp_n = glm::vec3(0.f, 1.f, 0.f);
		float FOV = 45.0f;
		float nearZ = 1.f;
		float farZ = 500.f;
		bool cursorMode = false;
		wp<SA::Window> registeredWindow;

	};
}
	
