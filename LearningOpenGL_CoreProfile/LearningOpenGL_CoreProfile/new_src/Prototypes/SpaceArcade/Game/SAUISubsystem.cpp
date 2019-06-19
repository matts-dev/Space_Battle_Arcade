#include "SAUISubsystem.h"

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include "../../../../Libraries/imgui.1.69.gl/imgui.h"
#include "../../../../Libraries/imgui.1.69.gl/imgui_impl_glfw.h"
#include "../../../../Libraries/imgui.1.69.gl/imgui_impl_opengl3.h"
#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SAWindowSubsystem.h"
#include "../Rendering/SAWindow.h"

namespace SA
{
	void UISubsystem::initSystem()
	{

		//requires window subsystem be available; which is safe within the initSystem 
		WindowSubsystem& windowSubsystem = GameBase::get().getWindowSubsystem();
		windowSubsystem.onWindowLosingOpenglContext.addWeakObj(sp_this(), &UISubsystem::handleLosingOpenGLContext);
		windowSubsystem.onWindowAcquiredOpenglContext.addWeakObj(sp_this(), &UISubsystem::handleWindowAcquiredOpenGLContext);

		//in case things are refactored and windows are created during subsystem initialization, this will catch
		//the edge case where a window is already created before we start listening to the primary changed delegate
		if (const sp<Window>& window = windowSubsystem.getPrimaryWindow())
		{
			handleWindowAcquiredOpenGLContext(window);
		}

		GameBase& game = GameBase::get();
		game.PostGameloopTick.addStrongObj(sp_this(), &UISubsystem::handleGameloopOver);
	}

	void UISubsystem::handleLosingOpenGLContext(const sp<Window>& window)
	{
		if (!imguiBoundWindow.expired())
		{
			//assuming window == imguiBoundWindow since it ImGui should always be associated with current bound context
			if (window)
			{
				window->onRawGLFWKeyCallback.removeStrong(sp_this(), &UISubsystem::handleRawGLFWKeyCallback);
				window->onRawGLFWCharCallback.removeStrong(sp_this(), &UISubsystem::handleRawGLFWCharCallback);
				window->onRawGLFWMouseButtonCallback.removeStrong(sp_this(), &UISubsystem::handleRawGLFWMouseButtonCallback);
				window->onRawGLFWScrollCallback.removeStrong(sp_this(), &UISubsystem::handleRawGLFWScroll);
				destroyImGuiContext();
			}
		}
	}

	void UISubsystem::handleWindowAcquiredOpenGLContext(const sp<Window>& window)
	{
		//make sure we have cleaned up the old context and have nullptr within the imguiBoundWindow
		assert(imguiBoundWindow.expired());

		if (window)
		{
			//set up IMGUI
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO imguiIO = ImGui::GetIO();
			ImGui::StyleColorsDark();
			ImGui_ImplOpenGL3_Init("#version 330 core");							 //seems to be window independent, but example code has this set after window
			ImGui_ImplGlfw_InitForOpenGL(window->get(), /*install callbacks*/false); //false will require manually calling callbacks in our own handlers
			imguiBoundWindow = window;

			//manually unregister these when window loses active context
			window->onRawGLFWKeyCallback.addStrongObj(sp_this(), &UISubsystem::handleRawGLFWKeyCallback);
			window->onRawGLFWCharCallback.addStrongObj(sp_this(), &UISubsystem::handleRawGLFWCharCallback);
			window->onRawGLFWMouseButtonCallback.addStrongObj(sp_this(), &UISubsystem::handleRawGLFWMouseButtonCallback);
			window->onRawGLFWScrollCallback.addStrongObj(sp_this(), &UISubsystem::handleRawGLFWScroll);
		}
	}

	void UISubsystem::handleRawGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	}

	void UISubsystem::handleRawGLFWCharCallback(GLFWwindow* window, unsigned int c)
	{
		ImGui_ImplGlfw_CharCallback(window, c);
	}

	void UISubsystem::handleRawGLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	}

	void UISubsystem::handleRawGLFWScroll(GLFWwindow* window, double xOffset, double yOffset)
	{
		ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
	}

	void UISubsystem::destroyImGuiContext()
	{
		//shut down IMGUI
		ImGui_ImplGlfw_Shutdown();
		ImGui_ImplOpenGL3_Shutdown();
		ImGui::DestroyContext();
		imguiBoundWindow = sp<Window>(nullptr);
	}

	void UISubsystem::shutdown()
	{
		if (!imguiBoundWindow.expired())
		{
			destroyImGuiContext();
		}
	}

	void UISubsystem::handleGameloopOver(float dt_sec)
	{
		if (!imguiBoundWindow.expired())
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			onUIFrameStarted.broadcast();
			//UI elements will draw during first broadcast

			ImGui::EndFrame(); 
			onUIFrameEnded.broadcast();
		}
	}

	void UISubsystem::render()
	{
		if (!imguiBoundWindow.expired())
		{
			//UI will have set up widgets between frame
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
	}
}

