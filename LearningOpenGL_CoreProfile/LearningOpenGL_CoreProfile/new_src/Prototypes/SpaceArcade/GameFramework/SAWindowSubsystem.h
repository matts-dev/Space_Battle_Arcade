#pragma once
#include "SASubsystemBase.h"
#include "..\Tools\DataStructures\MultiDelegate.h"
#include "..\Rendering\SAWindow.h"
#include <cstdint>
#include <map>
#include <set>


namespace SA
{
	class WindowSubsystem : public SubsystemBase
	{
	public: //events
		/*This event should not be used to determine when OpenGL contexts change */
		MultiDelegate<const sp<Window>& /*old_window*/, const sp<Window>& /*new_window*/> primaryWindowChangingEvent;
		MultiDelegate<const sp<Window>&> onWindowLosingOpenglContext;
		MultiDelegate<const sp<Window>&> onWindowAcquiredOpenglContext;
		MultiDelegate<const sp<Window>&> onFocusedWindowTryingToClose;

	public:
		const sp<Window>& getPrimaryWindow() { return focusedWindow; }
		void makeWindowPrimary(const sp<Window>& window);

	private:
		virtual void initSystem() override;
		virtual void tick(float deltaSec) override;
		virtual void handlePostRender() override;

	private:
		sp<Window> focusedWindow = nullptr;
	};
}