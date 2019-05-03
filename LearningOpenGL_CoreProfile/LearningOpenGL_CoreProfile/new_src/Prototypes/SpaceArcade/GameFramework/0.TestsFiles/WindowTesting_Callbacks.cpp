#include "..\..\Rendering\SAWindow.h"
#include "..\..\Rendering\OpenGLHelpers.h"
#include "..\SAGameEntity.h"

namespace
{
	class MouseMonitor : public SA::GameEntity
	{
	public:
		void posUpdate(double x, double y)
		{
			std::cout << x << " " << y << std::endl;
		}
	};

	int trueMain()
	{
		using namespace SA;

		sp<SA::Window> window_1 = new_sp<SA::Window>(1000, 500);

		sp<MouseMonitor> mm = new_sp<MouseMonitor>();
		window_1->cursorPosEvent.addWeakObj(mm, &MouseMonitor::posUpdate);

		while (window_1 && !glfwWindowShouldClose(window_1->get()))
		{
			if (glfwGetKey(window_1->get(), GLFW_KEY_1) == GLFW_PRESS)
			{
				window_1->markWindowForClose(true);
			}
			glfwMakeContextCurrent(window_1->get());
			ec(glClearColor(((sin((float)glfwGetTime()) + 1) / 2.0f), 0, 0, 1));
			ec(glClear(GL_COLOR_BUFFER_BIT));
			glfwSwapBuffers(window_1->get());
			glfwPollEvents();
		}

		return 0;
	}
}

//int main()
//{
//	return trueMain();
//}