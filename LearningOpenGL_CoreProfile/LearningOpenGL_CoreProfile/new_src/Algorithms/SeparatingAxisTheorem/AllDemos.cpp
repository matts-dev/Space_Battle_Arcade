#pragma once
#include<iostream>

#include<glad/glad.h> //include opengl headers, so should be before anything that uses those headers (such as GLFW)
#include<GLFW/glfw3.h>
#include <string>
#include<cmath>

#include "../../GettingStarted/Camera/CameraFPS.h"
#include "../../../InputTracker.h"
#include "../../nu_utils.h"
#include "../../../Shader.h"
#include "SATComponent.h"
#include "SATUnitTestUtils.h"
#include <gtx/quaternion.hpp>
#include <tuple>
#include <array>
#include "SATRenderDebugUtils.h"
#include "ModelLoader/SATModel.h"
#include "SATDemoInterface.h"

std::shared_ptr<ISATDemo> factory_ModelDemo(int width, int height);
std::shared_ptr<ISATDemo> factory_DynamicGeneratedPolyDemo(int width, int height);
std::shared_ptr<ISATDemo> factory_CapsuleShape(int width, int height);
std::shared_ptr<ISATDemo> factory_CubeShape(int width, int height);
std::shared_ptr<ISATDemo> factory_Demo2D(int width, int height);

namespace
{
	void true_main()
	{
		using glm::vec2;
		using glm::vec3;
		using glm::mat4;

		int width = 1200;
		int height = 800;

		GLFWwindow* window = init_window(width, height);

		glViewport(0, 0, width, height);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*window, int width, int height) {  glViewport(0, 0, width, height); });
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		std::shared_ptr<ISATDemo> sat2DDemo = factory_Demo2D(width, height);
		std::shared_ptr<ISATDemo> cubeShapeDemo = factory_CubeShape(width, height);
		std::shared_ptr<ISATDemo> capsuleShapeDemo = factory_CapsuleShape(width, height);
		std::shared_ptr<ISATDemo> dynCapsuleDemo = factory_DynamicGeneratedPolyDemo(width, height);
		std::shared_ptr<ISATDemo> modelDemo = factory_ModelDemo(width, height);
		std::vector<std::shared_ptr<ISATDemo>> demos = { sat2DDemo, cubeShapeDemo, capsuleShapeDemo, dynCapsuleDemo, modelDemo };

		std::shared_ptr<ISATDemo> activeDemo = sat2DDemo;
		activeDemo->handleModuleFocused(window);

		InputTracker input;

		std::cout << "To switch between Demos, hold left control and press the number keys (not numpad) \n\n\n"  << std::endl;

		/////////////////////////////////////////////////////////////////////
		// Game Loop
		/////////////////////////////////////////////////////////////////////
		while (!glfwWindowShouldClose(window))
		{
			input.updateState(window);
			if (input.isKeyDown(window, GLFW_KEY_LEFT_CONTROL))
			{
				for (unsigned int key = GLFW_KEY_0; key < GLFW_KEY_9 + 1; key++)
				{
					if (input.isKeyJustPressed(window, key))
					{
						unsigned int idx = key - GLFW_KEY_0;
						idx -= 1; //convert to 0 based
						idx = idx >= demos.size() ? demos.size() - 1 : idx;
						
						activeDemo = demos[idx];
						activeDemo->handleModuleFocused(window);
						break;
					}
				}
			}

			activeDemo->tickGameLoop(window);

			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		glfwTerminate();
	}
}

int main()
{
	true_main();
}