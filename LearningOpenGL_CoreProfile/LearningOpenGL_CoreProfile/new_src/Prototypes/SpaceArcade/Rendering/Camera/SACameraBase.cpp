#include "SACameraBase.h"
#include <iostream>

#include<glad/glad.h> //includes opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include "SACameraCallbackRegister.h"
#include "../SAWindow.h"
#include <gtc/matrix_transform.hpp>
#include "../../GameFramework/SALog.h"
#include "../../GameFramework/SAGameBase.h"
#include "../../GameFramework/SAWindowSystem.h"
#include "../../GameFramework/SALevelSystem.h"
#include "../../GameFramework/SALevel.h"
#include "../../GameFramework/TimeManagement/TickGroupManager.h"

namespace SA
{
	//hiding this function from header as it is implementation detail, but if subclass needs it then either make it shared function or protected member function
	static void _configureWindowForCursorMode(const sp<Window>& window, bool bCusorMode)
	{
		if (GLFWwindow* windowRaw = window ? window->get() : nullptr)
		{
			//It may be heavy handed to have the camera direct the state of the window; but doing this now as camera system is in flux
			glfwSetInputMode(windowRaw, GLFW_CURSOR, bCusorMode? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
		}
	}

	CameraBase::~CameraBase()
	{
	}

	void CameraBase::registerToWindowCallbacks_v(const sp<Window>& window)
	{
		deregisterToWindowCallbacks_v();

		if (window)
		{
			registeredWindow = window;

			if (!isInCursorMode())
			{
				window->cursorPosEvent.addWeakObj(sp_this(), &CameraBase::onMouseMoved_v);
				window->mouseLeftEvent.addWeakObj(sp_this(), &CameraBase::onWindowFocusedChanged_v);
				window->scrollChanged.addWeakObj(sp_this(), &CameraBase::onMouseWheelUpdate_v);
			}
			else
			{
				_configureWindowForCursorMode(window, cursorMode);
			}
		}
	}

	void CameraBase::deregisterToWindowCallbacks_v()
	{
		//this should not be deleted even with weak bindings, as user may switch between cameras and will need to deregister events.
		if (!registeredWindow.expired())
		{
			sp<Window> window = registeredWindow.lock();
			window->cursorPosEvent.removeWeak(sp_this(), &CameraBase::onMouseMoved_v);
			window->mouseLeftEvent.removeWeak(sp_this(), &CameraBase::onWindowFocusedChanged_v);
			window->scrollChanged.removeWeak(sp_this(), &CameraBase::onMouseWheelUpdate_v);
		}
	}

	void CameraBase::postConstruct()
	{
		GameBase::get().getLevelSystem().onPostLevelChange.addWeakObj(sp_this(), &CameraBase::v_handlePostLevelChange);
	}

	void CameraBase::activate()
	{
		if (!bActive)
		{
			static GameBase& game = GameBase::get();
			game.onPreGameloopTick.addWeakObj(sp_this(), &CameraBase::tick);

			onActivated();
			bActive = true;
		}
	}

	void CameraBase::deactivate()
	{
		if (bActive)
		{
			static GameBase& game = GameBase::get();
			game.onPreGameloopTick.removeWeak(sp_this(), &CameraBase::tick);

			onDeactivated();
			bActive = false;
		}
	}

	void CameraBase::setPosition(float x, float y, float z)
	{
		cameraPosition.x = x;
		cameraPosition.y = y;
		cameraPosition.z = z;
		onPositionSet_v(cameraPosition);
	}

	const glm::vec3 CameraBase::getFront() const
	{
		return cameraFront_n;
	}

	const glm::vec3 CameraBase::getRight() const
	{
		glm::vec3 cameraRight_n = glm::normalize(glm::cross(worldUp_n, -cameraFront_n));
		return cameraRight_n;
	}

	const glm::vec3 CameraBase::getUp() const
	{
		glm::vec3 cameraRight_n = glm::normalize(glm::cross(worldUp_n, -cameraFront_n));
		glm::vec3 cameraUp_n = glm::normalize(glm::cross(-cameraFront_n, cameraRight_n));
		return cameraUp_n;
	}

	void CameraBase::setFOV(float inFOV)
	{
		FOV = inFOV;
		onFOVSet_v(FOV);
	}

	glm::mat4 CameraBase::getView() const
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

		//glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + getFront(), getUp());

		return view;
	}

	glm::mat4 CameraBase::getPerspective() const
	{
		//default aspect to a box; if we don't have a window it would be strange to call this, but return something sensible anyways.
		float aspect = 1.f;

		//#TODO this can be expensive since it is acquiring a weak pointer, perhaps listen to registered window delegates on when size changes and cache values
		if (!registeredWindow.expired())
		{
			sp<Window> window = registeredWindow.lock();
			aspect = window->getAspect();
		}
		else
		{
			log("CameraBase", LogLevel::LOG_WARNING, "camera getPerspective() called but no registered window");
		}

		return glm::perspective(glm::radians(FOV), aspect, nearZ, farZ);
	}

	void CameraBase::setCursorMode(bool inCursorMode)
	{
		if (bCameraRequiresCursorMode && !inCursorMode)
		{
			log(__FUNCTION__, LogLevel::LOG_ERROR, "Attempting to disable cursor mode on a camera that has been flagged to require cursor mode! Logical error!");
		}

		cursorMode = inCursorMode;

		if (!registeredWindow.expired())
		{
			if (sp<Window> primaryWindow = registeredWindow.lock())
			{
				if (inCursorMode)
				{
					deregisterToWindowCallbacks_v();
				}
				else
				{
					registerToWindowCallbacks_v(primaryWindow);
				}
				_configureWindowForCursorMode(primaryWindow, cursorMode);
			}
		}
		onCursorModeSet_v(inCursorMode);
	}

	void CameraBase::v_handlePostLevelChange(const sp<LevelBase>& previousLevel, const sp<LevelBase>& newCurrentLevel)
	{
		if (bActive)
		{
			if (previousLevel)
			{
				previousLevel->getWorldTimeManager()->getEvent(TickGroups::get().CAMERA).removeWeak(sp_this(), &CameraBase::tick);
			}

			if (newCurrentLevel)
			{
				newCurrentLevel->getWorldTimeManager()->getEvent(TickGroups::get().CAMERA).addWeakObj(sp_this(), &CameraBase::tick);
			}
		}
	}

	void CameraBase::onCursorModeSet_v(bool inCursorMode)
	{
		if (!inCursorMode)
		{
			//don't jitter camera
			refocused = true;
		}
	}

	void CameraBase::tick(float dt_sec)
	{
		tickKeyboardInput(dt_sec);
	}

	void CameraBase::onMouseMoved_v(double xpos, double ypos)
	{
	}

	void CameraBase::onWindowFocusedChanged_v(int focusEntered)
	{
		refocused = focusEntered;
	}

	void CameraBase::onMouseWheelUpdate_v(double xOffset, double yOffset)
	{
	}

	void CameraBase::setPosition(const glm::vec3& newPosition)
	{
		cameraPosition = newPosition;
		onPositionSet_v(cameraPosition);
	}

	void CameraBase::setCameraRequiresCursorMode(bool bRequiresCursorMode, bool bApplyCursorModeRequirementNow /*= true*/)
	{
		bCameraRequiresCursorMode = bRequiresCursorMode;

		if (bApplyCursorModeRequirementNow)
		{
			setCursorMode(bRequiresCursorMode);
		}
	}

}