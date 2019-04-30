#pragma once
#include "SASubsystemBase.h"
#include "..\Tools\SmartPointerAlias.h"
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
		MultiDelegate<const sp<Window>& /*old_window*/, const sp<Window>& /*new window*/> primaryWindowChangingEvent;

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