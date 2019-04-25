#include "SACameraCallbackRegister.h"

#include<glad/glad.h> //includes opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include "SACameraBase.h"

namespace /*local to .cpp */
{
	//glfw callbacks are a c construct and making registering a bound obj to a member function hard.
	//I've opted to just assume there's only every one glfw callback handler that is set statically
	//its memory is cleaned up in camera dtors and can only be set via this .cpp.
	//I suppose a more elegant solution would be to have a static map that allows callbacks for
	//different windows, but I'll implement that if needed. The idea would be you use the window pointer
	//to look into the map and find the appropriate camera handler.
	SA::CameraBase* glfwCallbackHandler = nullptr;

	void mouseCallback(GLFWwindow* window, double xpos, double ypos)
	{
		if (glfwCallbackHandler && !glfwCallbackHandler->isInCursorMode())
		{
			glfwCallbackHandler->mouseMoved(xpos, ypos);
		}
	}

	void mouseLeftCallback(GLFWwindow* window, int enter)
	{
		if (glfwCallbackHandler)
		{
			glfwCallbackHandler->windowFocusedChanged(enter);
		}
	}

	void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
	{
		if (glfwCallbackHandler && !glfwCallbackHandler->isInCursorMode())
		{
			glfwCallbackHandler->mouseWheelUpdate(xOffset, yOffset);
		}
	}
}

namespace SA
{
	namespace CameraCallbackRegister
	{
		void registerExclusive(GLFWwindow& window, CameraBase& camera)
		{
			glfwCallbackHandler = &camera;
			glfwSetCursorPosCallback(&window, &mouseCallback);
			glfwSetCursorEnterCallback(&window, &mouseLeftCallback);
			glfwSetScrollCallback(&window, &scrollCallback);
		}

		void deregister(CameraBase& camera)
		{
			if (glfwCallbackHandler == &camera)
			{
				glfwCallbackHandler = nullptr;
				//glfwSetCursorPosCallback(&window, nullptr);
				//glfwSetCursorEnterCallback(&window, nullptr);
				//glfwSetScrollCallback(&window, nullptr);
			}
		}
	}
}
