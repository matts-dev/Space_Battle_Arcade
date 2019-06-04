#pragma once
#include "..\GameFramework\SASubsystemBase.h"
#include "..\Tools\DataStructures\MultiDelegate.h"

namespace SA
{
	class Window;

	class UISubsystem : public SubsystemBase
	{
	public:
		MultiDelegate<> onUIFrameStarted;
		MultiDelegate<> onUIFrameEnded;

		//rendering of UI may need refactoring; currently rendering is entirely done within renderloop game subclass method
		//there's not event that is pre-buffer swapping to hook into. Doesn't seem necessary to change at this time but it 
		//would be a cleaner system if subclass didn't have to hook this into render loop manually
		void render();

	private:
		virtual void tick(float deltaSec) {}
		virtual void initSystem() override;
		virtual void shutdown() override;
		
		void handlePrimaryWindowChangingEvent(const sp<Window>& old_window, const sp<Window>& new_window);
		void handleGameloopOver(float dt_sec);
		

	private:
		wp<Window> imguiBoundWindow;
	};
}