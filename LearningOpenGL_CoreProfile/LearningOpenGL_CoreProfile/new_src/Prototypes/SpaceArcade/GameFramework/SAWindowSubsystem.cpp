#include "SAWindowSubsystem.h"

namespace SA
{
	void WindowSubsystem::makeWindowPrimary(const sp<Window>& window)
	{
		if (window)
		{
			glfwMakeContextCurrent(window->get());
		}

		primaryWindowChangingEvent.broadcast(focusedWindow, window);
		focusedWindow = window;
	}

	void WindowSubsystem::tick(float deltaSec)
	{
		if (focusedWindow)
		{
			//fill out as needed, perhaps this subsystem will handle drawing of windows
			if (focusedWindow->shouldClose())
			{
				makeWindowPrimary(nullptr);
			}
		}
	}
}


