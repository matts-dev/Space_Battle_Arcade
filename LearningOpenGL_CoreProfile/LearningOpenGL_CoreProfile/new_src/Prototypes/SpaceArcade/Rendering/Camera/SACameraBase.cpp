#include "SACameraBase.h"
#include <iostream>

#include<glad/glad.h> //includes opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include "SACameraCallbackRegister.h"

SA::CameraBase::~CameraBase()
{
	//clean up if this is currently the registered camera for callbacks
	CameraCallbackRegister::deregister(*this);
}

void SA::CameraBase::registerToWindowCallbacks(GLFWwindow* window)
{
	if (window)
	{
		CameraCallbackRegister::registerExclusive(*window, *this);
	}
	else
	{
		std::cerr << "warning: null window passed when attempting to register for glfw mouse callbacks" << std::endl;
	}
}

