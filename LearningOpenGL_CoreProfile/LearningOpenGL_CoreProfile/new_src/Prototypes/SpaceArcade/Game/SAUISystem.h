#pragma once
#include "..\GameFramework\SASystemBase.h"
#include "..\Tools\DataStructures\MultiDelegate.h"

struct GLFWwindow;

namespace SA
{
	class Window;

	/** UI system fpr the developer menus; separate from in game UI. */
	class UISystem_Editor : public SystemBase
	{
	public:
		MultiDelegate<> onUIFrameStarted;
		MultiDelegate<> onUIFrameEnded;
		MultiDelegate<> onUIGameRender;

		inline void setEditorUIEnabled(bool bEnable) { bUIEnabled = bEnable; }
		inline bool getEditorUIEnabled() { return bUIEnabled; }

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

		void handleRenderDispatchEnding(float dt_sec);
		void processUIFrame();

		void destroyImGuiContext();

	private:
		wp<Window> imguiBoundWindow;
		bool bUIEnabled = true;
	};
}