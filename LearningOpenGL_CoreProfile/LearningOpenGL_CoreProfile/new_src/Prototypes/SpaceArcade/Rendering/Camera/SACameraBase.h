#pragma once

struct GLFWwindow;

namespace SA
{
	class CameraBase
	{
	public:

		virtual ~CameraBase();

		virtual void mouseMoved(double xpos, double ypos) = 0;
		virtual void windowFocusedChanged(int focusEntered) = 0;
		virtual void mouseWheelUpdate(double xOffset, double yOffset) = 0;
		virtual bool isInCursorMode() = 0;

		/** Assumes there is only ever one active window; perhaps this needs a redesign to use a window singleton*/
		void registerToWindowCallbacks(GLFWwindow* window);
	};
}
	
