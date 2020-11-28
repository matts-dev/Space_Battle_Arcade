#include "..\..\Rendering\SAWindow.h"
#include "..\..\Rendering\OpenGLHelpers.h"
#include "..\SAGameEntity.h"

namespace
{
	int trueMain()
	{
		using namespace SA;

		sp<SA::Window> window_1 = new_sp<SA::Window>(100, 50);
		sp<SA::Window> window_2 = new_sp<SA::Window>(200, 100);
		sp<SA::Window> window_3 = new_sp<SA::Window>(300, 200);

		glfwSetWindowPos(window_1->get(), 10, 100);
		glfwSetWindowPos(window_2->get(), 500, 250);
		glfwSetWindowPos(window_3->get(), 1000, 750);

		while (window_1 || window_2 || window_3)
		{
			//-------------WINDOW 1-------------
			if (window_1 && !glfwWindowShouldClose(window_1->get()))
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
			else
			{
				window_1 = nullptr;
			}

			//-------------WINDOW 2-------------
			
			if (window_2 && !glfwWindowShouldClose(window_2->get()))
			{
				if (glfwGetKey(window_2->get(), GLFW_KEY_2) == GLFW_PRESS)
				{
					window_2->markWindowForClose(true);
				}
				glfwMakeContextCurrent(window_2->get());
				ec(glClearColor(0, ((sin((float)glfwGetTime()) + 1) / 2.0f), 0, 1));
				ec(glClear(GL_COLOR_BUFFER_BIT));
				glfwSwapBuffers(window_2->get());
				glfwPollEvents();
			}
			else
			{
				window_2 = nullptr;
			}

			//-------------WINDOW 3-------------
			if (window_3 && !glfwWindowShouldClose(window_3->get()))
			{
				if (glfwGetKey(window_3->get(), GLFW_KEY_3) == GLFW_PRESS)
				{
					window_3->markWindowForClose(true);
				}
				glfwMakeContextCurrent(window_3->get());
				ec(glClearColor(0, 0, ((sin((float)glfwGetTime()) + 1) / 2.0f), 1));
				ec(glClear(GL_COLOR_BUFFER_BIT));
				glfwSwapBuffers(window_3->get());
				glfwPollEvents();
			}
			else
			{
				window_3 = nullptr;
			}
		}


		//test creating window after shutdown
		window_1 = new_sp<SA::Window>(50, 50);
		while (window_1)
		{
			if (window_1 && !glfwWindowShouldClose(window_1->get()))
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
			else
			{
				window_1 = nullptr;
			}
		}

		return 0;
	}
}

//int main()
//{
//	return trueMain();
//}