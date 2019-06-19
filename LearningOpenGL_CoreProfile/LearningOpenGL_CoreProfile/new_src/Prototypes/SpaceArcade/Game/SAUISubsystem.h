#pragma once
#include "..\GameFramework\SASubsystemBase.h"
#include "..\Tools\DataStructures\MultiDelegate.h"

struct GLFWwindow;

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
		
		void handleLosingOpenGLContext(const sp<Window>& window);
		void handleWindowAcquiredOpenGLContext(const sp<Window>& window);
		void handleRawGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		void handleRawGLFWCharCallback(GLFWwindow* window, unsigned int c);
		void handleRawGLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		void handleRawGLFWScroll(GLFWwindow* window, double xOffset, double yOffset);

		void handleGameloopOver(float dt_sec);

		void destroyImGuiContext();
		

	private:
		wp<Window> imguiBoundWindow;
	};
}