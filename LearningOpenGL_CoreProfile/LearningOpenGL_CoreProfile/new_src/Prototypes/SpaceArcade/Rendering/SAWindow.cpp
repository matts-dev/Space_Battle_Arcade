#include "SAWindow.h"

#include <stdexcept>
#include <iostream>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>

#include "OpenGLHelpers.h"

static bool gladLoaded = false;

namespace SA
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//STATIC GLFW MANAGEMENT
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	uint32_t Window::windowInstances = 0;
	void Window::startUp()
	{
		if (windowInstances == 0)
		{
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		}
	}
	void Window::shutDown()
	{
		if (windowInstances == 0)
		{
			glfwTerminate();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// C CALLBACK MANAGEMENT
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::unordered_map<GLFWwindow*, Window*> Window::windowMap;

	void Window::_cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		//for small number of windows, it is probably faster to walk over a vector to find associated object
		auto winIter = windowMap.find(window);
		if (winIter != windowMap.end())
		{
			Window* winObj = winIter->second;
			//winObj->CursorPosCallback(xpos, ypos);

		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Window Instances
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Window::Window(int width, int height)
	{
		startUp();
		++windowInstances;

		window = glfwCreateWindow(width, height, "OpenGL Window", nullptr, nullptr);
		if (!window)
		{
			glfwTerminate();
			throw std::runtime_error("FATAL: FAILED TO CREATE WINDOW");
		}

		//must be done everytime something is rendered to this window
		glfwMakeContextCurrent(window);
		
		//I believe GLAD proc address only needs to happen once
		if (!gladLoaded)
		{
			gladLoaded = true;

			if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress)))
			{
				std::cerr << "Failed to set up GLAD" << std::endl;
				glfwTerminate();
				throw std::runtime_error("FATAL: FAILED GLAD LOAD PROC");
			}
		}

		glfwSetCursorPosCallback(window, &Window::_cursorPosCallback);
		windowMap.insert({ window, this });

	}

	Window::~Window()
	{
		windowMap.erase(window);
		glfwDestroyWindow(window);
		windowInstances--;
		shutDown();
	}

	void Window::markWindowForClose(bool bClose)
	{
		glfwSetWindowShouldClose(window, bClose);
	}

	void Window::setViewportToWindowSize()
	{
		//window size is not in pixels, but frame size returns the framebuffer size which is pixels.
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		ec(glViewport(0, 0, width, height));
	}

}
