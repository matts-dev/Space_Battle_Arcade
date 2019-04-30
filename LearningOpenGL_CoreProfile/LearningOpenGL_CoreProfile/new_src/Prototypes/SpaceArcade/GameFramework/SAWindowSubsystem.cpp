#include "SAWindowSubsystem.h"
#include "SAGameBase.h"

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
		glfwPollEvents();
		if (focusedWindow)
		{
			if (focusedWindow->shouldClose())
			{
				//perhaps try to find another valid window
				makeWindowPrimary(nullptr);
			}
		}
	}

	void WindowSubsystem::handlePostRender()
	{
		//will need to do this for all windows that were rendered to if supporting more than a single window
		if (focusedWindow)
		{
			glfwSwapBuffers(focusedWindow->get());
		}
	}

	void WindowSubsystem::initSystem()
	{
		GameBase::get().SubscribePostRender(sp_this());
	}
}


