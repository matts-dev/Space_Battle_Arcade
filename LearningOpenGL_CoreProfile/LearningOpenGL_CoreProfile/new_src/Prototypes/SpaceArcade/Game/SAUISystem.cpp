#include "SAUISystem.h"

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include "../../../../Libraries/imgui.1.69.gl/imgui.h"
#include "../../../../Libraries/imgui.1.69.gl/imgui_impl_glfw.h"
#include "../../../../Libraries/imgui.1.69.gl/imgui_impl_opengl3.h"
#include "../GameFramework/SAGameBase.h"
#include "../GameFramework/SAWindowSystem.h"
#include "../Rendering/SAWindow.h"

namespace SA
{
	void UISystem::initSystem()
	{

		//requires window system be available; which is safe within the initSystem 
		WindowSystem& windowSystem = GameBase::get().getWindowSystem();
		windowSystem.onWindowLosingOpenglContext.addWeakObj(sp_this(), &UISystem::handleLosingOpenGLContext);
		windowSystem.onWindowAcquiredOpenglContext.addWeakObj(sp_this(), &UISystem::handleWindowAcquiredOpenGLContext);

		//in case things are refactored and windows are created during system initialization, this will catch
		//the edge case where a window is already created before we start listening to the primary changed delegate
		if (const sp<Window>& window = windowSystem.getPrimaryWindow())
		{
			handleWindowAcquiredOpenGLContext(window);
		}

		GameBase& game = GameBase::get();
		game.PostGameloopTick.addStrongObj(sp_this(), &UISystem::handleGameloopOver); //#future this should be a render command that is sorted after post-processing. 
	}

	void UISystem::handleLosingOpenGLContext(const sp<Window>& window)
	{
		if (!imguiBoundWindow.expired())
		{
			//assuming window == imguiBoundWindow since it ImGui should always be associated with current bound context
			if (window)
			{
				window->onRawGLFWKeyCallback.removeStrong(sp_this(), &UISystem::handleRawGLFWKeyCallback);
				window->onRawGLFWCharCallback.removeStrong(sp_this(), &UISystem::handleRawGLFWCharCallback);
				window->onRawGLFWMouseButtonCallback.removeStrong(sp_this(), &UISystem::handleRawGLFWMouseButtonCallback);
				window->onRawGLFWScrollCallback.removeStrong(sp_this(), &UISystem::handleRawGLFWScroll);
				destroyImGuiContext();
			}
		}
	}

	void UISystem::handleWindowAcquiredOpenGLContext(const sp<Window>& window)
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
			window->onRawGLFWKeyCallback.addStrongObj(sp_this(), &UISystem::handleRawGLFWKeyCallback);
			window->onRawGLFWCharCallback.addStrongObj(sp_this(), &UISystem::handleRawGLFWCharCallback);
			window->onRawGLFWMouseButtonCallback.addStrongObj(sp_this(), &UISystem::handleRawGLFWMouseButtonCallback);
			window->onRawGLFWScrollCallback.addStrongObj(sp_this(), &UISystem::handleRawGLFWScroll);
		}
	}

	void UISystem::handleRawGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	}

	void UISystem::handleRawGLFWCharCallback(GLFWwindow* window, unsigned int c)
	{
		ImGui_ImplGlfw_CharCallback(window, c);
	}

	void UISystem::handleRawGLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	}

	void UISystem::handleRawGLFWScroll(GLFWwindow* window, double xOffset, double yOffset)
	{
		ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
	}

	void UISystem::destroyImGuiContext()
	{
		//shut down IMGUI
		ImGui_ImplGlfw_Shutdown();
		ImGui_ImplOpenGL3_Shutdown();
		ImGui::DestroyContext();
		imguiBoundWindow = sp<Window>(nullptr);
	}

	void UISystem::shutdown()
	{
		if (!imguiBoundWindow.expired())
		{
			destroyImGuiContext();
		}
	}

	void UISystem::handleGameloopOver(float dt_sec)
	{
		if (!imguiBoundWindow.expired() && bUIEnabled)
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

	void UISystem::render()
	{
		if (!imguiBoundWindow.expired() && bUIEnabled)
		{
			//UI will have set up widgets between frame
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
	}
}

