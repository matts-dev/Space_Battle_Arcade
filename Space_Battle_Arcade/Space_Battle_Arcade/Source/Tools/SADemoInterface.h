#pragma once

//#include <GLFW/glfw3.h>
struct GLFWwindow;

namespace SA
{
	class DemoInterface
	{
	public:
		DemoInterface(int width, int height) {}
		virtual void tickGameLoop(GLFWwindow* window) = 0;
		virtual void handleModuleFocused(GLFWwindow* window) = 0;
	};
	
}