#include "SAWindowSubsystem.h"
#include "SAGameBase.h"
#include "SALog.h"

namespace SA
{
	void WindowSubsystem::makeWindowPrimary(const sp<Window>& window)
	{
		onPrimaryWindowChangingEvent.broadcast(focusedWindow, window);

		onWindowLosingOpenglContext.broadcast(focusedWindow);
		if (window)
		{
			glfwMakeContextCurrent(window->get());
			onWindowAcquiredOpenglContext.broadcast(window);
		}


		focusedWindow = window;
	}

	void WindowSubsystem::tick(float deltaSec)
	{
		glfwPollEvents();

		if (focusedWindow)
		{
			if (focusedWindow->shouldClose())
			{
				if (onFocusedWindowTryingToClose.numBound() > 0)
				{
					//NOTE: below may result in focus window changing; making a sp copy per tick isn't cheap so be careful
					onFocusedWindowTryingToClose.broadcast(focusedWindow);
				}
				else
				{
					log("Window Subsystem : Primary window requesting close, shutting down game.");
					GameBase::get().startShutdown();
				}
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


