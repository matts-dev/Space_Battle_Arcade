#pragma once
#include "..\..\GameFramework\SAGameEntity.h"

struct GLFWwindow;

namespace SA
{
	class Window;

	class CameraBase : public GameEntity
	{
	public:

		virtual ~CameraBase();

		virtual void registerToWindowCallbacks(sp<Window>& window) = 0;
		virtual void deregisterToWindowCallbacks() = 0;
	};
}
	
