#pragma once

#include <optional>
#include <cstdint>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "../../GameFramework/SAGameEntity.h"

struct GLFWwindow;

namespace SA
{
	class Window;
	class LevelBase;

	/**
	Vectors with the suffix _n can be assumed to be normalized.
	*/
	class CameraBase : public GameEntity
	{
	public:
		virtual ~CameraBase();

		/** Sets a camera to use window callbacks. This function exists because not all cameras represent what a viewport sees.
			When overridding registration, call super CamerBase::Method to get default registrations */
		virtual void registerToWindowCallbacks_v(const sp<Window>& window);
		virtual void deregisterToWindowCallbacks_v();
		virtual void lookAt_v(glm::vec3 point) = 0;
		virtual glm::quat getQuat() const = 0;
		virtual void postConstruct() override;
	public:
		void activate();
		void deactivate();
		const glm::vec3& getPosition() const { return cameraPosition; }
		void setPosition(float x, float y, float z);
		void setPosition(const glm::vec3& newPos);
		void setNear(float newNear) { nearZ = newNear; }
		void setFar(float newFar) { farZ = newFar; }

		//camera axes/bases
		virtual const glm::vec3 getFront() const;
		virtual const glm::vec3 getRight() const;
		virtual const glm::vec3 getUp() const;

		//game specific cameras can overload this to query the velocity of their current follow target or track velocity for previous frames
		virtual glm::vec3 getVelocity() const { return glm::vec3{ 0.f }; }

		const glm::vec3 getWorldUp_n() const { return worldUp_n; }

		void setFOV(float inFOV);
		float getFOV() const { return FOV; } //#TODO refactor so that this clearly states FOVy_deg
		float getNear() const { return nearZ; }
		float getFar() const { return farZ; }
		virtual glm::mat4 getView() const;
		virtual glm::mat4 getPerspective() const;

		bool isInCursorMode() { return cursorMode; }
		void setCursorMode(bool inCursorMode);

		bool cameraRequiresCursorMode() const { return bCameraRequiresCursorMode; }
		void setCameraRequiresCursorMode(bool bRequiresCursorMode, bool bApplyCursorModeRequirementNow = true);

		void setOwningPlayerIndex(std::optional<int32_t> playerIndex) { this->owningPlayerIndex = playerIndex; }

	protected:
		/** The setter virtuals are provided for recalculation when things outside of the camera are setting values. In some cases
		values have just been calculated, and it would be redundant to call the virtuals. In this case, the base class may set values
		without invoking the virtual functions. The functions below meet that requirement. */
		/** does not call native functions for setting position */
		void adjustPosition(const glm::vec3& deltaPosition) { cameraPosition += deltaPosition; }

		/* Not a great pattern to follow, but FOV case is trivial */
		float& adjustFOV() { return FOV; }
		void child_setFront(const glm::vec3& newFront_n) { cameraFront_n = newFront_n; }
		virtual void v_handlePreLevelChange(const sp<LevelBase>& /*previousLevel*/, const sp<LevelBase>& /*newCurrentLevel*/);

	private:
		virtual void onPositionSet_v(const glm::vec3& newPosition) {}
		virtual void onFOVSet_v(float FOV) {}
		virtual void onCursorModeSet_v(bool inCursorMode);
		virtual void onActivated() {};
		virtual void onDeactivated() {};
	protected:
		virtual void tick(float dt_sec);
		virtual void tickKeyboardInput(float dt_sec) {};

	protected: //protected so subclasses can call super
		virtual void onMouseMoved_v(double xpos, double ypos);
		virtual void onWindowFocusedChanged_v(int focusEntered);
		virtual void onMouseWheelUpdate_v(double xOffset, double yOffset);
		const std::optional<uint32_t>& getOwningPlayerIndex() { return owningPlayerIndex; }
	protected:
		bool refocused = true;
	private:
		bool bCameraRequiresCursorMode = false;
	private:
		glm::vec3 cameraPosition{0.f};
		glm::vec3 cameraFront_n;
		glm::vec3 worldUp_n = glm::vec3(0.f, 1.f, 0.f);
		float FOV = 45.0f;
		float nearZ = 0.1f;
		float farZ = 1000.f;
		std::optional<uint32_t> owningPlayerIndex;
		bool cursorMode = false;
		bool bActive = false;
		wp<SA::Window> registeredWindow;

	};
}
	
