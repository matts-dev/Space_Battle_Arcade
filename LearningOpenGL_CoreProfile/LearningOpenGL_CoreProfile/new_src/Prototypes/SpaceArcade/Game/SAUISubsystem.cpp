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
		//set up IMGUI
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO imguiIO = ImGui::GetIO();
		ImGui::StyleColorsDark();
		ImGui_ImplOpenGL3_Init("#version 330 core"); //seems to be window independent, but example code has this set after window

		//requires window subsystem be available; which is safe within the initSystem 
		WindowSubsystem& windowSubsystem = GameBase::get().getWindowSubsystem();
		windowSubsystem.primaryWindowChangingEvent.addWeakObj(sp_this(), &UISubsystem::handlePrimaryWindowChangingEvent);

		//in case things are refactored and windows are created during subsystem initialization, this will catch
		//the edge case where a window is already created before we start listening to the primary changed delegate
		if (const sp<Window>& window = windowSubsystem.getPrimaryWindow())
		{
			handlePrimaryWindowChangingEvent(nullptr, window);
		}

		GameBase& game = GameBase::get();
		game.PostGameloopTick.addStrongObj(sp_this(), &UISubsystem::handleGameloopOver);
	}

	void UISubsystem::handlePrimaryWindowChangingEvent(const sp<Window>& old_window, const sp<Window>& new_window)
	{
		//if there isn't a new window, we're probably shutting down
		if (old_window)
		{
			//unregister from old window; untested and this may not work
			ImGui_ImplGlfw_Shutdown();
			imguiBoundWindow = sp<Window>(nullptr);
		}

		if (new_window)
		{
			ImGui_ImplGlfw_InitForOpenGL(new_window->get(), /*install callbacks*/false); //false will require manually calling callbacks in our own handlers
			imguiBoundWindow = new_window;
		}
	}

	void UISubsystem::shutdown()
	{
		//shut down IMGUI
		if (!imguiBoundWindow.expired())
		{
			ImGui_ImplGlfw_Shutdown();
			imguiBoundWindow = sp<Window>(nullptr);
		}
		ImGui_ImplOpenGL3_Shutdown();
		ImGui::DestroyContext();
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

