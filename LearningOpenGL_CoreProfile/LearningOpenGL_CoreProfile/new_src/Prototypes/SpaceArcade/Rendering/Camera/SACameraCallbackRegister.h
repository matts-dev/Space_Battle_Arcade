#pragma once

struct GLFWwindow;
class CameraBase;

namespace SA
{
	namespace CameraCallbackRegister
	{
		void registerExclusive(GLFWwindow& window, CameraBase& camera);
		void deregister(CameraBase& camera);
	};
}

