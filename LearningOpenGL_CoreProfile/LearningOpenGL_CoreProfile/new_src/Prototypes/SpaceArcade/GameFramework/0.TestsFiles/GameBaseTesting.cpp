#include "..\..\Rendering\SAWindow.h"
#include "..\..\Tools\SmartPointerAlias.h"
#include "..\..\Rendering\OpenGLHelpers.h"
#include "..\SAGameBase.h"
#include "..\SAWindowSubsystem.h"


namespace
{
	class ProtoGame : public SA::GameBase
	{
	public:

		static ProtoGame& get() 
		{
			static sp<ProtoGame> singleton = new_sp<ProtoGame>();
			return *singleton;
		}
	private:
		virtual sp<SA::Window> startUp() override
		{
			sp<SA::Window> window = new_sp<SA::Window>(1000, 500);
			return window;
		}

		virtual void TickGameLoopDerived(float deltaTimeSecs) override
		{
			using namespace SA;

			const sp<Window> window = getWindowSubsystem().getPrimaryWindow();
			if (!window)
			{
				startShutdown();
				return;
			}

			if (glfwGetKey(window->get(), GLFW_KEY_1) == GLFW_PRESS)
			{
				window->markWindowForClose(true);
				
			}

			ec(glClearColor(((sin((float)glfwGetTime()) + 1) / 2.0f), 0, 0, 1));
			ec(glClear(GL_COLOR_BUFFER_BIT));

			glfwSwapBuffers(window->get());
			glfwPollEvents();
		}
	};


	int trueMain()
	{
		ProtoGame& game = ProtoGame::get();
		game.start();
		return 0;
	}
}

int main()
{
	return trueMain();
}